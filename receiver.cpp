#include <iostream>
#include <thread>
#include <utility>

#include "SRTNet.h"

SRTNet mySRTNetServer;

//This is my class managed by the network connection.
class MyClass {
public:
    MyClass() {
        isKnown = false;
    };
    int test = 0;
    int counter = 0;
    std::atomic_bool isKnown;
};

//**********************************
//Server part
//**********************************

//Return a connection object. (Return nullptr if you don't want to connect to that client)
std::shared_ptr<NetworkConnection> validateConnection(struct sockaddr &sin, SRTSOCKET newSocket) {

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

    auto a1 = std::make_shared<NetworkConnection>();
    a1->object = std::make_shared<MyClass>();
    return a1;

}

//Data callback.
bool handleData(std::unique_ptr <std::vector<uint8_t>> &content, SRT_MSGCTRL &msgCtrl, std::shared_ptr<NetworkConnection> ctx, SRTSOCKET clientHandle) {

    //Try catch?
    auto v = std::any_cast<std::shared_ptr<MyClass>&>(ctx -> object);

    if (!v->isKnown) { //just a example/test. This connection is unknown. See what connection it is and set the test-parameter accordingly
        if (content->data()[0] == 1) {
            v->isKnown=true;
            v->test = 1;
        }

        if (content->data()[0] == 2) {
            v->isKnown=true;
            v->test = 2;
        }
    }

    if (v->isKnown) {
        if (v->counter++ == 100) { //every 100 packet you got respond back to the client using the same data.
            v->counter = 0;
            SRT_MSGCTRL thisMSGCTRL = srt_msgctrl_default;
            mySRTNetServer.sendData(content->data(), content->size(), &thisMSGCTRL,clientHandle);
        }
    }

    // std::cout << "Got ->" << content->size() << " " << std::endl;

    return true;
};


int main(int argc, const char * argv[]) {

    std::cout << "Server started" << std::endl;

    //Register the server callbacks
    mySRTNetServer.clientConnected=std::bind(&validateConnection, std::placeholders::_1, std::placeholders::_2);
    mySRTNetServer.receivedData=std::bind(&handleData, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    /*Start the server
     * ip: bind to this ip (can be IPv4 or IPv6)
     * port: bind to this port
     * reorder: Number of packets in the reorder window
     * latency: the max latency in milliseconds before dropping the data
     * overhead: The % overhead tolerated for retransmits relative the original data stream.
     * mtu: max 1456
     */
    if (!mySRTNetServer.startServer("0.0.0.0", 8000, 16, 1000, 100, 1456,"Th1$_is_4_0pt10N4L_P$k")) {
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
}
