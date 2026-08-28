#pragma once
#include <functional>
#include <cstdint>
namespace engine {
class TcpServer {
public:
    explicit TcpServer(uint16_t port);
    void start(const std::function<void(const std::string&)>& onMessage);
private:
    int listen_fd_;
    uint16_t port_;
};
}
