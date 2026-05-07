#include "peer.hpp"
#include "udp.hpp"
#include "encryption.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>

void send_message(int socket, const std::string& message) {
    write(socket, message.c_str(), message.length());
}

std::string receive_message(int socket) {
    char buffer[1024] = {0};
    int bytes = read(socket, buffer, sizeof(buffer));
    
    if (bytes <= 0) return "";  // connection closed

    return std::string(buffer, bytes);
}

int connect_to_peer(const std::string& ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        // std::cout << "connect() failed\n";
        close(sock);

        return -1;
    }
    
    return sock;
}

Peer::Peer(int port, std::string _name) : tcp_port(port), name(_name) {}

void Peer::start() {
    std::thread(&Peer::tcp_server, this).detach();
    std::thread(&Peer::udp_listener, this).detach();
    std::thread(&Peer::discovery_broadcast, this).detach();
}

void Peer::tcp_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(tcp_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        // std::cout << "bind() failed\n";
        return;
    }

    listen(server_fd, 5);

    // std::cout << "TCP server listening on port " << tcp_port << "\n";

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);

        if (client >= 0) {
            {
                std::lock_guard<std::mutex> lock(conn_mutex);
                connections.push_back({ .socket = client, .name = "incoming", .shared_key = "" });
            }
            send_message(client, "NAME " + name + "\n");

            std::thread(&Peer::handle_client, this, client).detach();
        }
    }
}
void Peer::handle_client(int client) {
    std::string leftover;
    std::string peer_name;
    std::string encryption_key;

    while (true) {
        char buf[1024];
        int bytes = read(client, buf, sizeof(buf));
        if (bytes <= 0) break;

        leftover += std::string(buf, bytes);

        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string msg = leftover.substr(0, pos);
            leftover.erase(0, pos + 1);

            if (msg.empty()) continue;

            if (msg.rfind("NAME ", 0) == 0) {
                peer_name = msg.substr(5);
                
                std::string key1 = name;
                std::string key2 = peer_name;
                if (key1 > key2) std::swap(key1, key2);
                encryption_key = key1 + key2;
                
                std::lock_guard<std::mutex> lock(conn_mutex);
                for (auto& conn : connections)
                    if (conn.socket == client) { 
                        conn.name = peer_name;
                        conn.shared_key = encryption_key;
                        break;
                    }
            } else {
                if (!peer_name.empty()) {
                    std::string decrypted = decrypt(msg, encryption_key);
                    
                    {
                        std::lock_guard<std::mutex> lock(msg_mutex);
                        messages.push_back({ peer_name, decrypted, peer_name });
                    }
                }
            }
        }
    }

    close(client);

    {
        std::lock_guard<std::mutex> lock(conn_mutex);
        auto it = std::find_if(connections.begin(), connections.end(),
                               [&](const Connection& c){ return c.socket == client; });
        if (it != connections.end()) connections.erase(it);
    }
}

void Peer::udp_listener() {
    start_udp_listener(tcp_port, this);
}

void Peer::discover() {
    // TODO: get peer name from configuration
    broadcast_discovery(tcp_port, "bridge-peer");
}

void Peer::connect(const RemotePeer& peer) {
    int sock = connect_to_peer(peer.ip, peer.port);
    if (sock >= 0) {
        std::string key1 = name;
        std::string key2 = peer.name;
        if (key1 > key2) std::swap(key1, key2);
        std::string encryption_key = key1 + key2;
        
        {
            std::lock_guard<std::mutex> lock(conn_mutex);
            connections.push_back({
                    .socket = sock, 
                    .name = peer.name,
                    .shared_key = encryption_key
            });
        }

        send_message(sock, "NAME " + name + "\n");
        std::thread(&Peer::handle_client, this, sock).detach();
    }
}

std::vector<Connection> Peer::get_connections() {
    std::lock_guard<std::mutex> lock(conn_mutex);
    return connections;
}

void Peer::send_to(int index, const std::string& msg) {
    {
        std::lock_guard<std::mutex> lock(conn_mutex);
        if (index < 0 || index >= (int)connections.size()) return;
        
        std::string encrypted = encrypt(msg, connections[index].shared_key);

        send_message(connections[index].socket, encrypted + "\n");
        {
            std::lock_guard<std::mutex> lock(msg_mutex);
            messages.push_back({ name, msg, connections[index].name });
        }
    }
}

std::vector<Message> Peer::get_messages(){
    std::lock_guard<std::mutex> lock(msg_mutex);
    return messages;
}

void Peer::add_discovered_peer(const RemotePeer& peer) {
    std::lock_guard<std::mutex> lock(discovered_mutex);
    discovered_peers.insert(peer);
}

std::vector<RemotePeer> Peer::get_discovered_peers() {
    std::lock_guard<std::mutex> lock(discovered_mutex);
    return std::vector<RemotePeer>(discovered_peers.begin(), discovered_peers.end());
}

void Peer::discovery_broadcast() {
    while (true) {
        broadcast_discovery(tcp_port, name);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}