#pragma once

#include <string>
#include <cstdint>
#include <functional>

namespace e46 {
namespace kbus {

using ByteCallback = std::function<void(uint8_t)>;

class KBusInterface
{
public:
    KBusInterface();
    ~KBusInterface();

    bool open(const std::string& device = "/dev/ttyS4", int baud = 9600);
    void close();
    bool isOpen() const { return fd_ >= 0; }

    bool sendByte(uint8_t byte);
    int recvByte(int timeoutMs = -1);

    int fd() const { return fd_; }

private:
    bool configureUart(int baud);

    // Wait for bus silence (>2.1ms at 9600 baud) before transmitting.
    // Returns true if bus is idle, false if RX activity detected or error.
    bool waitForBusIdle(int timeoutUs = 2100);

    int fd_ = -1;
    std::string device_;
};

} // namespace kbus
} // namespace e46
