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
 **/

#ifndef NDN_CONSUMER_H
#define NDN_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"
#include "ModelData.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-value.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"
#include "ns3/ptr.h"

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <numeric>
#include <iostream>
#include <sstream>
#include <queue>
#include <utility>
#include <deque>
#include <tuple>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class Consumer : public App {
public:

    Consumer();
    virtual ~Consumer(){};

    static TypeId GetTypeId();

    void TreeBroadcast();

    void ConstructAggregationTree();

    virtual void
    OnData(shared_ptr<const Data> contentObject);

    virtual void
    OnNack(shared_ptr<const lp::Nack> nack);

    virtual void
    OnTimeout(std::string nameString);

    void
    SendPacket();

    // Generate new interests
    void
    InterestGenerator();

    // Data aggregation
    void aggregate(const ModelData& data, const uint32_t& seq);

    std::vector<float> getMean(const uint32_t& seq);

    // Calculate aggregate time and response time
    void ResponseTimeSum (int64_t response_time);

    int64_t
    GetResponseTimeAverage();

    void
    AggregateTimeSum (int64_t response_time);

    int64_t
    GetAggregateTimeAverage();


    // Measure threshold for congestion control
    void RTTThresholdMeasure(int64_t responseTime, int roundIndex);

    // Based on response time, measure RTT for each round
    Time RTOMeasurement(int64_t resTime, int roundIndex);

    // Record results in files, for testing purpose only
    void RTORecorder(std::string prefix, int roundIndex);

    // Record RTT of each packet and some other relevant info
    void ResponseTimeRecorder(int roundIndex, std::string prefix, uint32_t seq, Time responseTime, bool ECN);

    void AggregateTimeRecorder(Time aggregateTime);

    void InitializeLogFile();

    bool CanDecreaseWindow(int64_t threshold);

    void ThroughputRecorder(int interestThroughput, int dataThroughput, Time start_simulation, Time start_throughput);

    void AggTreeRecorder();

public:
    typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
    typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

protected:
    // from App
    virtual void
    StartApplication();

    virtual void
    StopApplication();

    virtual void
    ScheduleNextPacket() = 0;

    virtual void
    SendInterest(shared_ptr<Name> newName);

    void
    CheckRetxTimeout();

    void
    SetRetxTimer(Time retxTimer);

    Time
    GetRetxTimer() const;

public:
    // Override the function in App class to return leaf nodes
    std::map<std::string, std::set<std::string>>
    getLeafNodes(const std::string& key,const std::map<std::string, std::vector<std::string>>& treeMap);

    // Return round index
    int findRoundIndex(const std::string& target);



protected:
    // Topology file name
    std::string filename = "src/ndnSIM/examples/topologies/DataCenterTopology.txt";

    // Log path
    std::string folderPath = "src/ndnSIM/results/logs/con";
    std::map<std::string, std::string> RTO_recorder;
    std::map<std::string, std::string> responseTime_recorder; // Format: 'Time', 'seq', 'RTT', ‘ECN’, 'isWindowDecreaseSuppressed'
    std::map<std::string, std::string> windowRecorder; // Format: 'Time', 'cwnd', 'cwnd threshold' (when cwnd is larger than the threshold, cwnd increase will be suppressed)
    std::string aggregateTime_recorder; // Format: 'Time', 'aggTime'
    int suspiciousPacketCount; // When timeout is triggered, add one

    // Update when WindowDecrease() is called every time, used for CWA algorithm
    Time lastWindowDecreaseTime;
    bool isWindowDecreaseSuppressed;

    // Throughput measurement
    int totalInterestThroughput;
    int totalDataThroughput;
    Time startSimulation;
    Time stopSimulation;
    Time startThroughputMeasurement;
    bool throughputStable;

    // Congestion/rate control
    std::vector<std::vector<std::string>> globalTreeRound; // First dimension: round. Second dimension: next-tier leaves
    std::map<int, std::vector<int64_t>> RTT_threshold_vec; // Each mapping represents one round (if there're more than one round)
    std::map<int, int64_t> RTT_threshold; // Actual threshold used to detect congestion
    std::map<int, int64_t> RTT_measurement; // The estimated RTT value using EWMA
    std::map<int, int> RTT_count; // How many RTT packets this node has received, used to estimate how many iterations have passed
    int linkCount;

    // Global sequence number
    uint32_t globalSeq;
    int globalRound;

    // Interest queue
    std::queue<std::tuple<uint32_t, bool, shared_ptr<Name>>> interestQueue; // tuple: iteration, round, name

    // Get producer list, which are used to generate new interests
    std::string proList;

    // Constructed aggregation Tree
    std::vector<std::map<std::string, std::vector<std::string>>> aggregationTree; // Entire tree (including main tree and sub-trees)

    // Broadcast aggregation tree
    bool broadcastSync;
    std::set<std::string> broadcastList; // Elements within the set need to be broadcasted, all elements are unique

    // Aggregation synchronization
    std::map<uint32_t, std::vector<std::vector<std::string>>> map_agg_oldSeq_newName; // Manage names for round
    std::map<uint32_t, std::vector<std::string>> m_agg_newDataName; // Manage names for entire iteration

    // Used inside InterestGenerator function
    std::map<int, std::vector<std::string>> map_round_nameSec1_3;
    std::vector<std::string> vec_iteration;
    std::vector<std::vector<std::string>> vec_round;


    // Timeout check/ RTO measurement
    std::map<std::string, ns3::Time> m_timeoutCheck;
    std::map<int, Time> m_timeoutThreshold;
    std::map<int, Time> RTO_Timer;
    std::map<int, int64_t> SRTT;
    std::map<int, int64_t> RTTVAR;
    std::map<int, bool> initRTO;
    std::map<int, int> numTimeout; // Count the number of timeout, for testing purpose


    // Designed for actual aggregation operations
    std::map<uint32_t, std::vector<float>> sumParameters;
    std::map<uint32_t, std::vector<float>> aggregationResult;
    int producerCount;

    // Congestion signal
    bool ECNLocal;
    bool ECNRemote;

    // defined for response time
    std::map<std::string, ns3::Time> startTime;
    std::map<std::string, ns3::Time> responseTime;
    int64_t total_response_time;
    int round;

    // defined for aggregation time
    std::map<uint32_t, ns3::Time> aggregateStartTime;
    std::map<uint32_t, ns3::Time> aggregateTime;
    int64_t totalAggregateTime;
    int iterationCount;

    // New defined attribute variables
    std::string m_interestName; // Consumer's interest prefix
    std::string m_nodeprefix; // Consumer's node prefix
    uint32_t m_iteNum; // The number of iterations
    int m_interestQueue; // Queue size
    int m_constraint; // Constraint of each sub-tree
    double m_EWMAFactor; // Factor used in EWMA, recommended value is between 0.1 and 0.3
    double m_thresholdFactor; // Factor to compute "RTT_threshold", i.e. "RTT_threshold = Threshold_factor * RTT_measurement"


    Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator
    uint32_t m_seq;      ///< @brief currently requested sequence number
    uint32_t m_seqMax;   ///< @brief maximum number of sequence number
    EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
    Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
    EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

    Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator

    Time m_offTime;          ///< \brief Time interval between packets
    Time m_interestLifeTime; ///< \brief LifeTime for interest packet



    /// @cond include_hidden
      /**
    * \struct This struct contains sequence numbers of packets to be retransmitted
    */
    struct RetxSeqsContainer : public std::set<uint32_t> {
    };

    RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
  };
  /// @endcond

  /// @cond include_hidden
  class i_seq {
  };
  class i_timestamp {
  };
  /// @endcond

  /// @cond include_hidden
  /**
   * \struct This struct contains a multi-index for the set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

  /// @endcond
};

} // namespace ndn
} // namespace ns3

#endif
