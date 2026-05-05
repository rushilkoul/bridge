#pragma once
#include <string>
#include <mutex>
#include <vector>

struct RemotePeer {
    std::string ip;
    int port;
    std::string name;
};

struct Connection {
    int socket;
    std::string name;
    std::string shared_key;
};

struct Message {
    std::string sender;
    std::string text;
};


class Peer {
private:
    int tcp_port;

    std::vector<Connection> connections;
    std::mutex conn_mutex;

    std::vector<Message> messages;
    std::mutex msg_mutex;

    void tcp_server();
    void handle_client(int client);
    void udp_listener();

public:
    Peer(int port, std::string _name);
    void start();
    void discover(); // UDP broadcast for peer discovery

    void connect(const RemotePeer& peer);
    void send_to(int index, const std::string& msg);
    std::string name;
    std::vector<Connection> get_connections();
    std::vector<Message> get_messages(); 
};