/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
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
 * Main structure is modified by Yitong, new functions are added for CFNAgg
 * This class implements all functions for CFNAgg, except for cwnd management/congestion control mechanism.
 * Those parts are implemented in "ndn-consumer-INA"
 **/

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>

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

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include "ModelData.hpp"

#include "src/ndnSIM/apps/algorithm/include/AggregationTree.hpp"
#include "src/ndnSIM/apps/algorithm/utility/utility.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.Consumer");

using namespace std::chrono;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Consumer);



/**
 * Initiate attributes for consumer class, some of them may be used, some are optional
 * Note that currently only use "NodePrefix", ignore "Prefix" for now
 *
 * @return Total TypeId
 */
TypeId
Consumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::Consumer")
      .SetGroupName("Ndn")
      .SetParent<App>()

      .AddAttribute("StartSeq",
                    "Initial sequence number",
                    IntegerValue(0),
                    MakeIntegerAccessor(&Consumer::m_seq),
                    MakeIntegerChecker<int32_t>())
      .AddAttribute("Prefix",
                    "Name of the Interest",
                    StringValue(),
                    MakeStringAccessor(&Consumer::m_interestName),
                    MakeStringChecker())
      .AddAttribute("NodePrefix",
                    "Node prefix",
                    StringValue(),
                    MakeStringAccessor(&Consumer::m_nodeprefix),
                    MakeStringChecker())
      .AddAttribute("LifeTime",
                    "LifeTime for interest packet",
                    StringValue("4s"),
                    MakeTimeAccessor(&Consumer::m_interestLifeTime),
                    MakeTimeChecker())
      .AddAttribute("EWMAFactor",
                    "EWMA factor used when measuring RTT, recommended between 0.1 and 0.3",
                    DoubleValue(0.3),
                    MakeDoubleAccessor(&Consumer::m_EWMAFactor),
                    MakeDoubleChecker<double>())
      .AddAttribute("ThresholdFactor",
                    "Factor to compute actual RTT threshold",
                    DoubleValue(1.2),
                    MakeDoubleAccessor(&Consumer::m_thresholdFactor),
                    MakeDoubleChecker<double>())
      .AddAttribute("Iteration",
                    "The number of iterations to run in the simulation",
                    IntegerValue(200),
                    MakeIntegerAccessor(&Consumer::m_iteNum),
                    MakeIntegerChecker<int32_t>())
      .AddAttribute("InterestQueue",
                    "The size of interest queue",
                    IntegerValue(300),
                    MakeIntegerAccessor(&Consumer::m_queueSize),
                    MakeIntegerChecker<int64_t>())
      .AddAttribute("Constraint",
                    "Constraint of aggregation tree construction",
                    IntegerValue(5),
                    MakeIntegerAccessor(&Consumer::m_constraint),
                    MakeIntegerChecker<int>())
      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
                    MakeTimeChecker())
      .AddTraceSource("LastRetransmittedInterestDataDelay",
                    "Delay between last retransmitted Interest and received Data",
                    MakeTraceSourceAccessor(&Consumer::m_lastRetransmittedInterestDataDelay),
                    "ns3::ndn::Consumer::LastRetransmittedInterestDataDelayCallback")
      .AddTraceSource("FirstInterestDataDelay",
                     "Delay between first transmitted Interest and received Data",
                     MakeTraceSourceAccessor(&Consumer::m_firstInterestDataDelay),
                     "ns3::ndn::Consumer::FirstInterestDataDelayCallback");

  return tid;
}


/**
 * Constructor
 */
Consumer::Consumer()
    : suspiciousPacketCount(0)
    , globalSeq(0)
    , globalRound(0)
    , broadcastSync(false)
    , m_rand(CreateObject<UniformRandomVariable>())
    , m_seq(0)
    , total_response_time(0)
    , round(0)
    , totalAggregateTime(0)
    , iterationCount(0)
{
    m_rtt = CreateObject<RttMeanDeviation>();
}



/**
 * Broadcast the tree construction info to all aggregators
 */
void
Consumer::TreeBroadcast()
{
     const auto& broadcastTree = aggregationTree[0];

     for (const auto& [parentNode, childList] : broadcastTree) {
         // Don't broadcast to itself
         if (parentNode == m_nodeprefix) {
             continue;
         }

         std::string nameWithType;
         std::string nameType = "initialization";
         nameWithType += "/" + parentNode;
         auto result = getLeafNodes(parentNode, broadcastTree);

         // Construct nameWithType variable for tree broadcast
         for (const auto& [childNode, leaves] : result) {
             std::string name_indication;
             name_indication += childNode + ".";
             for (const auto& leaf : leaves) {
                 name_indication += leaf + ".";
             }
             name_indication.resize(name_indication.size() - 1);
             nameWithType += "/" + name_indication;
         }
         nameWithType += "/" + nameType;

         std::cout << "Node " << parentNode << "'s name is: " << nameWithType << std::endl;
         shared_ptr<Name> newName = make_shared<Name>(nameWithType);
         newName->appendSequenceNumber(globalSeq);
         SendInterest(newName);
     }
     globalSeq++;
}



/**
 * Implement the algorithm to compute aggregation tree
 */
void
Consumer::ConstructAggregationTree()
{
    App::ConstructAggregationTree();
    AggregationTree tree(filename);
    std::vector<std::string> dataPointNames = Utility::getProducers(filename);
    std::map<std::string, std::vector<std::string>> rawAggregationTree;
    std::vector<std::vector<std::string>> rawSubTree;


    if (tree.aggregationTreeConstruction(dataPointNames, m_constraint)) {
        rawAggregationTree = tree.aggregationAllocation;
        rawSubTree = tree.noCHTree;
    } else {
        NS_LOG_DEBUG("Fail to construct aggregation tree!");
        ns3::Simulator::Stop();
    }

    // Get the number of producers
    producerCount = Utility::countProducers(filename);

    // Create producer list
    for (const auto& item : dataPointNames) {
        proList += item + ".";
    }
    proList.resize(proList.size() - 1);


/*     std::cout << "\nAggregation tree: " << std::endl;
     for (const auto& pair : rawAggregationTree) {
         std::cout << pair.first << ": ";
         for (const auto& item : pair.second) {
             std::cout << item << " ";
         }
         std::cout << std::endl;
     }

     std::cout << "\nSub Tree without CH: " << std::endl;
     for (const auto& pair : rawSubTree) {
         for (const auto& item : pair) {
             std::cout << item << " ";
         }
         std::cout << std::endl;
     }*/

    // Create complete "aggregationTree" from raw ones
    aggregationTree.push_back(rawAggregationTree);
    while (!rawSubTree.empty()) {
        const auto& item = rawSubTree[0];
        rawAggregationTree[m_nodeprefix] = item;
        aggregationTree.push_back(rawAggregationTree);
        rawSubTree.erase(rawSubTree.begin());
    }


    int i = 0;
    std::cout << "\nIterate all aggregation tree (including main tree and sub-trees)." << std::endl;
    for (const auto& map : aggregationTree) {
        for (const auto& pair : map) {
            std::cout << pair.first << ": ";
            for (const auto& value : pair.second) {
                std::cout << value << " ";
            }
            std::cout << std::endl;

            // Initialize "broadcastList" for tree broadcasting synchronization
            if (pair.first != m_nodeprefix) {
                broadcastList.push_back(pair.first);
            }

            // Initialize "globalTreeRound" for all rounds (if there're multiple sub-trees)
            if (pair.first == m_nodeprefix) {
                std::vector<std::string> leavesRound;
                std::cout << "Round " << i << " has the following leaf nodes: ";
                for (const auto& leaves : pair.second) {
                    leavesRound.push_back(leaves);
                    std::cout << leaves << " ";
                }
                globalTreeRound.push_back(leavesRound);
                std::cout << std::endl;
            }
        }
        std::cout << "----" << std::endl;  // Separator between maps
        i++;
    }

    // Initialize variables for RTO computation/congestion control
    for (int i = 0; i < globalTreeRound.size(); i++) {
        initRTO[i] = false;
        RTO_Timer[i] = 6 * m_retxTimer;
        m_timeoutThreshold[i] = 6 * m_retxTimer;
        RTT_threshold[i] = 0;
        RTT_count[i] = 0;
    }
 }



/**
 * Originally defined in ndn::App class, override here. Start the running process of consumer class
 */
void
Consumer::StartApplication() // Called at time specified by Start
{
    // Initialize all log files
    //InitializeLogFile();

    Simulator::Schedule(MilliSeconds(5), &Consumer::RTORecorder, this);

    App::StartApplication();

    // Construct the tree
    ConstructAggregationTree();

    // Broadcast the tree
    TreeBroadcast();

    // Start interest generator
    InterestGenerator();

    ScheduleNextPacket();
}



/**
 * Originally defined in ndn::App class, override here. Stop the running process of consumer class
 */
void
Consumer::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION_NOARGS();
    // cancel periodic packet generation
    Simulator::Cancel(m_sendEvent);
    App::StopApplication();
}



/**
 * Get child nodes for given map and parent node. This function is originally defined in ndn::App class
 * @param key
 * @param treeMap
 * @return A map consisting all child nodes of the parent node
 */
std::map<std::string, std::set<std::string>>
Consumer::getLeafNodes(const std::string& key, const std::map<std::string, std::vector<std::string>>& treeMap)
{
    return App::getLeafNodes(key, treeMap);
}



/**
 * Return round index
 * @param target
 * @return Round index
 */
int
Consumer::findRoundIndex(const std::string& target)
{
    return App::findRoundIndex(globalTreeRound, target);
}



/**
 * Perform aggregation (plus all model parameters together) operation, perform average when this iteration is done
 * @param data
 * @param seq
 */
void
Consumer::aggregate(const ModelData& data, const uint32_t& seq)
{
    // first initialization
    if (sumParameters.find(seq) == sumParameters.end()){
        sumParameters[seq] = std::vector<float>(300, 0.0f);
    }

    // Aggregate data
    std::transform(sumParameters[seq].begin(), sumParameters[seq].end(), data.parameters.begin(), sumParameters[seq].begin(), std::plus<float>());
}



/**
 * Get mean average of model parameters for one iteration
 * @param seq
 * @return Mean average of model parameters
 */
std::vector<float>
Consumer::getMean(const uint32_t& seq)
{
    std::vector<float> result;
    if (sumParameters[seq].empty() || producerCount == 0) {
        NS_LOG_DEBUG("Error when calculating average model, please check!");
        return result;
    }

    for (auto value : sumParameters[seq]) {
        result.push_back(value / static_cast<float>(producerCount));
    }

    return result;
}



/**
 * Compute the total response time for average computation later
 * @param response_time
 */
void
Consumer::ResponseTimeSum (int64_t response_time)
{
    total_response_time += response_time;
    ++round;
}



/**
 * Compute average response time
 * @return Average response time
 */
int64_t
Consumer::GetResponseTimeAverage() {
    if (round == 0){
        NS_LOG_DEBUG("Error happened when calculating response time!");
        return 0;
    }
    return total_response_time / round;
}



/**
 * Compute total aggregation time for all iterations
 * @param aggregate_time
 */
void
Consumer::AggregateTimeSum (int64_t aggregate_time)
{
    totalAggregateTime += aggregate_time;
    //NS_LOG_DEBUG("totalAggregateTime is: " << totalAggregateTime);
    ++iterationCount;
}



/**
 * Return average aggregation time
 * @return Average aggregation time in certain iterations
 */
int64_t
Consumer::GetAggregateTimeAverage()
{
    if (iterationCount == 0)
    {
        NS_LOG_INFO("Error happened when calculating aggregate time!");
        Simulator::Stop();
        return 0;
    }

    return totalAggregateTime / iterationCount;
}



/**
 * Invoked when Nack is triggered
 * @param nack
 */
void
Consumer::OnNack(shared_ptr<const lp::Nack> nack)
{
    /// tracing inside
    App::OnNack(nack);
    NS_LOG_INFO("NACK received for: " << nack->getInterest().getName() << ", reason: " << nack->getReason());
}



/**
 * Triggered when timeout is triggered, timeout is traced using unique interest/data name
 * @param nameString
 */
void
Consumer::OnTimeout(std::string nameString)
{
    shared_ptr<Name> name = make_shared<Name>(nameString);
    SendInterest(name);

    // Add one to "suspiciousPacketCount"
    suspiciousPacketCount++;
}



/**
 * Set initial interval on how long to check timeout
 * @param retxTimer
 */
void
Consumer::SetRetxTimer(Time retxTimer)
{
    m_retxTimer = retxTimer;
    if (m_retxEvent.IsRunning()) {
        Simulator::Remove(m_retxEvent); // slower, but better for memory
    }

    // Schedule timeout check event
    NS_LOG_DEBUG("Next interval to check timeout is: " << m_retxTimer.GetMilliSeconds() << " ms");
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}



/**
 * Get timer for timeout check
 * @return Timer set for timeout checking
 */
Time
Consumer::GetRetxTimer() const
{
  return m_retxTimer;
}



/**
 * When invoked, check whether timeout happened for all packets in timeout list
 */
void
Consumer::CheckRetxTimeout()
{
    Time now = Simulator::Now();

    //NS_LOG_INFO("Check timeout after: " << m_retxTimer.GetMilliSeconds() << " ms");
    //NS_LOG_INFO("Current timeout threshold is: " << m_timeoutThreshold.GetMilliSeconds() << " ms");

    for (auto it = m_timeoutCheck.begin(); it != m_timeoutCheck.end();){
        // Parse the string and extract the first segment, e.g. "agg0", then find out its round
        std::string type = make_shared<Name>(it->first)->get(-2).toUri();

        // For two types of data, check timeout respectively
        if (type == "initialization") {
            if (now - it->second > (3 * m_retxTimer)) {
                std::string name = it->first;
                it = m_timeoutCheck.erase(it);
                OnTimeout(name);
            } else {
                ++it;
            }
        } else if (type == "data") {
            std::string name = it->first;
            std::string seg0 = make_shared<Name>(name)->get(0).toUri();
            int roundIndex = findRoundIndex(seg0);

            if (now - it->second > m_timeoutThreshold[roundIndex]) {
                std::string name = it->first;
                it = m_timeoutCheck.erase(it);
                numTimeout[roundIndex]++;
                OnTimeout(name);
            } else {
                ++it;
            }
        }
    }
    m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}



/**
 * Compute new RTO based on response time of recent packets
 * @param resTime
 * @param roundIndex data packet's round index
 * @return New RTO
 */
Time
Consumer::RTOMeasurement(int64_t resTime, int roundIndex)
{
    if (!initRTO[roundIndex]) {
        RTTVAR[roundIndex] = resTime / 2;
        SRTT[roundIndex] = resTime;
        NS_LOG_DEBUG("Initialize RTO for round: " << roundIndex);
        initRTO[roundIndex] = true;
    } else {
        RTTVAR[roundIndex] = 0.75 * RTTVAR[roundIndex] + 0.25 * std::abs(SRTT[roundIndex] - resTime); // RTTVAR = (1 - b) * RTTVAR + b * |SRTT - RTTsample|, where b = 0.25
        SRTT[roundIndex] = 0.875 * SRTT[roundIndex] + 0.125 * resTime; // SRTT = (1 - a) * SRTT + a * RTTsample, where a = 0.125
    }
    int64_t RTO = SRTT[roundIndex] + 4 * RTTVAR[roundIndex]; // RTO = SRTT + K * RTTVAR, where K = 4

    return MilliSeconds(2 * RTO);
}



/**
 * When ScheduleNextPacket() is invoked, this function is called. Pop elements from the interest queue, and prepare for actual interest sending
 */
void
Consumer::SendPacket()
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
        NS_LOG_INFO("All available interests have been sent, no further operation required.");
    }
}



/**
 * Generate interests for all iterations and push them into queue for further interest sending scheduling
 * First invoke when start simulation, every time when one iteration finished aggregation, update interestQueue again
 */
void
Consumer::InterestGenerator()
{
    // Generate name from section 1 to 3 (except for seq)
    if (map_round_nameSec1_3.empty()) {
        std::vector<std::string> objectProducer;
        std::string token;
        std::istringstream tokenStream(proList);
        char delimiter = '.';
        while (std::getline(tokenStream, token, delimiter)) {
            objectProducer.push_back(token);
        }

        int i = 0;
        for (const auto& aggTree : aggregationTree) {
            auto initialAllocation = getLeafNodes(m_nodeprefix, aggTree);
            std::vector<std::string> roundChild;
            std::vector<std::string> vec_sec1_3;

            for (const auto& [child, leaves] : initialAllocation) {
                std::string name_sec1_3;
                std::string name_sec1;
                roundChild.push_back(child);

                for (const auto& leaf : leaves) {
                    name_sec1 += leaf + ".";
                }
                name_sec1.resize(name_sec1.size() - 1);
                name_sec1_3 = "/" + child + "/" + name_sec1 + "/data";
                vec_sec1_3.push_back(name_sec1_3);

                vec_iteration.push_back(child); // Iteration's vector
            }
            vec_round.push_back(roundChild); // Round's vector
            map_round_nameSec1_3[i] = vec_sec1_3; // Name (section1 - section3), to be pushed into queue later
            i++;
        }
    }

/*  // For testing
    std::cout << "map_round_nameSec1_3: " << std::endl;
    for (const auto& pair : map_round_nameSec1_3) {
        std::cout << "Key: " << pair.first << std::endl;
        for (const auto& str : pair.second) {
            std::cout << "  Value: " << str << std::endl;
        }
    }

    std::cout << "vec_iteration" << std::endl;
    for (const auto& vec : vec_iteration) {
        std::cout << vec << std::endl;
    }

    std::cout << "vec_round" << std::endl;
    for (const auto& innerVec : vec_round) {
        for (const auto& vec : innerVec) {
            std::cout << vec << " ";
        }
        std::cout << std::endl;
    }*/

    // Generate entire interest name for all iterations
    while (interestQueue.size() <= m_queueSize) {
        // Generate new interests for upcoming iteration if necessary
        if (globalSeq <= m_iteNum) {
            map_agg_oldSeq_newName[globalSeq] = vec_round;
            m_agg_newDataName[globalSeq] = vec_iteration;

            bool isNewIteration = true;

            for (const auto& map : map_round_nameSec1_3) {
                for (const auto& name1_3 : map.second) {
                    shared_ptr<Name> name = make_shared<Name>(name1_3);
                    name->appendSequenceNumber(globalSeq);
                    interestQueue.push(std::make_tuple(globalSeq, isNewIteration, name));

                    isNewIteration = false;
                }
            }

            globalSeq++;
        } else {
            NS_LOG_INFO("All iterations' interests have been generated, no need for further operation.");
            break;
        }
    }
}



/**
 * Called in SendPacket() function, construct interest packet and send it actually
 * @param newName
 */
void Consumer::SendInterest(shared_ptr<Name> newName)
{
    if (!m_active)
        return;

    std::string nameWithSeq = newName->toUri();

    // Trace timeout
    m_timeoutCheck[nameWithSeq] = ns3::Simulator::Now();

    // Start response time
    if (startTime.find(nameWithSeq) == startTime.end())
        startTime[nameWithSeq] = ns3::Simulator::Now();

    shared_ptr<Interest> interest = make_shared<Interest>();
    interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    interest->setName(*newName);
    interest->setCanBePrefix(false);
    time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
    interest->setInterestLifetime(interestLifeTime);
    NS_LOG_INFO("Sending interest >>>>" << nameWithSeq);
    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);

}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////
/**
 * Process returned data packets
 * @param data
 */
void
Consumer::OnData(shared_ptr<const Data> data)
{
    if (!m_active)
        return;

    App::OnData(data); // tracing inside
    std::string type = data->getName().get(-2).toUri();
    uint32_t seq = data->getName().at(-1).toSequenceNumber();
    std::string dataName = data->getName().toUri();
    ECNRemote = false;
    ECNLocal = false;
    NS_LOG_INFO ("Received content object: " << boost::cref(*data));
    //NS_LOG_INFO("The incoming data packet size is: " << data->wireEncode().size());


    // Erase timeout
    if (m_timeoutCheck.find(dataName) != m_timeoutCheck.end())
        m_timeoutCheck.erase(dataName);
    else
        NS_LOG_DEBUG("Suspicious data packet, not exists in timeout list.");

    if (type == "data") {
        std::string name_sec0 = data->getName().get(0).toUri();

        // Perform data name matching with interest name
        ModelData modelData;
        auto data_map = map_agg_oldSeq_newName.find(seq);
        auto data_agg = m_agg_newDataName.find(seq);
        if (data_map != map_agg_oldSeq_newName.end() && data_agg != m_agg_newDataName.end()) {

            // Response time computation (RTT)
            if (startTime.find(dataName) != startTime.end()){
                responseTime[dataName] = ns3::Simulator::Now() - startTime[dataName];
                ResponseTimeSum(responseTime[dataName].GetMilliSeconds());
                startTime.erase(dataName);
                NS_LOG_INFO("Consumer's response time of sequence " << dataName << " is: " << responseTime[dataName].GetMilliSeconds() << " ms");
            }

            // Search round index
            const auto& test_data_map = data_map->second;
            int roundIndex = findRoundIndex(name_sec0);
            if (roundIndex == -1) {
                NS_LOG_DEBUG("Error on roundIndex!");
                ns3::Simulator::Stop();
            }
            NS_LOG_INFO("This packet comes from round " << roundIndex);


            // Setup RTT_threshold based on RTT of the first 5 iterations, then update RTT_threshold after each new iteration based on EWMA
            // ToDo: What about setting up a initial cwnd to run several iterations, other than start cwnd from 1
            RTTThresholdMeasure(responseTime[dataName].GetMilliSeconds(), roundIndex);

            // RTT_threshold measurement initialization is done after 3 iterations, before that, don't perform cwnd control
            if (RTT_count[roundIndex] >= globalTreeRound[roundIndex].size() * 3 && responseTime[dataName].GetMilliSeconds() > RTT_threshold[roundIndex]) {
                ECNLocal = true;
            }

            // Record response time
            ResponseTimeRecorder(responseTime[dataName], seq, ECNLocal, RTT_measurement[roundIndex], RTT_threshold[roundIndex]);

            // Reset RetxTimer and timeout interval
            RTO_Timer[roundIndex] = RTOMeasurement(responseTime[dataName].GetMilliSeconds(), roundIndex);
            m_timeoutThreshold[roundIndex] = RTO_Timer[roundIndex];
            NS_LOG_DEBUG("responseTime for name : " << dataName << " is: " << responseTime[dataName].GetMilliSeconds() << " ms");
            NS_LOG_DEBUG("Current RTO measurement: " << RTO_Timer[roundIndex].GetMilliSeconds() << " ms");


            // This data exist in the map, perform aggregation
            auto& aggVec = data_agg->second;
            auto aggVecIt = std::find(aggVec.begin(), aggVec.end(), name_sec0);

            std::vector<uint8_t> oldbuffer(data->getContent().value(), data->getContent().value() + data->getContent().value_size());

            if (deserializeModelData(oldbuffer, modelData) && aggVecIt != aggVec.end()) {
                aggregate(modelData, seq); // Aggregate data payload
                ECNRemote = !modelData.congestedNodes.empty();
                aggVec.erase(aggVecIt);
            } else{
                NS_LOG_INFO("Data name doesn't exist in map_agg_oldSeq_newName, meaning this data packet is duplicate, do nothing!");
                return;
            }

            // Judge whether the aggregation iteration has finished
            if (aggVec.empty()) {
                NS_LOG_DEBUG("Aggregation of iteration " << seq << " finished!");

                // Get aggregation result and store them
                aggregationResult[seq] = getMean(seq);

                // Update new elements for interestQueue if necessary
                if (interestQueue.size() < m_queueSize && globalSeq <= m_iteNum) {
                    InterestGenerator();
                }

                // Calculate aggregation time
                if (aggregateStartTime.find(seq) != aggregateStartTime.end()) {
                    aggregateTime[seq] = ns3::Simulator::Now() - aggregateStartTime[seq];
                    AggregateTimeSum(aggregateTime[seq].GetMilliSeconds());
                    NS_LOG_DEBUG("Iteration " << std::to_string(seq) << " aggregation time is: " << aggregateTime[seq].GetMilliSeconds() << " ms");
                    aggregateStartTime.erase(seq);
                } else {
                    NS_LOG_DEBUG("Error when calculating aggregation time, no reference found for seq " << seq);
                }

                // Record aggregation time
                AggregateTimeRecorder(aggregateTime[seq]);

            }

            /// Stop simulation
            if (iterationCount == m_iteNum) {
                NS_LOG_DEBUG("Reach " << m_iteNum << " iterations, stop!");
                ns3::Simulator::Stop();
                NS_LOG_INFO("Timeout is triggered " << suspiciousPacketCount << " times.");
                NS_LOG_INFO("The average aggregation time of Consumer in " << iterationCount << " iteration is: " << GetAggregateTimeAverage() << " ms");
                return;
            }
        } else {
            NS_LOG_DEBUG("Suspicious data packet, not exist in data map.");
            ns3::Simulator::Stop();
        }


    } else if (type == "initialization") {
        std::string destNode = data->getName().get(0).toUri();

        // Update synchronization info
        auto it = std::find(broadcastList.begin(), broadcastList.end(), destNode);
        if (it != broadcastList.end()) {
            broadcastList.erase(it);
            NS_LOG_DEBUG("Node " << destNode << " has received aggregationTree map, erase it from broadcastList");
        }

        // Tree broadcasting synchronization is done
        if (broadcastList.empty()) {
            broadcastSync = true;
            NS_LOG_DEBUG("Synchronization of tree broadcasting finished!");
        }
    }
}



/**
 * Based on RTT of the first iteration, compute their RTT average as threshold, use the threshold for congestion control
 * Apply Exponentially Weighted Moving Average (EWMA) for RTT Threshold Computation
 * @param responseTime
 * @param index round index
 */
void
Consumer::RTTThresholdMeasure(int64_t responseTime, int index)
{
    int aggregationSize = globalTreeRound[index].size();
    if (RTT_count[index] == 0) {
        RTT_measurement[index] = responseTime;
    } else {
        RTT_measurement[index] = m_EWMAFactor * responseTime + (1 - m_EWMAFactor) * RTT_measurement[index];
        RTT_threshold[index] = m_thresholdFactor * RTT_measurement[index];
    }

    if (RTT_count[index] >= aggregationSize * 3) // Whether it's larger than 3 iterations
    {
        NS_LOG_INFO("Apply RTT_threshold, current value is: " << RTT_threshold[index]);
    }
    RTT_count[index]++;
}



/**
 * Record the RTT every 5 ms, store them in a file
 */
void
Consumer::RTORecorder()
{
    // Open the file using fstream in append mode
    std::ofstream file(RTO_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << RTO_recorder << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << ns3::Simulator::Now().GetMilliSeconds();
    for (const auto& timer : RTO_Timer) {
        file << " " << timer.second.GetMilliSeconds();
    }
    file << std::endl;

    // Close the file
    file.close();
    Simulator::Schedule(MilliSeconds(5), &Consumer::RTORecorder, this);
}



/**
 * Record the response time for each returned packet, store them in a file
 * @param responseTime
 * @param seq sequence number
 * @param ECN Whether ECN exist in current packet, type is boolean
 */
void
Consumer::ResponseTimeRecorder(Time responseTime, uint32_t seq, bool ECN, int64_t threshold_measure, int64_t threshold_actual) {
    // Open the file using fstream in append mode
    std::ofstream file(responseTime_recorder, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << responseTime_recorder << std::endl;
        return;
    }

    // Write the response_time to the file, followed by a newline
    file << ns3::Simulator::Now().GetMilliSeconds() << " " << seq << " " << ECN << " " << threshold_measure << " " << threshold_actual << " " << responseTime.GetMilliSeconds() << std::endl;

    // Close the file
    file.close();
}



/**
 * Record the aggregate time when each iteration finished
 * @param aggregateTime
 */
void
Consumer::AggregateTimeRecorder(Time aggregateTime) {
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
Consumer::InitializeLogFile()
{
    // Check whether the object path exists, if not, create it first
    //CheckDirectoryExist(folderPath);

    // Open the file and clear all contents for all log files
    OpenFile(RTO_recorder);
    OpenFile(responseTime_recorder);
    OpenFile(aggregateTime_recorder);
}

} // namespace ndn
} // namespace ns3
