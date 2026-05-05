#include "peer.hpp"
#include "udp.hpp"
#include <thread>
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
                connections.push_back({ .socket = client, .name = "incoming" });
            }
            send_message(client, "NAME " + name + "\n");

            std::thread(&Peer::handle_client, this, client).detach();
        }
    }
}
void Peer::handle_client(int client) {
    std::string leftover;

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
                std::string new_name = msg.substr(5);
                std::lock_guard<std::mutex> lock(conn_mutex);
                for (auto& conn : connections)
                    if (conn.socket == client) { conn.name = new_name; break; }
            } else {
                std::string sender = "unknown";
                {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    for (auto& c : connections)
                        if (c.socket == client) { sender = c.name; break; }
                }
                {
                    std::lock_guard<std::mutex> lock(msg_mutex);
                    messages.push_back({ sender, msg });
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
    start_udp_listener(tcp_port);
}

void Peer::discover() {
    // TODO: get peer name from configuration
    broadcast_discovery(tcp_port, "bridge-peer");
}

void Peer::connect(const RemotePeer& peer) {
    int sock = connect_to_peer(peer.ip, peer.port);
    if (sock >= 0) {
        {
            std::lock_guard<std::mutex> lock(conn_mutex);
            connections.push_back({
                    .socket = sock, 
                    .name = peer.name
            });
        }

        // send_message(sock, "hello from " + peer.name);
        send_message(sock, "NAME " + name + "\n");
        std::thread(&Peer::handle_client, this, sock).detach();
        // std::cout << "Connected successfully to " + peer.ip + ":" + std::to_string(peer.port);
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
        send_message(connections[index].socket, msg + "\n");
    }
    {
        std::lock_guard<std::mutex> lock(msg_mutex);
        messages.push_back({ "You", msg });
    }
}

std::vector<Message> Peer::get_messages(){
    std::lock_guard<std::mutex> lock(msg_mutex);
    return messages;
}