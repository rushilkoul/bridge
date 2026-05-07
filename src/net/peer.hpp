#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <set>

struct RemotePeer
{
    std::string ip;
    int port;
    std::string name;

    bool operator<(const RemotePeer &other) const
    {
        if (ip != other.ip)
            return ip < other.ip;
        return port < other.port;
    }
};

struct Connection
{
    int socket;
    std::string name;
    std::string shared_key;
};

struct Message
{
    std::string sender;
    std::string text;
    std::string peer_name;
};

class Peer
{
private:
    int tcp_port;

    std::vector<Connection> connections;
    std::mutex conn_mutex;

    std::set<RemotePeer> discovered_peers;
    std::mutex discovered_mutex;

    std::vector<Message> messages;
    std::mutex msg_mutex;

    void tcp_server();
    void handle_client(int client);
    void udp_listener();
    void discovery_broadcast();

public:
    Peer(int port, std::string _name);
    void start();
    void discover(); // UDP broadcast for peer discovery

    void connect(const RemotePeer &peer);
    void send_to(int index, const std::string &msg);
    void add_discovered_peer(const RemotePeer &peer);

    std::string name;
    std::vector<Connection> get_connections();
    std::vector<Message> get_messages();
    std::vector<RemotePeer> get_discovered_peers();
};