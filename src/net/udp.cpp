#include "udp.hpp"
#include "peer.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <thread>

static const int DISCOVERY_PORT = 9999;  // FIXED UDP PORT

void broadcast_discovery(int port, const std::string& peer_name) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        return;
    }

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Format: "BRIDGE_DISCOVER:port:peer_name"
    std::string message = "BRIDGE_DISCOVER:" + std::to_string(port) + ":" + peer_name;

    sendto(sock, message.c_str(), message.length(), 0, 
               (sockaddr*)&addr, sizeof(addr));

    close(sock);
}

void send_udp(const std::string& ip, int port, const std::string& message) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        std::cout << "UDP socket creation failed\n";
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (sendto(sock, message.c_str(), message.length(), 0,
               (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "sendto() failed\n";
    }

    close(sock);
}

void start_udp_listener(int port, Peer* peer_ptr) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        return;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    #ifdef SO_REUSEPORT
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    #endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return;
    }

    char buffer[1024];
    sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    while (true) {
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (sockaddr*)&sender_addr, &sender_len);

        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string message(buffer);
            std::string sender_ip = inet_ntoa(sender_addr.sin_addr);

            // check if discovery
            if (message.find("BRIDGE_DISCOVER:") == 0 && peer_ptr) {
                RemotePeer peer = parse_discovery_message(message, sender_ip, port);
                // lol, dont add yourself
                if (peer.port != port) {
                    peer_ptr->add_discovered_peer(peer);
                }
            }
        }
    }

    close(sock);
}

RemotePeer parse_discovery_message(const std::string& message, const std::string& sender_ip, int local_port) {
    RemotePeer peer;
    peer.ip = sender_ip;
    peer.port = local_port;  // default
    
    size_t first_colon = message.find(":");
    size_t second_colon = message.find(":", first_colon + 1);
    
    if (first_colon != std::string::npos && second_colon != std::string::npos) {
        try {
            peer.port = std::stoi(message.substr(first_colon + 1, second_colon - first_colon - 1));
            peer.name = message.substr(second_colon + 1);
        } catch (...) {
            peer.name = "unknown";
        }
    }
    
    return peer;
}
