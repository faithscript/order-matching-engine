#include "socket.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace engine {
TcpServer::TcpServer(uint16_t port) : listen_fd_(-1), port_(port) {}
void TcpServer::start(const std::function<void(const std::string&)>& onMessage) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "socket error" << std::endl;
        return;
    }
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind error" << std::endl;
        close(listen_fd_);
        return;
    }
    if (listen(listen_fd_, 1) < 0) {
        std::cerr << "listen error" << std::endl;
        close(listen_fd_);
        return;
    }
    while (true) {
        int client_fd = accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            std::cerr << "accept error" << std::endl;
            continue;
        }
        std::string buffer;
        char recv_buf[1024];
        while (true) {
            ssize_t n = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
            if (n <= 0) break;
            buffer.append(recv_buf, n);
            size_t pos;
            while ((pos = buffer.find('\x01')) != std::string::npos) {
                std::string message = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                onMessage(message);
            }
        }
        close(client_fd);
    }
}
}
