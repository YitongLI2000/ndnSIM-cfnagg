//
// Created by 李怿曈 on 16/4/2024.
//

#include "ndn-aggregator.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "ModelData.hpp"
#include "ndn-consumer.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include <limits>

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <iostream>
#include <sstream>

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.Aggregator");

namespace ns3{
namespace ndn{

NS_OBJECT_ENSURE_REGISTERED(Aggregator);



/**
 * Initiate attributes for consumer class, some of them may be used, some are optional
 *
 * @return Total TypeId
 */
TypeId
Aggregator::GetTypeId(void)
{
    static TypeId tid =
            TypeId("ns3::ndn::Aggregator")
            .SetGroupName("Ndn")
            .SetParent<App>()
            .AddConstructor<Aggregator>()
            .AddAttribute("StartSeq", "Starting sequence number", IntegerValue(0),
                          MakeIntegerAccessor(&Aggregator::m_seq), MakeIntegerChecker<int32_t>())
            .AddAttribute("Prefix", "Interest prefix/name", StringValue("/"),
                          MakeNameAccessor(&Aggregator::m_prefix), MakeNameChecker())
            .AddAttribute("LifeTime", "Life time for interest packet", StringValue("4s"),
                          MakeTimeAccessor(&Aggregator::m_interestLifeTime), MakeTimeChecker())
            .AddAttribute("RetxTimer",
                          "Timeout defining how frequent retransmission timeouts should be checked",
                          StringValue("50ms"),
                          MakeTimeAccessor(&Aggregator::GetRetxTimer, &Aggregator::SetRetxTimer),
                          MakeTimeChecker())
            .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                          TimeValue(Seconds(0)), MakeTimeAccessor(&Aggregator::m_freshness),
                          MakeTimeChecker())
            .AddAttribute("Signature","Fake signature, 0 valid signature (default), other values application-specific",
                          UintegerValue(0), MakeUintegerAccessor(&Aggregator::m_signature),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("KeyLocator",
                          "Name to be used for key locator.  If root, then key locator is not used",
                          NameValue(), MakeNameAccessor(&Aggregator::m_keyLocator), MakeNameChecker())
            .AddAttribute("Window", "Initial size of the window", StringValue("1"),
                          MakeUintegerAccessor(&Aggregator::GetWindow, &Aggregator::SetWindow),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("MaxSeq", "Maximum sequence number to request (alternative to Size attribute, "
                                    "would activate only if Size is -1). "
                                    "The parameter is activated only if Size negative (not set)",
                            IntegerValue(std::numeric_limits<uint32_t>::max()),
                            MakeUintegerAccessor(&Aggregator::GetSeqMax, &Aggregator::SetSeqMax),
                            MakeUintegerChecker<uint32_t>())
            .AddAttribute("InitialWindowOnTimeout", "Set window to initial value when timeout occurs",
                          BooleanValue(true),
                          MakeBooleanAccessor(&Aggregator::m_setInitialWindowOnTimeout),
                          MakeBooleanChecker())
            .AddAttribute("Beta", "TCP Multiplicative Decrease factor", DoubleValue(0.5),
                          MakeDoubleAccessor(&Aggregator::m_beta),
                          MakeDoubleChecker<double>())
            .AddAttribute("AddRttSuppress", "Minimum number of RTTs (1 + this factor) between window decreases",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&Aggregator::m_addRttSuppress),
                          MakeDoubleChecker<double>())
            .AddAttribute("ReactToCongestionMarks",
                          "If true, process received congestion marks",
                          BooleanValue(true),
                          MakeBooleanAccessor(&Aggregator::m_reactToCongestionMarks),
                          MakeBooleanChecker())
            .AddAttribute("UseCwa",
                          "If true, use Conservative Window Adaptation",
                          BooleanValue(true),
                          MakeBooleanAccessor(&Aggregator::m_useCwa),
                          MakeBooleanChecker())
            .AddTraceSource("LastRetransmittedInterestDataDelay",
                            "Delay between last retransmitted Interest and received Data",
                            MakeTraceSourceAccessor(&Aggregator::m_lastRetransmittedInterestDataDelay),
                            "ns3::ndn::Aggregator::LastRetransmittedInterestDataDelayCallback")
            .AddTraceSource("FirstInterestDataDelay",
                            "Delay between first transmitted Interest and received Data",
                            MakeTraceSourceAccessor(&Aggregator::m_firstInterestDataDelay),
                            "ns3::ndn::Aggregator::FirstInterestDataDelayCallback")
            .AddTraceSource("WindowTrace",
                            "Window that controls how many outstanding interests are allowed",
                            MakeTraceSourceAccessor(&Aggregator::m_window),
                            "ns3::ndn::Aggregator::WindowTraceCallback")
            .AddTraceSource("InFlight", "Current number of outstanding interests",
                            MakeTraceSourceAccessor(&Aggregator::m_inFlight),
                            "ns3::ndn::Aggregator::WindowTraceCallback");
    return tid;

}



/**
 * Constructor
 */
Aggregator::Aggregator()
    : m_rand(CreateObject<UniformRandomVariable>())
    , treeSync(false)
    , RTT_threshold(0)
    , m_inFlight(0)
    , m_ssthresh(std::numeric_limits<double>::max())
    , m_highData(0)
    , m_recPoint(0.0)
    , m_alpha(0.6)
    , m_gamma(0.7)
    , m_seq(0)
    , SRTT(0)
    , RTTVAR(0)
    , roundRTT(0)
    , m_timeoutList(100)
    , totalResponseTime(0)
    , round(0)
    , totalAggregateTime(0)
    , iteration(0)
{
    m_rtt = CreateObject<RttMeanDeviation>();
}



/**
 * Intermediate function to parse string, used in aggTreeProcessStrings()
 * @param input
 * @return Parsed string
 */
std::pair<std::string, std::set<std::string>>
Aggregator::aggTreeProcessSingleString(const std::string& input)
{
    std::istringstream iss(input);
    std::string segment;
    std::vector<std::string> segments;

    // Use getline to split the string by '.'
    while (getline(iss, segment, '.')) {
        segments.push_back(segment);
    }

    // Check if there are enough segments to form a key and a set
    if (segments.size() > 1) {
        std::string key = segments[0];
        std::set<std::string> values(segments.begin() + 1, segments.end());
        return {key, values};
    }

    return {};  // Return an empty pair if not enough segments
}



/**
 * When receive "initialization" message (tree construction) from consumer, parse the message to get required info
 * @param inputs
 * @return A map consist child node info for current aggregator
 */
std::map<std::string, std::set<std::string>>
Aggregator::aggTreeProcessStrings(const std::vector<std::string>& inputs)
{
    std::map<std::string, std::set<std::string>> result;

    for (const std::string& input : inputs) {
        auto entry = aggTreeProcessSingleString(input);
        if (!entry.first.empty()) {
            result[entry.first].insert(entry.second.begin(), entry.second.end());
        }
    }

    return result;
}



/**
 * Sum response time
 * @param response_time
 */
void
Aggregator::ResponseTimeSum (int64_t response_time)
{
    totalResponseTime += response_time;
    ++round;
}



/**
 * Compute average for response time
 * @return Average response time
 */
int64_t
Aggregator::GetResponseTimeAverage()
{
    if (round == 0)
    {
        NS_LOG_DEBUG("Error happened when calculating average response time!");
        return 0;
    }

    return totalResponseTime / round;
}



/**
 * Sum aggregation time from each iteration
 * @param aggregate_time
 */
void
Aggregator::AggregateTimeSum (int64_t aggregate_time)
{
    totalAggregateTime += aggregate_time;
    ++iteration;
}



/**
 * Get average aggregation time
 * @return Average aggregation time
 */
int64_t
Aggregator::GetAggregateTimeAverage()
{
    if (iteration == 0)
    {
        NS_LOG_DEBUG("Error happened when calculating aggregate time!");
        return 0;
    }

    return totalAggregateTime / iteration;
}



/**
 * Check timeout every certain interval
 */
void
Aggregator::CheckRetxTimeout()
{
    Time now = Simulator::Now();

    //NS_LOG_INFO("Checking timeout. Current inFlight: " << m_inFlight);

    //NS_LOG_DEBUG("Check timeout after: " << m_retxTimer.GetMilliSeconds() << " ms");
    //NS_LOG_DEBUG("Current timeout threshold is: " << m_timeoutThreshold.GetMilliSeconds() << " ms");

    for (auto it = m_timeoutCheck.begin(); it != m_timeoutCheck.end();){
        NS_LOG_INFO("Interest name: " << it->first);
        if (now - it->second > m_timeoutThreshold) {
            std::string name = it->first;
            it = m_timeoutCheck.erase(it);
            OnTimeout(name);
        } else {
            ++it;
        }
    }
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Aggregator::CheckRetxTimeout, this);
}



/**
 * Based on RTT of the first iteration, compute their RTT average as threshold, use the threshold for congestion control
 * @param responseTime
 */
void
Aggregator::RTTThresholdMeasure(int64_t responseTime)
{
    RTT_threshold_vec.push_back(responseTime);
    if (RTT_threshold_vec.size() == numChild) {
        int64_t sum = 0;
        for (int64_t item: RTT_threshold_vec) {
            sum += item;
        }
        RTT_threshold = 1.2 * (sum / numChild);

        if (RTT_threshold != 0) {
            NS_LOG_INFO("RTT_threshold is set as: " << RTT_threshold << " ms");
        } else {
            NS_LOG_INFO("Error! RTT_threshold is set as 0, please check!");
            ns3::Simulator::Stop();
        }
    }
}



/**
 * Measure new RTO
 * @param resTime
 * @return New RTO
 */
Time
Aggregator::RTOMeasurement(int64_t resTime)
{
    if (roundRTT == 0) {
        RTTVAR = resTime / 2;
        SRTT = resTime;
    } else {
        RTTVAR = 0.75 * RTTVAR + 0.25 * std::abs(SRTT - resTime); // RTTVAR = (1 - b) * RTTVAR + b * |SRTT - RTTsample|, where b = 0.25
        SRTT = 0.875 * SRTT + 0.125 * resTime; // SRTT = (1 - a) * SRTT + a * RTTsample, where a = 0.125
    }
    roundRTT++;
    int64_t RTO = SRTT + 4 * RTTVAR; // RTO = SRTT + K * RTTVAR, where K = 4

    return MilliSeconds(2 * RTO);
}



/**
 * Triggered when timeout
 * @param nameString
 */
void
Aggregator::OnTimeout(std::string nameString)
{
    // Designed for AIMD
    WindowDecrease("timeout");

    // Start tracing timeout packets
    m_timeoutList.push_back(nameString);

    if (m_inFlight > static_cast<uint32_t>(0)){
        m_inFlight--;
    } else {
        NS_LOG_DEBUG("m_inFlight is 0, stop.");
        Simulator::Stop();
    }
    NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);

    shared_ptr<Name> name = make_shared<Name>(nameString);
    SendInterest(name);

}



/**
 * Set initial timeout checking interval
 * @param retxTimer
 */
void
Aggregator::SetRetxTimer(Time retxTimer)
{
    m_retxTimer = retxTimer;
    if (m_retxEvent.IsRunning()) {
        Simulator::Remove(m_retxEvent);
    }

    // Schedule new timeout
    m_timeoutThreshold = retxTimer;
    //NS_LOG_DEBUG("Next interval to check timeout is: " << m_retxTimer.GetMilliSeconds() << " ms");
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Aggregator::CheckRetxTimeout, this);
}



/**
 * Get timeout checking interval
 * @return Timeout checking interval
 */
Time
Aggregator::GetRetxTimer() const
{
    return m_retxTimer;
}



/**
 * Override, start this class
 */
void
Aggregator::StartApplication()
{
    //NS_LOG_FUNCTION_NOARGS();
    App::StartApplication();
    FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);


    // ToDo: enable later
    /*    // For testing purpose
    RTO_recorder = "src/ndnSIM/results/log/" + m_prefix.toUri() + "_RTO.txt";
    window_Recorder = "src/ndnSIM/results/log/" + m_prefix.toUri() + "_window.txt";
    responseTime_recorder = "src/ndnSIM/results/log/" + m_prefix.toUri() + "_RTT.txt";

    // Open and immediately close the file in write mode to clear it
    std::ofstream file1(RTO_recorder, std::ios::out);
    std::ofstream file2(window_Recorder, std::ios::out);
    std::ofstream file3(responseTime_recorder, std::ios::out);
    if (!file1.is_open()) {
        std::cerr << "Failed to open the file: " << RTO_recorder << std::endl;
        Simulator::Stop();
    }
    if (!file2.is_open()) {
        std::cerr << "Failed to open the file: " << window_Recorder << std::endl;
        Simulator::Stop();
    }
    if (!file3.is_open()) {
        std::cerr << "Failed to open the file: " << responseTime_recorder << std::endl;
        Simulator::Stop();
    }
    file1.close(); // Optional here since file will be closed automatically
    file2.close();
    file3.close();

    Simulator::Schedule(MilliSeconds(5), &Aggregator::RTO_Recorder, this);
    Simulator::Schedule(MilliSeconds(5), &Aggregator::WindowRecorder, this);*/
}



/**
 * Override, stop this class
 */
void
Aggregator::StopApplication()
{
    /// Cancel packet generation - can be a way to stop simulation gracefully?
    Simulator::Cancel(m_sendEvent);

    //NS_LOG_INFO("The average response time of Aggregator in " << round << " aggregation rounds is: " << GetResponseTimeAverage() << " ms");
    //NS_LOG_INFO("The average aggregate time is: " << GetAggregateTimeAverage() << " ms");
    App::StopApplication();
}



/**
 * Perform aggregation for incoming data packets (sum)
 * @param data
 * @param dataName
 */
void Aggregator::aggregate(const ModelData& data, const uint32_t& seq) {
    // first initialization
    if (sumParameters.find(seq) == sumParameters.end()){
        sumParameters[seq] = std::vector<float>(300, 0.0f);
        count[seq] = 0;
    }

    // Aggregate data
    std::transform(sumParameters[seq].begin(), sumParameters[seq].end(), data.parameters.begin(), sumParameters[seq].begin(), std::plus<float>());

    // Aggregate congestion signal
    congestionSignalList[seq].insert(congestionSignalList[seq].end(), data.congestedNodes.begin(), data.congestedNodes.end());

    count[seq]++;
}



/**
 * Don't get mean for now, just reformating the data packets, perform aggregation at consumer
 * @param dataName
 * @return Data content
 */
ModelData Aggregator::getMean(const uint32_t& seq){
    ModelData result;
    if (sumParameters.find(seq) != sumParameters.end() && congestionSignalList.find(seq) != congestionSignalList.end()) {
        result.parameters = sumParameters[seq];

        // Add congestionSignal of current node if necessary
        if (congestionSignal[seq]) {
            congestionSignalList[seq].push_back(m_prefix.toUri());
            NS_LOG_DEBUG("Congestion detected on current node!");
        }


        result.congestedNodes = congestionSignalList[seq];


    } else {
        NS_LOG_DEBUG("Error when get aggregation result, please exit and check!");
        ns3::Simulator::Stop();
    }

    return result;
}



/**
 * Invoked when Nack
 * @param nack
 */
void
Aggregator::OnNack(shared_ptr<const lp::Nack> nack)
{
    /// tracing inside
    App::OnNack(nack);

    NS_LOG_INFO("NACK received for: " << nack->getInterest().getName()
                                          << ", reason: " << nack->getReason());
}



/**
 * Set cwnd
 * @param window
 */
void
Aggregator::SetWindow(uint32_t window)
{
    m_initialWindow = window;
    m_window = m_initialWindow;
}



/**
 * Get cwnd
 * @return cwnd
 */
uint32_t
Aggregator::GetWindow() const
{
    return m_initialWindow;
}



/**
 * Set max sequence number, not used now
 * @param seqMax
 */
void
Aggregator::SetSeqMax(uint32_t seqMax)
{
    // Be careful, ignore maxSize here
    m_seqMax = seqMax;
}



/**
 * Get max sequence number, not used now
 * @return
 */
uint32_t
Aggregator::GetSeqMax() const
{
    return m_seqMax;
}



/**
 * Increase cwnd
 */
void
Aggregator::WindowIncrease()
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
 */
void
Aggregator::WindowDecrease(std::string type)
{
    if (!m_useCwa || m_highData > m_recPoint) {
        const double diff = m_seq - m_highData;
        BOOST_ASSERT(diff >= 0);

        m_recPoint = m_seq + (m_addRttSuppress * diff);

        if (type == "timeout") {
            m_ssthresh = m_window * m_beta;
            m_window = m_ssthresh;
        } else if (type == "LocalCongestion") {
            m_ssthresh = m_window * m_alpha;
            m_window = m_ssthresh;
        } else if (type == "RemoteCongestion") {
            m_ssthresh = m_window * m_gamma;
            m_window = m_ssthresh;
        }

        // Window size can't be reduced below initial size
        if (m_window < m_initialWindow) {
            m_window = m_initialWindow;
        }

        NS_LOG_DEBUG("Window size decreased to " << m_window);
    }
    else {
        NS_LOG_DEBUG("Window decrease suppressed, HighData: " << m_highData << ", RecPoint: " << m_recPoint);
    }
}



/**
 * Process incoming interest packets
 * @param interest
 */
void
Aggregator::OnInterest(shared_ptr<const Interest> interest)
{
    NS_LOG_INFO("Receiving interest:  " << *interest);
    //NS_LOG_INFO("The incoming interest packet size is: " << interest->wireEncode().size());
    App::OnInterest(interest);

    std::string interestType = interest->getName().get(-2).toUri();

    if (interestType == "data") {

        // Parse incoming interest, retrieve their name segments, currently use "/NextHop/Destination/Type/Seq"
        std::string dest = interest->getName().get(1).toUri();
        uint32_t seq = interest->getName().get(-1).toSequenceNumber();
        std::string originalName = interest->getName().toUri();

        std::vector<std::string> segments;  // Vector to store the segments
        std::istringstream iss(dest);
        std::string segment;

        // Store the interest segments
        std::vector<std::string> value_agg;

        // Store divided elements and push them into interest queue
        std::vector<std::tuple<uint32_t, bool, shared_ptr<Name>>> interestList;

        // split the destination segment into several ones and store them individually in a vector called "segments"
        while (std::getline(iss, segment, '.')) {
            segments.push_back(segment);
        }

        // Check whether aggregation tree is received
        if (!treeSync){
            NS_LOG_DEBUG("Error! No aggregation tree info!");
            ns3::Simulator::Stop();
        }

        // Initialize new iteration bool
        bool isNewIteration = true;

        // Signal indicating duplicate retransmission
        bool isDuplicate = false;

        // Divide interests and push them into queue
        for (const auto& [child, leaves] : aggregationMap) {
            std::string name_sec1;
            std::string name;

            // interest is divided
            for (const auto& leaf : leaves) {
                if (std::find(segments.begin(), segments.end(), leaf) != segments.end()) {
                    name_sec1 += leaf + ".";
                } else {
                    NS_LOG_INFO("Data from " << leaf << " is not required for this iteration.");
                }
            }
            name_sec1.resize(name_sec1.size() - 1);

            if (name_sec1.empty()) {
                NS_LOG_INFO("No interest needs to be sent to node " << child << " in this iteration");
                continue;
            } else {
                name = "/" + child + "/" + name_sec1 + "/data";
                shared_ptr<Name> newName = make_shared<Name>(name);
                newName->appendSequenceNumber(seq);
                std::string newNameString = newName->toUri();
                value_agg.push_back(newNameString);

                // Check whether incoming interest is a retransmission duplicate, if so, drop it directly
                auto it = std::find(m_timeoutList.begin(), m_timeoutList.end(), newNameString);
                if (it != m_timeoutList.end()) {
                    isDuplicate = true;
                }

                // Store divided interests into interest list first, push into interest queue later if they're not duplicate retransmission
                interestList.push_back(std::make_tuple(seq, isNewIteration, newName));
                isNewIteration = false;
            }
        }

        // If current packet isn't duplicate, then push divided interests into interest queue; otherwise, drop current packet
        if (!isDuplicate) {
            for (const auto& element : interestList) {
                interestQueue.push(element);
            }
        } else {
            NS_LOG_INFO("This is a replicate retransmission from upper tier, drop the entire packet!");
            return;
        }

        if (map_agg_oldSeq_newName.find(seq) == map_agg_oldSeq_newName.end() && m_agg_newDataName.find(seq) == m_agg_newDataName.end()){
            map_agg_oldSeq_newName[seq] = value_agg; // name segments
            m_agg_newDataName[seq] = originalName; // whole name
        }

        ScheduleNextPacket();
    } else if (interestType == "initialization") {
        // Synchronize signal
        treeSync = true;

        // Extract useful info and parse it into readable format
        std::vector<std::string> inputs;
        if (interest->getName().size() > 3) {
            for (size_t i = 1; i < interest->getName().size() - 2; ++i) {
                inputs.push_back(interest->getName().get(i).toUri());
            }
        }
        aggregationMap = aggTreeProcessStrings(inputs);

        // Define for new congestion control
        numChild = static_cast<int> (aggregationMap.size());

        // testing, delete later!!!!
/*        if (aggregationMap.empty())
            NS_LOG_DEBUG("aggregationMap is empty!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        else {
            // Print the result mapping
            for (const auto& [node, leaves] : aggregationMap) {
                NS_LOG_DEBUG(node << " contains leaf nodes: ");
                for (const auto& leaf : leaves) {
                    NS_LOG_DEBUG(leaf << " ");
                }
            }
        }*/

        // Initialize file name
        RTO_recorder = folderPath + "/" + m_prefix.toUri() + "_RTO.txt";
        window_Recorder = folderPath + "/" + m_prefix.toUri() + "_window.txt";
        responseTime_recorder = folderPath + "/" + m_prefix.toUri() + "_RTT.txt";
        aggregateTime_recorder = folderPath + "/" + m_prefix.toUri() + "_aggregationTime.txt";

        // Initialize logging session
        InitializeLogFile();

        // Start recording into logs
        Simulator::Schedule(MilliSeconds(5), &Aggregator::RTORecorder, this);
        Simulator::Schedule(MilliSeconds(5), &Aggregator::WindowRecorder, this);

        // Generate a new data packet to respond to tree broadcasting
        Name dataName(interest->getName());
        auto data = make_shared<Data>();
        data->setName(dataName);
        data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

        SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
        if (m_keyLocator.size() > 0) {
            signatureInfo.setKeyLocator(m_keyLocator);
        }
        data->setSignatureInfo(signatureInfo);
        ::ndn::EncodingEstimator estimator;
        ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
        encoder.appendVarNumber(m_signature);
        data->setSignatureValue(encoder.getBuffer());

        // to create real wire encoding
        data->wireEncode();
        m_transmittedDatas(data, this, m_face);
        m_appLink->onReceiveData(*data);
    }
}



/**
 * Schedule next packet sending operation by cwnd
 */
void
Aggregator::ScheduleNextPacket()
{
    if (!treeSync) {
        NS_LOG_INFO("Haven't received aggregation tree, don't send new interests for now.");
    }
    else if (m_window == static_cast<uint32_t>(0)) {
        Simulator::Remove(m_sendEvent);
        NS_LOG_DEBUG("New event in " << (std::min<double>(0.5, m_rtt->RetransmitTimeout().ToDouble(Time::S))) << " sec");

        m_sendEvent = Simulator::Schedule(Seconds(std::min<double>(0.5, (m_retxTimer * 6).GetSeconds())), &Aggregator::SendPacket, this);
    }
    else if (m_inFlight >= m_window) {
        // do nothing
        NS_LOG_INFO("m_inFlight >= m_window, do nothing.");
        NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);
    }
    else {
        // This step seems necessary if "m_window = 0" condition is triggered before
        if (m_sendEvent.IsRunning()) {
            Simulator::Remove(m_sendEvent);
        }
        NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);
        m_sendEvent = Simulator::ScheduleNow(&Aggregator::SendPacket, this);
    }
}



/**
 * Schedule to send new interests
 */
void
Aggregator::SendPacket()
{
    if (!interestQueue.empty()) {
        auto interestTuple = interestQueue.front();
        interestQueue.pop();
        uint32_t iteration = std::get<0>(interestTuple);
        bool isNewIteration = std::get<1>(interestTuple);
        shared_ptr<Name> name = std::get<2>(interestTuple);

        // New iteration, start compute aggregation time
        if (isNewIteration) {
            aggregateStartTime[iteration] = ns3::Simulator::Now();
        }
        SendInterest(name);
        ScheduleNextPacket();
    } else {
        NS_LOG_INFO("Pending new interests from upper tier...");
    }
}



/**
 * Format the interest packet and send out interests
 * @param newName
 */
void
Aggregator::SendInterest(shared_ptr<Name> newName)
{
    if (!m_active)
        return;

    std::string nameWithSeq = newName->toUri();

    // Trace timeout
    m_timeoutCheck[nameWithSeq] = ns3::Simulator::Now();

    // Start response time
    startTime[nameWithSeq] = ns3::Simulator::Now();

    NS_LOG_INFO("Sending new interest >>>> " << nameWithSeq);
    shared_ptr<Interest> newInterest = make_shared<Interest>();
    newInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    newInterest->setCanBePrefix(false);
    newInterest->setName(*newName);
    time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
    newInterest->setInterestLifetime(interestLifeTime);
    m_transmittedInterests(newInterest, this, m_face);
    m_appLink->onReceiveInterest(*newInterest);

    /// Designed for Window
    m_inFlight++;
}



/**
 * Process incoming data packets
 * @param data
 */
void
Aggregator::OnData(shared_ptr<const Data> data)
{
    if(!m_active)
        return;

    App::OnData(data);
    NS_LOG_INFO ("Received content object: " << boost::cref(*data));
    //NS_LOG_INFO("The incoming data packet size is: " << data->wireEncode().size());

    std::string dataName = data->getName().toUri();
    uint32_t seq = data->getName().at(-1).toSequenceNumber();
    ECNLocal = false;
    ECNRemote = false;

    // Stop checking timeout associated with this seq
    if (m_timeoutCheck.find(dataName) != m_timeoutCheck.end())
        m_timeoutCheck.erase(dataName);
    else
        NS_LOG_DEBUG("Data " << dataName << " doesn't exist in the map, please check!");

    // Check whether this is a retransmission packet
    auto timeoutItem = std::find(m_timeoutList.begin(),  m_timeoutList.end(), dataName);
    if (timeoutItem != m_timeoutList.end()) {
        NS_LOG_INFO("This packet is retransmission packet.");
    }

    // Perform data name matching with interest name
    ModelData modelData;

    // Initialize congestionSignal for new iteration
    if (sumParameters.find(seq) == sumParameters.end())
        congestionSignal[seq] = false;

    auto data_map = map_agg_oldSeq_newName.find(seq);
    if (data_map != map_agg_oldSeq_newName.end())
    {
        NS_LOG_INFO("Received data name: " << data->getName().toUri());

        // Response time computation (RTT)
        if (startTime.find(dataName) != startTime.end()){
            responseTime[dataName] = ns3::Simulator::Now() - startTime[dataName];
            ResponseTimeSum(responseTime[dataName].GetMilliSeconds());
            startTime.erase(dataName);
        }

        // Record RTT
        ResponseTimeRecorder(responseTime[dataName]);

        // Reset RetxTimer and timeout interval
        RTO_Timer = RTOMeasurement(responseTime[dataName].GetMilliSeconds());
        m_timeoutThreshold = RTO_Timer;
        NS_LOG_DEBUG("responseTime for name : " << dataName << " is: " << responseTime[dataName].GetMilliSeconds() << " ms");
        NS_LOG_DEBUG("RTO measurement: " << RTO_Timer.GetMilliSeconds() << " ms");

        // Set RTT_threshold to control cwnd
        if (RTT_threshold_vec.size() < numChild) {
            RTTThresholdMeasure(responseTime[dataName].GetMilliSeconds());
        } else if (RTT_threshold != 0 && responseTime[dataName].GetMilliSeconds() > RTT_threshold) {
            ECNLocal = true;
        }

        // Aggregation starts
        auto& vec = data_map->second;
        auto vecIt = std::find(vec.begin(), vec.end(), dataName);
        std::vector<uint8_t> oldbuffer(data->getContent().value(), data->getContent().value() + data->getContent().value_size());

        if (deserializeModelData(oldbuffer, modelData) && vecIt != vec.end()) {
            aggregate(modelData, seq);
            ECNRemote = !modelData.congestedNodes.empty();
            vec.erase(vecIt);
        } else{
            NS_LOG_INFO("Data name doesn't exist in map_agg_oldSeq_newName, meaning this data packet is duplicate, do nothing!");
            return;
        }


        /// AIMD begins
        if (m_highData < seq) {
            m_highData = seq;
        }

        if (data->getCongestionMark() > 0) {
            if (m_reactToCongestionMarks) {
                NS_LOG_DEBUG("Received congestion mark: " << data->getCongestionMark());
                //WindowDecrease("RTT_threshold");
            }
            else {
                NS_LOG_DEBUG("Ignored received congestion mark: " << data->getCongestionMark());
            }
        }
        else if (ECNLocal) {
            NS_LOG_DEBUG("Local congestion is detected");
            congestionSignal[seq] = true;
            WindowDecrease("LocalCongestion");
        }
/*        else if (ECNRemote) {
            NS_LOG_INFO("Remote congestion is detected.");
            WindowDecrease("RemoteCongestion");
        }*/
        else {
            NS_LOG_INFO("No congestion, increase the cwnd.");
            WindowIncrease();
        }

        if (m_inFlight > static_cast<uint32_t>(0)) {
            m_inFlight--;
        } else {
            NS_LOG_DEBUG("m_inFlight is 0, error.");
            Simulator::Stop();
        }

        NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);

        ScheduleNextPacket();
        /// AIMD ends



        // Check whether the aggregation of current iteration is done
        if (vec.empty()){
            NS_LOG_DEBUG("Aggregation finished.");
            // reset the aggregation count
            count[seq] = 0;

            // Aggregation time computation
            if (aggregateStartTime.find(seq) != aggregateStartTime.end()) {
                aggregateTime[seq] = ns3::Simulator::Now() - aggregateStartTime[seq];
                AggregateTimeSum(aggregateTime[seq].GetMilliSeconds());
                aggregateStartTime.erase(seq);
                NS_LOG_INFO("Aggregator's aggregate time of sequence " << seq << " is: " << aggregateTime[seq].GetMilliSeconds() << " ms");
            } else {
                NS_LOG_DEBUG("Error when calculating aggregation time, no reference found for seq " << seq);
            }

            // Record aggregation time
            AggregateTimeRecorder(aggregateTime[seq]);

            // Get aggregation result for current iteration
            std::vector<uint8_t> newbuffer;
            serializeModelData(getMean(seq), newbuffer);

            // create data packet
            auto data = make_shared<Data>();

            std::string name_string = m_agg_newDataName[seq];
            NS_LOG_DEBUG("New aggregated data's name: " << name_string);

            shared_ptr<Name> newName = make_shared<Name>(name_string);
            data->setName(*newName);
            data->setContent(make_shared< ::ndn::Buffer>(newbuffer.begin(), newbuffer.end()));
            data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
            SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

            if (m_keyLocator.size() > 0){
                signatureInfo.setKeyLocator(m_keyLocator);
            }
            data->setSignatureInfo(signatureInfo);
            ::ndn::EncodingEstimator estimator;
            ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
            encoder.appendVarNumber(m_signature);
            data->setSignatureValue(encoder.getBuffer());
            data->wireEncode();
            m_transmittedDatas(data, this, m_face);
            m_appLink->onReceiveData(*data);
        } else{
            NS_LOG_DEBUG("Wait for others to aggregate.");
        }
    }else{
        NS_LOG_DEBUG("Error, data name can't be recognized!");
        ns3::Simulator::Stop();
    }
}



/**
 * Record window every 5 ms, and store them in a file
 */
void
Aggregator::WindowRecorder()
{
    // Open file; on first call, truncate it to delete old content
    std::ofstream file(window_Recorder, std::ios::app);

    if (file.is_open()) {
        file << ns3::Simulator::Now().GetMilliSeconds() << " " << m_window << "\n";  // Write text followed by a newline
        file.close();          // Close the file after writing
    } else {
        std::cerr << "Unable to open file: " << window_Recorder << std::endl;
    }
    Simulator::Schedule(MilliSeconds(5), &Aggregator::WindowRecorder, this);
}



/**
 * Record RTT every 5 ms, and store them in a file
 */
void
Aggregator::RTORecorder()
{
    // Open the file using fstream in append mode
    std::ofstream file(RTO_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << RTO_recorder << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << ns3::Simulator::Now().GetMilliSeconds() << " " << RTO_Timer.GetMilliSeconds() << std::endl;

    // Close the file
    file.close();
    Simulator::Schedule(MilliSeconds(5), &Aggregator::RTORecorder, this);
}



/**
 * Record the response time for each returned packet, store them in a file
 * @param responseTime
 */
void
Aggregator::ResponseTimeRecorder(Time responseTime) {
    // Open the file using fstream in append mode
    std::ofstream file(responseTime_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << responseTime_recorder << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << ns3::Simulator::Now().GetMilliSeconds() << " " << responseTime.GetMilliSeconds() << std::endl;

    // Close the file
    file.close();
}



/**
 * Record the aggregate time when each iteration finished
 * @param aggregateTime
 */
void
Aggregator::AggregateTimeRecorder(Time aggregateTime) {
    // Open the file using fstream in append mode
    std::ofstream file(aggregateTime_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << aggregateTime_recorder << std::endl;
        return;
    }

    // Write aggregation time to file, followed by a new line
    file << Simulator::Now().GetMilliSeconds() << " " << aggregateTime.GetMilliSeconds() << std::endl;

    file.close();
}



/**
 * Initialize all new log files, called in the beginning of simulation
 */
void
Aggregator::InitializeLogFile()
{
    // Check whether the object path exists, if not, create it first
    CheckDirectoryExist(folderPath);

    // Open the file and clear all contents for all log files
    OpenFile(RTO_recorder);
    OpenFile(responseTime_recorder);
    OpenFile(aggregateTime_recorder);
    OpenFile(window_Recorder);
}


} // namespace ndn
} // namespace ns3


