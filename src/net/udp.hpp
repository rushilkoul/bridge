#pragma once
#include <string>
#include <vector>
#include "peer.hpp"

// UDP broadcast for peer discovery
void broadcast_discovery(int port, const std::string& peer_name);

// Send UDP message to a specific peer
void send_udp(const std::string& ip, int port, const std::string& message);

// Listen for incoming UDP discovery messages
void start_udp_listener(int port);

// Parse discovery message to extract peer info
RemotePeer parse_discovery_message(const std::string& message, const std::string& sender_ip);
