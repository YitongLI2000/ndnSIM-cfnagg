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
 **/

#ifndef NDN_CONSUMER_INA_H
#define NDN_CONSUMER_INA_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-consumer.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include <limits>

namespace ns3 {
namespace ndn {


/**
 * @ingroup ndn-apps
 * \brief NDN consumer application with more advanced congestion control options
 *
 * This app uses the algorithms from "A Practical Congestion Control Scheme for Named
 * Data Networking" (https://dl.acm.org/citation.cfm?id=2984369).
 *
 * It implements slow start, conservative window adaptation (RFC 6675),
 * and 3 different TCP algorithms: AIMD, BIC, and CUBIC (RFC 8312).
 */
class ConsumerINA : public Consumer {
public:
    static TypeId
    GetTypeId();

    ConsumerINA();

    virtual void
    StartApplication();

    virtual void
    OnData(shared_ptr<const Data> data) override;

    virtual void
    OnTimeout(std::string nameString) override;

    virtual void
    SendInterest(shared_ptr<Name> newName);

    virtual void
    ScheduleNextPacket();

private:
    void
    WindowIncrease();

    void
    WindowDecrease(std::string type);

    virtual void
    SetWindow(uint32_t window);

    uint32_t
    GetWindow() const;

    // For testing purpose, measure the consumer's window
    void WindowRecorder();

    void ResponseTimeRecorder(bool flag);

    void InitializeLogFile();

public:
    typedef std::function<void(double)> WindowTraceCallback;

private:
    // Window design
    uint32_t m_initialWindow;
    TracedValue<double> m_window;
    TracedValue<uint32_t> m_inFlight;
    bool m_setInitialWindowOnTimeout;

    // AIMD design
    double m_ssthresh;
    bool m_useCwa;
    uint32_t m_highData;
    double m_recPoint;
    double m_alpha; // Timeout decrease factor
    double m_beta; // Local congestion decrease factor
    double m_gamma; // Remote congestion decrease factor
    double m_addRttSuppress;
    bool m_reactToCongestionMarks;

    // For testing purpose, consumer window monitor
    std::string windowTimeRecorder = folderPath + "/consumer_window.txt";
    EventId windowMonitor;


};

} // namespace ndn
} // namespace ns3

#endif // NDN_CONSUMER_INA_H
