#include <iostream>
#include "kissnet/kissnet.hpp"
#include "mpegts_demuxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
#include "ElasticFrameProtocol.h"
#include "SRTNet.h"
#include "RESTInterface.hpp"
#include "statshandler.h"

#define PAYLOAD_SIZE 1456 //SRT-max
#define LISTEN_INTERFACE "127.0.0.1"
#define LISTEN_PORT 8080

#define TIME_OUT_STATUS_HANDLER 86400 //Keep a client for a day

#define TYPE_AUDIO 0x0f
#define TYPE_VIDEO 0x1b

SRTNet mySRTNetClient; //The SRT client
ElasticFrameProtocolSender myEFPSender(PAYLOAD_SIZE); //EFP sender
RESTInterface myRESTInterface;

int64_t timestampCreated = {0};
bool threadRuns = {true};

//Here is where you need to add logic for how you want to map TS to EFP
//In this simple example we map one h264 video and the first AAC audio we find.
uint16_t videoPID = 0;
uint16_t audioPID = 0;
uint8_t efpStreamIDVideo = 0;
uint8_t efpStreamIDAudio = 0;

bool mapTStoEFP(PMTHeader &rPMTdata) {
    bool foundVideo = false;
    bool foundAudio = false;
    for (auto &rStream: rPMTdata.mInfos) {
        if (rStream->mStreamType == TYPE_VIDEO && !foundVideo) {
            foundVideo = true;
            videoPID = rStream->mElementaryPid;
            efpStreamIDVideo = 10;
        }
        if (rStream->mStreamType == TYPE_AUDIO && !foundAudio) {
            foundAudio = true;
            audioPID = rStream->mElementaryPid;
            efpStreamIDAudio = 20;
        }
    }
    if (!foundVideo || !foundAudio) return false;
    return true;
}

void sendData(const std::vector<uint8_t> &subPacket) {

    SRT_MSGCTRL thisMSGCTRL1 = srt_msgctrl_default;
    bool result = mySRTNetClient.sendData((uint8_t *) subPacket.data(), subPacket.size(), &thisMSGCTRL1);
    if (!result) {
        std::cout << "Failed sending. Deal with that." << std::endl;
        //mySRTNetClient.stop(); ?? then reconnect?? try again for x times?? Notify the user?? Use a alternative socket??
    }
}

void handleDataClient(std::unique_ptr<std::vector<uint8_t>> &content,
                      SRT_MSGCTRL &msgCtrl,
                      std::shared_ptr<NetworkConnection> ctx,
                      SRTSOCKET serverHandle) {
    std::cout << "Got data from server" << std::endl;
}
void statsGarbageHandler() {
    std::vector<std::string> removeMapKeys;
    while (threadRuns) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        statsMapMtx.lock();
        removeMapKeys.clear();
        for (auto &statsWorker: statsMap) {
            if (!statsWorker.second->tick()) {
                removeMapKeys.push_back(statsWorker.first);
            }
        }
        for (auto &key: removeMapKeys) {
            std::cout << "garbage collect stats worker id: " << key << std::endl;
            statsMap.erase(key);
        }
        statsMapMtx.unlock();
    }
};

json getStats(std::string cmdString, std::string statsID, bool diff) {
    json j;
    if (cmdString == "dumpall") {
        std::lock_guard<std::mutex> lock(statsMapMtx);
        if ( statsMap.find(statsID) == statsMap.end() ) {
            statsMap[statsID] = std::make_unique<StatsHandler>(TIME_OUT_STATUS_HANDLER);
        } else {
            statsMap[statsID]->setTimeout(TIME_OUT_STATUS_HANDLER);
        }

        std::string handle = std::to_string(mySRTNetClient.context);
        SRT_TRACEBSTATS currentServerStats = {0};
        if (mySRTNetClient.getStatistics(&currentServerStats, SRTNetClearStats::no, SRTNetInstant::yes)) {
            std::cout << "Stats asked by id: " << statsID << std::endl;
            if (diff) {
                statsMap[statsID]->generateStatsDiff(currentServerStats);
                j[handle.c_str()]["msTimeStamp"] = statsMap[statsID]->msTimeStampDelta;
                j[handle.c_str()]["pktFlowWindow"] = statsMap[statsID]->pktFlowWindowDelta;
                j[handle.c_str()]["pktCongestionWindow"] = statsMap[statsID]->pktCongestionWindowDelta;
                j[handle.c_str()]["pktFlightSize"] = statsMap[statsID]->pktFlightSizeDelta;
                j[handle.c_str()]["msRTT"] = statsMap[statsID]->msRTTDelta;
                j[handle.c_str()]["mbpsBandwidth"] = statsMap[statsID]->mbpsBandwidthDelta;
                j[handle.c_str()]["mbpsMaxBW"] = statsMap[statsID]->mbpsMaxBWDelta;
                j[handle.c_str()]["pktSent"] = statsMap[statsID]->pktSentDelta;
                j[handle.c_str()]["pktSndLoss"] = statsMap[statsID]->pktSndLossDelta;
                j[handle.c_str()]["pktSndDrop"] = statsMap[statsID]->pktSndDropDelta;
                j[handle.c_str()]["pktRetrans"] = statsMap[statsID]->pktRetransDelta;
                j[handle.c_str()]["byteSent"] = statsMap[statsID]->byteSentDelta;
                j[handle.c_str()]["byteAvailSndBuf"] = statsMap[statsID]->byteAvailSndBufDelta;
                j[handle.c_str()]["byteSndDrop"] = statsMap[statsID]->byteSndDropDelta;
                j[handle.c_str()]["mbpsSendRate"] = statsMap[statsID]->mbpsSendRateDelta;
                j[handle.c_str()]["usPktSndPeriod"] = statsMap[statsID]->usPktSndPeriodDelta;
                j[handle.c_str()]["msSndBuf"] = statsMap[statsID]->msSndBufDelta;
                j[handle.c_str()]["pktRecv"] = statsMap[statsID]->pktRecvDelta;
                j[handle.c_str()]["pktRcvLoss"] = statsMap[statsID]->pktRcvLossDelta;
                j[handle.c_str()]["pktRcvDrop"] = statsMap[statsID]->pktRcvDropDelta;
                j[handle.c_str()]["pktRcvRetrans"] = statsMap[statsID]->pktRcvRetransDelta;
                j[handle.c_str()]["pktRcvBelated"] = statsMap[statsID]->pktRcvBelatedDelta;
                j[handle.c_str()]["byteRecv"] = statsMap[statsID]->byteRecvDelta;
                j[handle.c_str()]["byteAvailRcvBuf"] = statsMap[statsID]->byteAvailRcvBufDelta;
                j[handle.c_str()]["byteRcvLoss"] = statsMap[statsID]->byteRcvLossDelta;
                j[handle.c_str()]["byteRcvDrop"] = statsMap[statsID]->byteRcvDropDelta;
                j[handle.c_str()]["mbpsRecvRate"] = statsMap[statsID]->mbpsRecvRateDelta;
                j[handle.c_str()]["msRcvBuf"] = statsMap[statsID]->msRcvBufDelta;
                j[handle.c_str()]["msRcvTsbPdDelay"] = statsMap[statsID]->msRcvTsbPdDelayDelta;
                j[handle.c_str()]["pktReorderTolerance"] = statsMap[statsID]->pktReorderToleranceDelta;
            } else {
                j[handle.c_str()]["msTimeStamp"] = currentServerStats.msTimeStamp;
                j[handle.c_str()]["pktFlowWindow"] = currentServerStats.pktFlowWindow;
                j[handle.c_str()]["pktCongestionWindow"] = currentServerStats.pktCongestionWindow;
                j[handle.c_str()]["pktFlightSize"] = currentServerStats.pktFlightSize;
                j[handle.c_str()]["msRTT"] = currentServerStats.msRTT;
                j[handle.c_str()]["mbpsBandwidth"] = currentServerStats.mbpsBandwidth;
                j[handle.c_str()]["mbpsMaxBW"] = currentServerStats.mbpsMaxBW;
                j[handle.c_str()]["pktSent"] = currentServerStats.pktSent;
                j[handle.c_str()]["pktSndLoss"] = currentServerStats.pktSndLoss;
                j[handle.c_str()]["pktSndDrop"] = currentServerStats.pktSndDrop;
                j[handle.c_str()]["pktRetrans"] = currentServerStats.pktRetrans;
                j[handle.c_str()]["byteSent"] = currentServerStats.byteSent;
                j[handle.c_str()]["byteAvailSndBuf"] = currentServerStats.byteAvailSndBuf;
                j[handle.c_str()]["byteSndDrop"] = currentServerStats.byteSndDrop;
                j[handle.c_str()]["mbpsSendRate"] = currentServerStats.mbpsSendRate;
                j[handle.c_str()]["usPktSndPeriod"] = currentServerStats.usPktSndPeriod;
                j[handle.c_str()]["msSndBuf"] = currentServerStats.msSndBuf;
                j[handle.c_str()]["pktRecv"] = currentServerStats.pktRecv;
                j[handle.c_str()]["pktRcvLoss"] = currentServerStats.pktRcvLoss;
                j[handle.c_str()]["pktRcvDrop"] = currentServerStats.pktRcvDrop;
                j[handle.c_str()]["pktRcvRetrans"] = currentServerStats.pktRcvRetrans;
                j[handle.c_str()]["pktRcvBelated"] = currentServerStats.pktRcvBelated;
                j[handle.c_str()]["byteRecv"] = currentServerStats.byteRecv;
                j[handle.c_str()]["byteAvailRcvBuf"] = currentServerStats.byteAvailRcvBuf;
                j[handle.c_str()]["byteRcvLoss"] = currentServerStats.byteRcvLoss;
                j[handle.c_str()]["byteRcvDrop"] = currentServerStats.byteRcvDrop;
                j[handle.c_str()]["mbpsRecvRate"] = currentServerStats.mbpsRecvRate;
                j[handle.c_str()]["msRcvBuf"] = currentServerStats.msRcvBuf;
                j[handle.c_str()]["msRcvTsbPdDelay"] = currentServerStats.msRcvTsbPdDelay;
                j[handle.c_str()]["pktReorderTolerance"] = currentServerStats.pktReorderTolerance;
            }
        }
        j[handle.c_str()]["duration_seconds_cnt"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count() - timestampCreated;
        return j;
    } else {
        return j;
    }
}

int main(int argc, char *argv[]) {
    std::cout << "Client listens for incoming MPEG-TS at: " << LISTEN_INTERFACE << ":" << unsigned(LISTEN_PORT)
              << std::endl;

    if (argc != 5) {
        std::cout << "Expected 4 arguments: Target_IP Target_Port JSON_IP JSON_Port" << std::endl;
    }

    std::string targetIP = argv[1];
    int targetPort = std::stoi(argv[2]);
    std::string listenJsonIP = argv[3];
    int listenJsonPort = std::stoi(argv[4]);


    myRESTInterface.getStatsCallback = std::bind(&getStats, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    if (!myRESTInterface.startServer(listenJsonIP.c_str(), listenJsonPort, "/restapi/version1")) {
        std::cout << "REST interface did not start." << std::endl;
        return EXIT_FAILURE;
    }


    myEFPSender.sendCallback = std::bind(&sendData, std::placeholders::_1);

    //Set-up SRT
    auto client1Connection = std::make_shared<NetworkConnection>();
    mySRTNetClient.receivedData = std::bind(&handleDataClient,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4);
    if (!mySRTNetClient.startClient(targetIP, targetPort, 4, 1000, 100, client1Connection, PAYLOAD_SIZE,
                                    "Th1$_is_4_0pt10N4L_P$k")) {
        std::cout << "SRT client1 failed starting." << std::endl;
        return EXIT_FAILURE;
    }

    timestampCreated = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

    ElasticFrameMessages efpMessage;

    SimpleBuffer in;
    MpegTsDemuxer demuxer;
    kissnet::udp_socket serverSocket(kissnet::endpoint(LISTEN_INTERFACE, LISTEN_PORT));
    serverSocket.bind();
    kissnet::buffer<4096> receiveBuffer;
    bool firstRun = true;

    //Start stats garbage collector
    std::thread(std::bind(&statsGarbageHandler)).detach();

    while (true) {
        auto[received_bytes, status] = serverSocket.recv(receiveBuffer);
        if (!received_bytes || status != kissnet::socket_status::valid) {
            break;
        }
        in.append((const char *) receiveBuffer.data(), received_bytes);
        TsFrame *frame = nullptr;
        demuxer.decode(&in, frame);
        if (frame) {

            if (firstRun && demuxer.mPmtIsValid) {
                if (!mapTStoEFP(demuxer.mPmtHeader)) {
                    std::cout << "Unable to find a video and a audio in this TS stream" << std::endl;
                    threadRuns = false;
                    return EXIT_FAILURE;
                }
            }


            //std::cout << "GOT: " << unsigned(frame->mStreamType);
            if (frame->mStreamType == 0x0f && frame->mPid == audioPID) {

                //  for (auto lIt = demuxer.mStreamPidMap.begin(); lIt != demuxer.mStreamPidMap.end(); lIt++) {
                //      std::cout << "nfo: " << (int)lIt->first << " " << (int)lIt->second << std::endl;
                //  }

                //std::cout << " AAC Frame";
                efpMessage = myEFPSender.packAndSendFromPtr((const uint8_t *) frame->mData->data(),
                                                            frame->mData->size(),
                                                            ElasticFrameContent::adts,
                                                            frame->mPts,
                                                            frame->mPts,
                                                            EFP_CODE('A', 'D', 'T', 'S'),
                                                            efpStreamIDAudio,
                                                            NO_FLAGS
                );

                if (efpMessage != ElasticFrameMessages::noError) {
                    std::cout << "h264 packAndSendFromPtr error " << (uint8_t)efpMessage << std::endl;
                }
            } else if (frame->mStreamType == 0x1b && frame->mPid == videoPID) {
                //std::cout << " H.264 frame";
                efpMessage = myEFPSender.packAndSendFromPtr((const uint8_t *) frame->mData->data(),
                                                            frame->mData->size(),
                                                            ElasticFrameContent::h264,
                                                            frame->mPts,
                                                            frame->mDts,
                                                            EFP_CODE('A', 'N', 'X', 'B'),
                                                            efpStreamIDVideo,
                                                            NO_FLAGS
                );
                if (efpMessage != ElasticFrameMessages::noError) {
                    std::cout << "h264 packAndSendFromPtr error " << (uint8_t)efpMessage <<std::endl;
                }
            }
            //std::cout << std::endl;
        }
    }
    threadRuns = false;
    return EXIT_SUCCESS;
}
