#pragma once
#include <string>

void send_message(int socket, const std::string& message);
std::string receive_message(int socket);

void handle_client(int client);

void start_tcp_server(int port);
int connect_to_peer(const std::string& ip, int port);