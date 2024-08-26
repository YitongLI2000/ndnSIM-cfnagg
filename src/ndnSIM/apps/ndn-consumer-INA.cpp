/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2018  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * This class is written by Yitong, which inherits "ndn-consumer" class, aiming to provide cwnd management and congestion function
 * This class is the one being instantiated in ndnSIM's scenario file
 **/

#include "ndn-consumer-INA.hpp"
#include <fstream>
#include <string>

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerINA");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerINA);

/**
 * Initiate attributes for consumer class, some of them may be used, some are optional
 *
 * @return Total TypeId
 */
TypeId
ConsumerINA::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerINA")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<ConsumerINA>()

      .AddAttribute("Alpha",
                    "TCP Multiplicative Decrease factor",
                    DoubleValue(0.5),
                    MakeDoubleAccessor(&ConsumerINA::m_alpha),
                    MakeDoubleChecker<double>())
      .AddAttribute("Beta",
                    "Local congestion decrease factor",
                    DoubleValue(0.6),
                    MakeDoubleAccessor(&ConsumerINA::m_beta),
                    MakeDoubleChecker<double>())
      .AddAttribute("Gamma",
                    "Remote congestion decrease factor",
                    DoubleValue(0.7),
                    MakeDoubleAccessor(&ConsumerINA::m_gamma),
                    MakeDoubleChecker<double>())
      .AddAttribute("AddRttSuppress",
                    "Minimum number of RTTs (1 + this factor) between window decreases",
                    DoubleValue(0.5), // This default value was chosen after manual testing
                    MakeDoubleAccessor(&ConsumerINA::m_addRttSuppress),
                    MakeDoubleChecker<double>())
      .AddAttribute("ReactToCongestionMarks",
                    "If true, process received congestion marks",
                    BooleanValue(true),
                    MakeBooleanAccessor(&ConsumerINA::m_reactToCongestionMarks),
                    MakeBooleanChecker())
      .AddAttribute("UseCwa",
                    "If true, use Conservative Window Adaptation",
                    BooleanValue(false),
                    MakeBooleanAccessor(&ConsumerINA::m_useCwa),
                    MakeBooleanChecker())
      .AddAttribute("Window", "Initial size of the window", StringValue("1"),
                    MakeUintegerAccessor(&ConsumerINA::GetWindow, &ConsumerINA::SetWindow),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("InitialWindowOnTimeout", "Set window to initial value when timeout occurs",
                    BooleanValue(true),
                    MakeBooleanAccessor(&ConsumerINA::m_setInitialWindowOnTimeout),
                    MakeBooleanChecker())
      .AddTraceSource("WindowTrace",
                      "Window that controls how many outstanding interests are allowed",
                      MakeTraceSourceAccessor(&ConsumerINA::m_window),
                      "ns3::ndn::ConsumerINA::WindowTraceCallback")
      .AddTraceSource("InFlight", "Current number of outstanding interests",
                      MakeTraceSourceAccessor(&ConsumerINA::m_inFlight),
                      "ns3::ndn::ConsumerINA::WindowTraceCallback");
  return tid;
}



/**
 * Constructor
 */
ConsumerINA::ConsumerINA()
    : m_inFlight(0)
    , m_ssthresh(std::numeric_limits<double>::max())
    , m_highData(0)
    , m_recPoint(0.0)
{
}



/**
 * Override from consumer class
 * @param newName
 */
void
ConsumerINA::SendInterest(shared_ptr<Name> newName)
{
    m_inFlight++;
    Consumer::SendInterest(newName);
}



/**
 * Based on cwnd, schedule when to send packets
 */
void
ConsumerINA::ScheduleNextPacket()
{
    if (!broadcastSync && globalSeq != 0) {
        NS_LOG_INFO("Haven't finished tree broadcasting synchronization, don't send actual data packet for now.");
    }
    else if (m_window == static_cast<uint32_t>(0)) {
        Simulator::Remove(m_sendEvent);

        NS_LOG_DEBUG("Error! Window becomes 0!!!!!!");
        //NS_LOG_DEBUG("New event in " << (std::min<double>(0.5, m_rtt->RetransmitTimeout().ToDouble(Time::S))) << " sec");
        m_sendEvent = Simulator::Schedule(Seconds(std::min<double>(0.5, (m_retxTimer*6).GetSeconds())),
                                          &Consumer::SendPacket, this);
    }
    else if (m_window - m_inFlight <= 0) {
        NS_LOG_INFO("Wait until cwnd allows new transmission.");
        NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);
        // do nothing
    }
    else {
        if (m_sendEvent.IsRunning()) {
            Simulator::Remove(m_sendEvent);
        }
        NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);
        m_sendEvent = Simulator::ScheduleNow(&Consumer::SendPacket, this);
    }
}



/**
 * Override from consumer
 */
void
ConsumerINA::StartApplication()
{
    // Initialize log files
    InitializeLogFile();

    Consumer::StartApplication();
}



/**
 * Override from consumer, perform AIMD congestion control (add cnwd by 1 every time a packet is received)
 * @param data
 */
void
ConsumerINA::OnData(shared_ptr<const Data> data)
{
    Consumer::OnData(data);


    std::string dataName = data->getName().toUri();
    uint64_t sequenceNum = data->getName().get(-1).toSequenceNumber();

    // Set highest received Data to sequence number
    if (m_highData < sequenceNum) {
        m_highData = sequenceNum;
    }

    if (!broadcastSync) {
        NS_LOG_INFO("Currently broadcasting aggregation tree, ignore relevant cwnd/congestion management");
    }
    else if (data->getCongestionMark() > 0) {
        if (m_reactToCongestionMarks) {
            NS_LOG_DEBUG("Received congestion mark: " << data->getCongestionMark());
            WindowDecrease("ConsumerCongestion");
        }
        else {
            NS_LOG_DEBUG("Ignored received congestion mark: " << data->getCongestionMark());
        }
    }
    else if (ECNLocal) {
        NS_LOG_INFO("Congestion signal exists in consumer!");
        WindowDecrease("ConsumerCongestion");
    }
/*    else if (ECNRemote) {
        NS_LOG_INFO("Congestion signal exists in aggregator!");
        WindowDecrease("AggregatorCongestion");
    }*/
    else {
        NS_LOG_INFO("No congestion, increase the cwnd.");
        WindowIncrease();
    }

    if (m_inFlight > static_cast<uint32_t>(0)) {
        m_inFlight--;
    }

    NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);

    // Record cwnd
    WindowRecorder();

    ScheduleNextPacket();
}



/**
 * Multiplicative decrease cwnd when timeout
 * @param nameString
 */
void
ConsumerINA::OnTimeout(std::string nameString)
{
    WindowDecrease("timeout");

    if (m_inFlight > static_cast<uint32_t>(0)) {
        m_inFlight--;
    }

    NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);

    Consumer::OnTimeout(nameString);
}



/**
 * Set cwnd
 * @param window
 */
void
ConsumerINA::SetWindow(uint32_t window)
{
    m_initialWindow = window;
    m_window = m_initialWindow;
}



/**
 * Return cwnd
 * @return cwnd
 */
uint32_t
ConsumerINA::GetWindow() const
{
    return m_initialWindow;
}



/**
 * Increase cwnd
 */
void
ConsumerINA::WindowIncrease()
{
    if (m_window < m_ssthresh) {
        m_window += 1.0;
    }else {
        m_window += (1.0 / m_window);
    }
    NS_LOG_DEBUG("Window size increased to " << m_window);
}



/**
 * Decrease cwnd
 * @param type
 */
void
ConsumerINA::WindowDecrease(std::string type)
{
    if (!m_useCwa || m_highData > m_recPoint) {
        // ToDo: "m_seq" is pending modification, it needs to be modified into the one recording current max seq that has been sent out
        const double diff = m_seq - m_highData;
        BOOST_ASSERT(diff >= 0);

        m_recPoint = m_seq + (m_addRttSuppress * diff);

        // AIMD for timeout
        if (type == "timeout") {
            m_ssthresh = m_window * m_alpha;
            m_window = m_ssthresh;
        }
        else if (type == "ConsumerCongestion") {
            m_ssthresh = m_window * m_beta;
            m_window = m_ssthresh;
        }
        else if (type == "AggregatorCongestion") {
            m_ssthresh = m_window * m_gamma;
            m_window = m_ssthresh;
        }

        // Window size can't be reduced below initial size
        if (m_window < m_initialWindow) {
            m_window = m_initialWindow;
        }

        NS_LOG_DEBUG("Encounter " << type << ". Window size decreased to " << m_window);
    }
    else {
        NS_LOG_DEBUG("Window decrease suppressed, HighData: " << m_highData << ", RecPoint: " << m_recPoint);
    }
}



/**
 * Record window every 5 ms, store them into a file
 */
void
ConsumerINA::WindowRecorder()
{
    // Open file; on first call, truncate it to delete old content
    std::ofstream file(windowTimeRecorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << responseTime_recorder << std::endl;
        return;
    }

    file << ns3::Simulator::Now().GetMilliSeconds() << " " << m_window << std::endl;

    file.close();
}



/**
 * Initialize log file for "consumer_window.txt"
 */
void
ConsumerINA::InitializeLogFile()
{
    // Check whether object path exists, create it if not
    CheckDirectoryExist(folderPath);

    // Open the file and clear all contents for log file
    OpenFile(windowTimeRecorder);

    Consumer::InitializeLogFile();
}

} // namespace ndn
} // namespace ns3
