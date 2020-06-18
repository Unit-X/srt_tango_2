//
// Created by Anders Cedronius on 2020-06-17.
//

#ifndef SRT_TEST_STATSHANDLER_H
#define SRT_TEST_STATSHANDLER_H

#include <map>
#include <thread>
#include "SRTNet.h"


class StatsHandler {
public:

    explicit StatsHandler(uint32_t timeout){
        mTimeout = timeout;
    }

    bool tick(){
        return --mTimeout;
    }

    void setTimeout(uint32_t timeout){
        mTimeout = timeout;
    }

    void generateStatsEFPDiff (int32_t efpFrameCnt, int32_t efpBrokenCnt, int64_t byteCnt) {
        efpFrameCounterDelta = efpFrameCnt - efpFrameCounter;
        efpBrokenCounterDelta  = efpBrokenCnt - efpBrokenCounter;
        byteCounterDelta  = byteCnt - byteCounter;

        efpFrameCounter = efpFrameCnt;
        efpBrokenCounter  = efpBrokenCnt;
        byteCounter  = byteCnt;
    }

    void generateStatsDiff (SRT_TRACEBSTATS &stats) {
        msTimeStampDelta = stats.msTimeStamp - msTimeStamp;
        pktFlowWindowDelta = stats.pktFlowWindow - pktFlowWindow;
        pktCongestionWindowDelta = stats.pktCongestionWindow - pktCongestionWindow;
        pktFlightSizeDelta = stats.pktFlightSize - pktFlightSize;
        msRTTDelta = stats.msRTT - msRTT;
        mbpsBandwidthDelta = stats.mbpsBandwidth - mbpsBandwidth;
        mbpsMaxBWDelta = stats.mbpsMaxBW - mbpsMaxBW;
        pktSentDelta = stats.pktSent - pktSent;
        pktSndLossDelta = stats.pktSndLoss - pktSndLoss;
        pktSndDropDelta = stats.pktSndDrop - pktSndDrop;
        pktRetransDelta = stats.pktRetrans - pktRetrans;
        byteSentDelta = stats.byteSent - byteSent;
        byteAvailSndBufDelta = stats.byteAvailSndBuf - byteAvailSndBuf;
        byteSndDropDelta = stats.byteSndDrop - byteSndDrop;
        mbpsSendRateDelta = stats.mbpsSendRate - mbpsSendRate;
        usPktSndPeriodDelta = stats.usPktSndPeriod - usPktSndPeriod;
        msSndBufDelta = stats.msSndBuf - msSndBuf;
        pktRecvDelta = stats.pktRecv - pktRecv;
        pktRcvLossDelta = stats.pktRcvLoss - pktRcvLoss;
        pktRcvDropDelta = stats.pktRcvDrop - pktRcvDrop;
        pktRcvRetransDelta = stats.pktRcvRetrans - pktRcvRetrans;
        pktRcvBelatedDelta = stats.pktRcvBelated - pktRcvBelated;
        byteRecvDelta = stats.byteRecv - byteRecv;
        byteAvailRcvBufDelta = stats.byteAvailRcvBuf - byteAvailRcvBuf;
        byteRcvLossDelta = stats.byteRcvLoss - byteRcvLoss;
        byteRcvDropDelta = stats.byteRcvDrop - byteRcvDrop;
        mbpsRecvRateDelta = stats.mbpsRecvRate - mbpsRecvRate;
        msRcvBufDelta = stats.msRcvBuf - msRcvBuf;
        msRcvTsbPdDelayDelta = stats.msRcvTsbPdDelay - msRcvTsbPdDelay;
        pktReorderToleranceDelta = stats.pktReorderTolerance - pktReorderTolerance;

        msTimeStamp = stats.msTimeStamp;
        pktFlowWindow = stats.pktFlowWindow;
        pktCongestionWindow = stats.pktCongestionWindow;
        pktFlightSize = stats.pktFlightSize;
        msRTT = stats.msRTT;
        mbpsBandwidth = stats.mbpsBandwidth;
        mbpsMaxBW = stats.mbpsMaxBW;
        pktSent = stats.pktSent;
        pktSndLoss = stats.pktSndLoss;
        pktSndDrop = stats.pktSndDrop;
        pktRetrans = stats.pktRetrans;
        byteSent = stats.byteSent;
        byteAvailSndBuf = stats.byteAvailSndBuf;
        byteSndDrop = stats.byteSndDrop;
        mbpsSendRate = stats.mbpsSendRate;
        usPktSndPeriod = stats.usPktSndPeriod;
        msSndBuf = stats.msSndBuf;
        pktRecv = stats.pktRecv;
        pktRcvLoss = stats.pktRcvLoss;
        pktRcvDrop = stats.pktRcvDrop;
        pktRcvRetrans = stats.pktRcvRetrans;
        pktRcvBelated = stats.pktRcvBelated;
        byteRecv = stats.byteRecv;
        byteAvailRcvBuf = stats.byteAvailRcvBuf;
        byteRcvLoss = stats.byteRcvLoss;
        byteRcvDrop = stats.byteRcvDrop;
        mbpsRecvRate = stats.mbpsRecvRate;
        msRcvBuf = stats.msRcvBuf;
        msRcvTsbPdDelay = stats.msRcvTsbPdDelay;
        pktReorderTolerance = stats.pktReorderTolerance;
    }

    //Instant values
    int64_t  msTimeStamp = 0;                // time since the UDT entity is started, in milliseconds
    int      pktFlowWindow = 0;              // flow window size, in number of packets
    int      pktCongestionWindow = 0;        // congestion window size, in number of packets
    int      pktFlightSize = 0;              // number of packets on flight
    double   msRTT = 0;                      // RTT, in milliseconds
    double   mbpsBandwidth = 0;              // estimated bandwidth, in Mb/s
    double   mbpsMaxBW = 0;                  // Transmit Bandwidth ceiling (Mbps)
    int64_t  pktSent = 0;                    // number of sent data packets, including retransmissions
    int      pktSndLoss = 0;                 // number of lost packets (sender side)
    int      pktSndDrop = 0;                 // number of too-late-to-send dropped packets
    int      pktRetrans = 0;                 // number of retransmitted packets
    uint64_t byteSent = 0;                   // number of sent data bytes, including retransmissions
    int      byteAvailSndBuf = 0;            // available UDT sender buffer size
    uint64_t byteSndDrop = 0;                // number of too-late-to-send dropped bytes
    double   mbpsSendRate = 0;               // sending rate in Mb/s
    double   usPktSndPeriod = 0;             // packet sending period, in microseconds
    int      msSndBuf = 0;                   // UnACKed timespan (msec) of UDT sender
    int64_t  pktRecv = 0;                    // number of received packets
    int      pktRcvLoss = 0;                 // number of lost packets (receiver side)
    int      pktRcvDrop = 0;                 // number of too-late-to play missing packets
    int      pktRcvRetrans = 0;              // number of retransmitted packets received
    int64_t  pktRcvBelated = 0;              // number of received AND IGNORED packets due to having come too late
    uint64_t byteRecv = 0;                   // number of received bytes
    int      byteAvailRcvBuf = 0;            // available UDT receiver buffer size
    uint64_t byteRcvLoss = 0;                // number of retransmitted bytes
    uint64_t byteRcvDrop = 0;                // number of too-late-to play missing bytes (estimate based on average packet size)
    double   mbpsRecvRate = 0;               // receiving rate in Mb/s
    int      msRcvBuf = 0;                   // Undelivered timespan (msec) of UDT receiver
    int      msRcvTsbPdDelay = 0;            // Timestamp-based Packet Delivery Delay
    int      pktReorderTolerance = 0;        // packet reorder tolerance value

    //Delta values
    int64_t  msTimeStampDelta = 0;                // time since the UDT entity is started, in milliseconds
    int      pktFlowWindowDelta = 0;              // flow window size, in number of packets
    int      pktCongestionWindowDelta = 0;        // congestion window size, in number of packets
    int      pktFlightSizeDelta = 0;              // number of packets on flight
    double   msRTTDelta = 0;                      // RTT, in milliseconds
    double   mbpsBandwidthDelta = 0;              // estimated bandwidth, in Mb/s
    double   mbpsMaxBWDelta = 0;                  // Transmit Bandwidth ceiling (Mbps)
    int64_t  pktSentDelta = 0;                    // number of sent data packets, including retransmissions
    int      pktSndLossDelta = 0;                 // number of lost packets (sender side)
    int      pktSndDropDelta = 0;                 // number of too-late-to-send dropped packets
    int      pktRetransDelta = 0;                 // number of retransmitted packets
    uint64_t byteSentDelta = 0;                   // number of sent data bytes, including retransmissions
    int      byteAvailSndBufDelta = 0;            // available UDT sender buffer size
    uint64_t byteSndDropDelta = 0;                // number of too-late-to-send dropped bytes
    double   mbpsSendRateDelta = 0;               // sending rate in Mb/s
    double   usPktSndPeriodDelta = 0;             // packet sending period, in microseconds
    int      msSndBufDelta = 0;                   // UnACKed timespan (msec) of UDT sender
    int64_t  pktRecvDelta = 0;                    // number of received packets
    int      pktRcvLossDelta = 0;                 // number of lost packets (receiver side)
    int      pktRcvDropDelta = 0;                 // number of too-late-to play missing packets
    int      pktRcvRetransDelta = 0;              // number of retransmitted packets received
    int64_t  pktRcvBelatedDelta = 0;              // number of received AND IGNORED packets due to having come too late
    uint64_t byteRecvDelta = 0;                   // number of received bytes
    int      byteAvailRcvBufDelta = 0;            // available UDT receiver buffer size
    uint64_t byteRcvLossDelta = 0;                // number of retransmitted bytes
    uint64_t byteRcvDropDelta = 0;                // number of too-late-to play missing bytes (estimate based on average packet size)
    double   mbpsRecvRateDelta = 0;               // receiving rate in Mb/s
    int      msRcvBufDelta = 0;                   // Undelivered timespan (msec) of UDT receiver
    int      msRcvTsbPdDelayDelta = 0;            // Timestamp-based Packet Delivery Delay
    int      pktReorderToleranceDelta = 0;        // packet reorder tolerance value


    //EFP Part
    int32_t efpFrameCounter = 0;
    int32_t efpBrokenCounter = 0;
    int64_t byteCounter = 0;

    int32_t efpFrameCounterDelta = 0;
    int32_t efpBrokenCounterDelta  = 0;
    int64_t byteCounterDelta  = 0;

private:
    uint32_t mTimeout = 0;
    bool firstRun = true;
};

        std::map <std::string,std::unique_ptr<StatsHandler>>statsMap;
        std::mutex statsMapMtx;

#endif //SRT_TEST_STATSHANDLER_H
