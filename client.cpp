#include <iostream>
#include "kissnet/kissnet.hpp"
#include "mpegts_demuxer.h"
#include "simple_buffer.h"
#include "ts_packet.h"
#include "ElasticFrameProtocol.h"
#include "SRTNet.h"
#include "RESTInterface.hpp"

#define MTU 1456 //SRT-max
#define LISTEN_INTERFACE "127.0.0.1"
#define LISTEN_PORT 8080

#define TYPE_AUDIO 0x0f
#define TYPE_VIDEO 0x1b

SRTNet mySRTNetClient; //The SRT client
ElasticFrameProtocolSender myEFPSender(MTU); //EFP sender
RESTInterface myRESTInterface;

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

json getStats(std::string cmdString) {
    json j;
    if (cmdString == "dumpall") {

        std::string handle = std::to_string(mySRTNetClient.context);
        SRT_TRACEBSTATS currentServerStats = {0};
        if (mySRTNetClient.getStatistics(&currentServerStats, SRTNetClearStats::yes, SRTNetInstant::no)) {
            //Send all stats
            j[handle.c_str()]["pktSent"] = currentServerStats.pktSent;
            j[handle.c_str()]["pktRecv"] = currentServerStats.pktRecv;
            j[handle.c_str()]["pktSndLoss"] = currentServerStats.pktSndLoss;
            j[handle.c_str()]["pktRcvLoss"] = currentServerStats.pktRcvLoss;
            j[handle.c_str()]["pktRetrans"] = currentServerStats.pktRetrans;
            j[handle.c_str()]["pktRcvRetrans"] = currentServerStats.pktRcvRetrans;
            j[handle.c_str()]["pktSentACK"] = currentServerStats.pktSentACK;
            j[handle.c_str()]["pktRecvACK"] = currentServerStats.pktRecvACK;
            j[handle.c_str()]["pktSentNAK"] = currentServerStats.pktSentNAK;
            j[handle.c_str()]["pktRecvNAK"] = currentServerStats.pktRecvNAK;
            j[handle.c_str()]["mbpsSendRate"] = currentServerStats.mbpsSendRate;
            j[handle.c_str()]["mbpsRecvRate"] = currentServerStats.mbpsRecvRate;
            j[handle.c_str()]["pktSndDrop"] = currentServerStats.pktSndDrop;
            j[handle.c_str()]["pktRcvDrop"] = currentServerStats.pktRcvDrop;
        }
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


    myRESTInterface.getStatsCallback = std::bind(&getStats, std::placeholders::_1);
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
    if (!mySRTNetClient.startClient(targetIP, targetPort, 16, 1000, 100, client1Connection, MTU,
                                    "Th1$_is_4_0pt10N4L_P$k")) {
        std::cout << "SRT client1 failed starting." << std::endl;
        return EXIT_FAILURE;
    }


    ElasticFrameMessages efpMessage;

    SimpleBuffer in;
    MpegTsDemuxer demuxer;
    kissnet::udp_socket serverSocket(kissnet::endpoint(LISTEN_INTERFACE, LISTEN_PORT));
    serverSocket.bind();
    kissnet::buffer<4096> receiveBuffer;
    bool firstRun = true;
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
                    return EXIT_FAILURE;
                }
            }


            std::cout << "GOT: " << unsigned(frame->mStreamType);
            if (frame->mStreamType == 0x0f && frame->mPid == audioPID) {

                //  for (auto lIt = demuxer.mStreamPidMap.begin(); lIt != demuxer.mStreamPidMap.end(); lIt++) {
                //      std::cout << "Hej: " << (int)lIt->first << " " << (int)lIt->second << std::endl;
                //  }

                std::cout << " AAC Frame";
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
                    std::cout << "h264 packAndSendFromPtr error " << std::endl;
                }
            } else if (frame->mStreamType == 0x1b && frame->mPid == videoPID) {
                std::cout << " H.264 frame";
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
                    std::cout << "h264 packAndSendFromPtr error " << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }
    return EXIT_SUCCESS;
}
