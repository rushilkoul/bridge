#include "peer.hpp"
#include "tcp.hpp"
#include "udp.hpp"
#include <thread>
#include <iostream>
#include <unistd.h> 

Peer::Peer(int port) : tcp_port(port) {}

void Peer::start() {
    std::thread(&Peer::tcp_server, this).detach();
    std::thread(&Peer::udp_listener, this).detach();
}

void Peer::tcp_server() {
    start_tcp_server(tcp_port);
}

void Peer::udp_listener() {
    start_udp_listener(tcp_port);
}

void Peer::discover() {
    // TODO: get peer name from configuration
    broadcast_discovery(tcp_port, "bridge-peer");
}

void Peer::connect(const RemotePeer& peer) {
    int sock = connect_to_peer(peer.ip, peer.port);
    if (sock >= 0) {
        // send_message(sock, "hello from " + peer.name);
        std::cout << "Connected successfully to " + peer.ip + ":" + std::to_string(peer.port);
        close(sock);
    }
}