#pragma once
#include <string>

struct RemotePeer {
    std::string ip;
    int port;
    std::string name;
};

class Peer {
public:
    Peer(int tcp_port);
    void start();
    void discover(); // UDP broadcast (implement later)

    void connect(const RemotePeer& peer);

private:
    int tcp_port;

    void tcp_server();
    void udp_listener();
};