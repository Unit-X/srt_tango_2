#include <iostream>
#include "ElasticFrameProtocol.h"
#include "SRTNet.h"
#include "RESTInterface.hpp"

#define PAYLOAD_SIZE 1456 //SRT-max

SRTNet mySRTNetServer; //SRT
RESTInterface myRESTInterface;

void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket);


//**********************************
//Server part
//**********************************

// This is the class and everything you want to associate with a SRT connection
// You can see this as a classic c-'void* context' on steroids since SRTNet will own it and handle
// it's lifecycle. The destructor is called when the SRT connection is terminated. Magic!

class MyClass {
public:
    MyClass() {
        myEFPReceiver = new (std::nothrow) ElasticFrameProtocolReceiver(5, 2);
    }
    virtual ~MyClass() {
        *efpActiveElement = false; //Release active marker
        delete myEFPReceiver;
    };
    uint8_t efpId = 0;
    std::atomic_bool *efpActiveElement;
    ElasticFrameProtocolReceiver *myEFPReceiver;
};

// Array of 256 possible EFP receivers, could be millions but I just decided 256 change to your needs.
// You could make it much simpler just giving a new connection a uint64_t number++
std::atomic_bool efpActiveList[UINT8_MAX] = {false};
uint8_t getEFPId() {
    for (int i = 1; i < UINT8_MAX - 1; i++) {
        if (!efpActiveList[i]) {
            efpActiveList[i] = true; //Set active
            return i;
        }
    }
    return UINT8_MAX;
}

//Global EFP stats
int32_t efpFrameCounter[UINT8_MAX] = {0};
int32_t efpBrokenCounter[UINT8_MAX] = {0};
int64_t byteCounter[UINT8_MAX] = {0};
void clearStats(uint8_t efpId) {
    efpFrameCounter[efpId] = 0;
    efpBrokenCounter[efpId] = 0;
    byteCounter[efpId] = 0;
}

// Return a connection object. (Return nullptr if you don't want to connect to that client)
std::shared_ptr<NetworkConnection> validateConnection(struct sockaddr &sin) {

    char addrIPv6[INET6_ADDRSTRLEN];

    if (sin.sa_family == AF_INET) {
        struct sockaddr_in* inConnectionV4 = (struct sockaddr_in*) &sin;
        auto *ip = (unsigned char *) &inConnectionV4->sin_addr.s_addr;
        std::cout << "Connecting IPv4: " << unsigned(ip[0]) << "." << unsigned(ip[1]) << "." << unsigned(ip[2]) << "."
                  << unsigned(ip[3]) << std::endl;

        //Do we want to accept this connection?
        //return nullptr;


    } else if (sin.sa_family == AF_INET6) {
        struct sockaddr_in6* inConnectionV6 = (struct sockaddr_in6*) &sin;
        inet_ntop(AF_INET6, &inConnectionV6->sin6_addr, addrIPv6, INET6_ADDRSTRLEN);
        printf("Connecting IPv6: %s\n", addrIPv6);

        //Do we want to accept this connection?
        //return nullptr;

    } else {
        //Not IPv4 and not IPv6. That's weird. don't connect.
        return nullptr;
    }

    //Get EFP ID.
    uint8_t efpId = getEFPId();
    if (efpId == UINT8_MAX) {
        std::cout << "Unable to accept more EFP connections " << std::endl;
        return nullptr;
    }

    // Here we can put whatever into the connection. The object we embed is maintained by SRTNet
    // In this case we put MyClass in containing the EFP ID we got from getEFPId() and a EFP-receiver
    auto a1 = std::make_shared<NetworkConnection>(); // Create a connection
    a1->object = std::make_shared<MyClass>(); // And my object containing my stuff
    auto v = std::any_cast<std::shared_ptr<MyClass> &>(a1->object); //Then get a pointer to my stuff
    v->efpId = efpId; // Populate it with the efpId
    v->efpActiveElement =
            &efpActiveList[efpId]; // And a pointer to the list so that we invalidate the id when SRT drops the connection
    v->myEFPReceiver->receiveCallback =
            std::bind(&gotData, std::placeholders::_1); //In this example we aggregate all callbacks..

            // Clear the stats for this efpID
    clearStats(efpId);

    return a1; // Now hand over the ownership to SRTNet
}

//Network data recieved callback.
bool handleData(std::unique_ptr<std::vector<uint8_t>> &content,
                SRT_MSGCTRL &msgCtrl,
                std::shared_ptr<NetworkConnection> ctx,
                SRTSOCKET clientHandle) {
    //We got data from SRTNet
    auto v = std::any_cast<std::shared_ptr<MyClass> &>(ctx->object); //Get my object I gave SRTNet
    v->myEFPReceiver->receiveFragment(*content,
                                      v->efpId); //unpack the fragment I got using the efpId created at connection time.
    return true;
}

//ElasticFrameProtocol got som data from some efpSource.. Everything you need to know is in the rPacket
//meaning EFP stream number EFP id and content type. if it's broken the PTS value
//code with additional information of payload variant and if there is embedded data to extract and so on.


void gotData(ElasticFrameProtocolReceiver::pFramePtr &rPacket) {

    efpFrameCounter[rPacket->mSource]++;
    byteCounter[rPacket->mSource] += rPacket->mFrameSize;
    if (rPacket->mBroken) {
        efpBrokenCounter[rPacket->mSource]++;
    }
}

json getStats(std::string cmdString) {
    json j;
    if (cmdString == "dumpall") {

        mySRTNetServer.getActiveClients([&](std::map<SRTSOCKET, std::shared_ptr<NetworkConnection>> &clientList)
                                        {
                                            for (const auto& client: clientList) {
                                                std::string handle = std::to_string(client.first);
                                                SRT_TRACEBSTATS currentServerStats = {0};
                                                if (mySRTNetServer.getStatistics(&currentServerStats, SRTNetClearStats::yes, SRTNetInstant::no, client.first)) {
                                                    //Send all stats
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
                                                auto v = std::any_cast<std::shared_ptr<MyClass> &>(client.second->object); //Get my object I gave SRTNet
                                                j[handle.c_str()]["efp_broken_cnt"] = efpBrokenCounter[v->efpId];
                                                j[handle.c_str()]["efp_frame_cnt"] = efpFrameCounter[v->efpId];
                                                j[handle.c_str()]["efp_byte_cnt"] = byteCounter[v->efpId];
                                            }
                                        }
        );
        return j;
    } else {
        return j;
    }
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        std::cout << "Expected 3 arguments: Listen_IP Listen_Port JSON_Port" << std::endl;
    }

    std::string listenIP = argv[1];
    int listenPort = std::stoi(argv[2]);
    int listenJsonPort = std::stoi(argv[3]);

    if (listenPort == listenJsonPort) {
        std::cout << "Listen_Port and JSON_Port can't be same port" << std::endl;
        return EXIT_FAILURE;
    }

    myRESTInterface.getStatsCallback=std::bind(&getStats, std::placeholders::_1);
    if (!myRESTInterface.startServer(listenIP.c_str(), listenJsonPort, "/restapi/version1")) {
        std::cout << "REST interface did not start." << std::endl;
        return EXIT_FAILURE;
    }

    //Setup and start the SRT server
    mySRTNetServer.clientConnected = std::bind(&validateConnection, std::placeholders::_1);
    mySRTNetServer.receivedData = std::bind(&handleData,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4);
    if (!mySRTNetServer.startServer(listenIP, listenPort, 4, PAYLOAD_SIZE, "Th1$_is_4_0pt10N4L_P$k")) {
        std::cout << "SRT Server failed to start." << std::endl;
        return EXIT_FAILURE;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        mySRTNetServer.getActiveClients([](std::map<SRTSOCKET, std::shared_ptr<NetworkConnection>> &clientList)
                                        {
                                            std::cout << "The server got " << clientList.size() << " client(s)." << std::endl;
                                        }
        );
    }

    //When you decide to quit garbage collect and stop threads....
    mySRTNetServer.stop();

    std::cout << "Done serving. Will exit." << std::endl;
    return 0;
}

