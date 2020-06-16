#include <iostream>
#include "ElasticFrameProtocol.h"
#include "SRTNet.h"

#define MTU 1456 //SRT-max
#define WORKER_VIDEO_FRAMES 3000

SRTNet mySRTNetClient; //The SRT client
ElasticFrameProtocolSender myEFPSender(MTU); //EFP sender

//This will act as our encoder and just provide us with a H264 AnnexB frame when we want one.
std::vector<uint8_t> getNALUnit(int i) {
    std::string fileName = "../media/" + std::to_string(i) + ".h264";
    FILE *f = fopen(fileName.c_str(), "rb");
    if (!f) {
        std::cout << "Failed opening file" << std::endl;
        std::vector<uint8_t> empty;
        return empty;
    }
    fseek(f, 0, SEEK_END);
    size_t fSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *pictureBuffer = (uint8_t *)malloc(fSize);
    if (!pictureBuffer) {
        std::cout << "Failed reserving memory" << std::endl;
        std::vector<uint8_t> empty;
        return empty;
    }
    size_t fResult = fread(pictureBuffer, 1, fSize, f);
    if (fResult != fSize) {
        std::cout << "Failed reading data" << std::endl;
        std::vector<uint8_t> empty;
        return empty;
    }
    std::vector<uint8_t> my_vector(&pictureBuffer[0], &pictureBuffer[fSize]);
    free(pictureBuffer);
    fclose(f);
    return my_vector;
}

//Just for debug
int packSize;

void sendData(const std::vector<uint8_t> &subPacket) {

    //for debugging actual bytes sent (payload bytes)
    if (subPacket[0] == 1 || subPacket[0] == 3) {
        packSize += subPacket.size() - myEFPSender.geType1Size();
    }

    //for debugging actual bytes sent (payload bytes)
    if (subPacket[0] == 2) {
        packSize += subPacket.size() - myEFPSender.geType2Size();
        std::cout << "Sent-> " << packSize << std::endl;
        packSize = 0;
    }

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

int main() {
    packSize = 0;

    //Set-up framing protocol
    myEFPSender.sendCallback = std::bind(&sendData, std::placeholders::_1);

    //Set-up SRT
    auto client1Connection = std::make_shared<NetworkConnection>();
    mySRTNetClient.receivedData = std::bind(&handleDataClient,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4);
    if (!mySRTNetClient.startClient("127.0.0.1", 8000, 16, 1000, 100, client1Connection, MTU)) {
        std::cout << "SRT client1 failed starting." << std::endl;
        return EXIT_FAILURE;
    }

    //Generate some data to send
    uint64_t pts = 0;
    uint64_t dts = 0;
    for (int i = 0; i < WORKER_VIDEO_FRAMES; ++i) {
        std::vector<uint8_t> thisNalData = getNALUnit(i + 1);
        std::cout << "SendNAL > " << thisNalData.size() << " pts " << pts << std::endl;
        //We got no DTS in this example set to PTS if no DTS is available (In this version of EFP DTS MUST be set to PTS if not available)
        dts = pts;
        myEFPSender.packAndSend(thisNalData,
                                ElasticFrameContent::h264,
                                pts,
                                dts,
                                EFP_CODE('A', 'N', 'X', 'B'),
                                1,
                                NO_FLAGS);
        //you could also send other data from other threads for example audio when the audio encoder spits out something.
        //myEFPSender.packAndSend(thisADTSData,ElasticFrameContent::adts,pts,'ADTS',1,NO_FLAGS);
        pts += 90000 / 60; //fake a pts of 60Hz. FYI.. the codestream is 23.98 (I and P only)
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); //sleep for 16ms ~60Hz
    }
    mySRTNetClient.stop();
    std::cout << "Done sending will exit" << std::endl;
    return 0;
}
