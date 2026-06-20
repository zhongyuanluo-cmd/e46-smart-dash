#pragma once

#include <string>
#include <functional>
#include <linux/can.h>
#include <linux/can/raw.h>

namespace e46 {
namespace can {

using FrameCallback = std::function<void(const can_frame&)>;

class CanInterface
{
public:
    CanInterface();
    ~CanInterface();

    bool open(const std::string& ifname = "can0");
    bool openMock();  // Socket pair for testing (no hardware needed)
    void close();
    bool isOpen() const { return fd_ >= 0; }

    bool bringUp(int bitrate = 500000);
    bool isUp() const;  // ioctl-based, no system() call

    bool send(const can_frame& frame);
    bool recv(can_frame& frame, int timeoutMs = -1);

    int fd() const { return fd_; }
    int mockPairFd() const { return mockPairFd_; }
    const std::string& ifname() const { return ifname_; }
    bool isMock() const { return mockPairFd_ >= 0; }

private:
    int fd_ = -1;
    int mockPairFd_ = -1;
    std::string ifname_;
};

} // namespace can
} // namespace e46
