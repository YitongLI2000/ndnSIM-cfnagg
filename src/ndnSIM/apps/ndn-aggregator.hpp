//
// Created by 李怿曈 on 16/4/2024.
//

#ifndef NDN_AGGREGATOR_HPP
#define NDN_AGGREGATOR_HPP

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

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/circular_buffer.hpp>

namespace ns3{
namespace ndn{

class Aggregator : public App
{
public:

    static TypeId GetTypeId();

    Aggregator();
    virtual ~Aggregator(){};


    // Core part
    virtual void OnInterest(shared_ptr<const Interest> interest);

    virtual void OnData(shared_ptr<const Data> data);

    virtual void ScheduleNextPacket();

    void SendPacket();

    void SendInterest(shared_ptr<Name> newName);

    virtual void OnTimeout(std::string nameString);

    virtual void OnNack(shared_ptr<const lp::Nack> nack);


    // Compute window measurement
    void SetWindow(uint32_t payload);

    uint32_t GetWindow() const;

    void SetSeqMax(uint32_t seqMax);

    uint32_t GetSeqMax() const;

    void WindowIncrease();

    void WindowDecrease(std::string type);

    // Data aggregation
    void aggregate(const ModelData& data, const uint32_t& seq);

    ModelData getMean(const uint32_t& seq);


    // Compute RTT/ Aggregation time
    void
    ResponseTimeSum (int64_t response_time);

    int64_t
    GetResponseTimeAverage();

    void
    AggregateTimeSum (int64_t response_time);

    int64_t
    GetAggregateTimeAverage();

    Time RTOMeasurement(int64_t resTime);

    // Measure threshold for congestion control
    void RTTThresholdMeasure(int64_t responseTime);


    // Parse the received aggregation tree
    std::pair<std::string, std::set<std::string>> aggTreeProcessSingleString(const std::string& input);

    std::map<std::string, std::set<std::string>> aggTreeProcessStrings(const std::vector<std::string>& inputs);


    // For testing purpose, measure the consumer's window
    void WindowRecorder();

    void RTORecorder();

    void ResponseTimeRecorder(Time responseTime);

    void AggregateTimeRecorder(Time aggregateTime);

    void InitializeLogFile();



protected:
    virtual void StartApplication() override;

    virtual void StopApplication() override;

    void CheckRetxTimeout();

    void SetRetxTimer(Time retxTimer);

    Time GetRetxTimer() const;



protected:
    Ptr<UniformRandomVariable> m_rand;
    Name m_prefix;
    Name m_nexthop;
    Name m_nexttype;
    Time m_interestLifeTime;


public:
    typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
    typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

protected:
    // log file
    // All logs start to write after synchronization, make sure only the chosen aggregators will generate log files; those aren't chosen will disable this function
    // ToDo: Start log function after synchronization
    std::string folderPath = "src/ndnSIM/results/logs";
    std::string RTO_recorder;
    std::string window_Recorder;
    std::string responseTime_recorder;
    std::string aggregateTime_recorder;

    // Tree broadcast synchronization
    bool treeSync;

    // Congestion control, measure RTT threshold to detect congestion
    int numChild;
    std::vector<int64_t> RTT_threshold_vec;
    int64_t RTT_threshold;

    // Congestion signal
    bool ECNLocal;
    bool ECNRemote;

    // cwnd management
    uint32_t m_initialWindow;
    TracedValue<double> m_window;;
    TracedValue<uint32_t> m_inFlight;
    bool m_setInitialWindowOnTimeout;

    // AIMD cwnd management
    double m_ssthresh;
    bool m_useCwa;
    uint32_t m_highData;
    double m_recPoint;
    double m_beta;
    double m_alpha;
    double m_gamma;
    double m_addRttSuppress;
    bool m_reactToCongestionMarks;


    uint32_t m_seq;      ///< @brief currently requested sequence number
    uint32_t m_seqMax;   ///< @brief maximum number of sequence number
    EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
    Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
    EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

    Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator

    Time m_offTime;          ///< \brief Time interval between packets
    Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
    Time m_freshness;
    uint32_t m_signature;
    Name m_keyLocator;


    // Interest queue definition (interest name)
    //std::queue<shared_ptr<Name>> interestQueue;
    std::queue<std::tuple<uint32_t, bool, shared_ptr<Name>>> interestQueue;

    // Timeout check and RTT measurement
    std::map<std::string, ns3::Time> m_timeoutCheck;
    Time m_timeoutThreshold;
    int64_t SRTT;
    int64_t RTTVAR;
    int roundRTT;
    Time RTO_Timer;
    //std::vector<std::string> m_timeoutList;
    boost::circular_buffer<std::string> m_timeoutList; // Currently initialize with size of 100

    // Aggregation list
    std::map<uint32_t, std::vector<std::string>> map_agg_oldSeq_newName; // name segments
    std::map<uint32_t, std::string> m_agg_newDataName; // whole name

    // Aggregation variables storage
    std::map<uint32_t, std::vector<float>> sumParameters; // result after performing mean average aggregation
    std::map<uint32_t, int> count; // count of aggregation
    std::map<uint32_t, std::vector<std::string>> congestionSignalList; // result after aggregating congestion signal
    std::map<uint32_t, bool> congestionSignal; // congestion signal for current node


    // Response/Aggregation time measurement
    std::map<std::string, ns3::Time> startTime;
    std::map<std::string, ns3::Time> responseTime;
    int64_t totalResponseTime;
    int round;

    std::map<uint32_t, ns3::Time> aggregateStartTime;
    std::map<uint32_t, ns3::Time> aggregateTime;
    int64_t totalAggregateTime;
    int iteration;

    // Receive aggregation tree from consumer
    std::map<std::string, std::set<std::string>> aggregationMap;




    struct RetxSeqsContainer : public std::set<uint32_t> {
    };

    RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted


    struct SeqTimeout {
        SeqTimeout(uint32_t _seq, Time _time)
            : seq(_seq)
            , time(_time)
        {
        }

        uint32_t seq;
        Time time;
        };

class i_seq {};

class i_timestamp {};


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


TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/> m_lastRetransmittedInterestDataDelay;
TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

};

} // namespace ndn
} // namespace ns3


#endif //APPS_NDN_AGGREGATOR_HPP
