#pragma once
#include <string>
#include <vector>

struct RemotePeer;
class Peer;

void broadcast_discovery(int port, const std::string& peer_name);

void send_udp(const std::string& ip, int port, const std::string& message);

void start_udp_listener(int port, Peer* peer_ptr);

RemotePeer parse_discovery_message(const std::string& message, const std::string& sender_ip, int port);
