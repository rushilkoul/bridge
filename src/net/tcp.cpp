#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>


void send_message(int socket, const std::string& message) {
    write(socket, message.c_str(), message.length());
}


std::string receive_message(int socket) {
    char buffer[1024] = {0};
    int bytes = read(socket, buffer, sizeof(buffer));
    
    if (bytes <= 0) return "";  // connection closed

    return std::string(buffer, bytes);
}

void handle_client(int client) {
    while (true) {
        std::string msg = receive_message(client);
        if (msg.empty()) break;
        std::cout << "received: " << msg << "\n";
    }
    close(client);
}

void start_tcp_server(int port) {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "bind() failed\n";
        return;
    }

    listen(server_fd, 5); // backlog is 5 for now. maybe change this later?

    std::cout << "TCP server listening on port " << port << "\n";

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);

        if (client >= 0) {
            std::thread(&handle_client, client).detach();
        }
    }
}

int connect_to_peer(const std::string& ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "connect() failed\n";
        close(sock);

        return -1;
    }
    return sock;
}