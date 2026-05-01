#include "udp.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>

void broadcast_discovery(int port, const std::string& peer_name) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        std::cout << "UDP socket creation failed\n";
        return;
    }

    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        std::cout << "setsockopt() failed\n";
        close(sock);
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // Format: "BRIDGE_DISCOVER:peer_name"
    std::string message = "BRIDGE_DISCOVER:" + peer_name;

    if (sendto(sock, message.c_str(), message.length(), 0, 
               (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "sendto() failed\n";
    } else {
        std::cout << "Discovery broadcast sent: " << message << "\n";
    }

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

void start_udp_listener(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sock < 0) {
        std::cout << "UDP socket creation failed\n";
        return;
    }

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cout << "setsockopt() failed\n";
        close(sock);
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "bind() failed\n";
        close(sock);
        return;
    }

    std::cout << "UDP listener started on port " << port << "\n";

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

            std::cout << "UDP received from " << sender_ip << ": " << message << "\n";

            // Check if it's a discovery message
            if (message.find("BRIDGE_DISCOVER:") == 0) {
                RemotePeer peer = parse_discovery_message(message, sender_ip);
                std::cout << "Discovered peer: " << peer.name << " at " << peer.ip 
                         << ":" << peer.port << "\n";
            }
        }
    }

    close(sock);
}

RemotePeer parse_discovery_message(const std::string& message, const std::string& sender_ip) {
    RemotePeer peer;
    peer.ip = sender_ip;
    peer.port = 0; // Will be filled from TCP connection
    
    size_t pos = message.find(":");
    if (pos != std::string::npos) {
        peer.name = message.substr(pos + 1);
    }
    
    return peer;
}
