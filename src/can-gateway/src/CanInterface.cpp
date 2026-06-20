#include "CanInterface.h"
#include "e46/common/logging.h"
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cerrno>

namespace e46 {
namespace can {

namespace {
// Validate interface name: only lowercase letters, digits, hyphen, max 15 chars.
// Prevents command injection when used with system() in bringUp().
bool validIfname(const std::string& name)
{
    return !name.empty() && name.size() < 16 &&
           name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") == std::string::npos;
}
} // anonymous namespace

CanInterface::CanInterface() = default;

CanInterface::~CanInterface()
{
    close();
}

bool CanInterface::open(const std::string& ifname)
{
    ifname_ = ifname;

    fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd_ < 0) {
        E46_LOG_ERROR("Failed to create CAN socket: %s", strerror(errno));
        return false;
    }

    // Set non-blocking for epoll integration
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);

    // Enable CAN FD support (backward compatible)
    int enable = 1;
    setsockopt(fd_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable));

    // Enable error frames for link monitoring
    can_err_mask_t errMask = CAN_ERR_MASK;
    setsockopt(fd_, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &errMask, sizeof(errMask));

    // Bind to interface
    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, ifname_.c_str(), IFNAMSIZ - 1);
    if (ioctl(fd_, SIOCGIFINDEX, &ifr) < 0) {
        E46_LOG_ERROR("Failed to get index for %s: %s", ifname_.c_str(), strerror(errno));
        close();
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        E46_LOG_ERROR("Failed to bind CAN socket: %s", strerror(errno));
        close();
        return false;
    }

    E46_LOG_INFO("CAN interface %s opened (fd=%d)", ifname_.c_str(), fd_);
    return true;
}

bool CanInterface::openMock()
{
    ifname_ = "mock0";
    int socks[2];
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, socks) < 0) {
        E46_LOG_ERROR("Failed to create mock socket pair: %s", strerror(errno));
        return false;
    }
    fd_ = socks[0];
    mockPairFd_ = socks[1];  // Other end for test frame injection

    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);

    E46_LOG_INFO("Mock CAN interface opened (fd=%d, pair=%d)", fd_, mockPairFd_);
    return true;
}

void CanInterface::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    if (mockPairFd_ >= 0) {
        ::close(mockPairFd_);
        mockPairFd_ = -1;
    }
    if (!ifname_.empty()) {
        E46_LOG_INFO("CAN interface %s closed", ifname_.c_str());
    }
}

bool CanInterface::bringUp(int bitrate)
{
    if (!validIfname(ifname_)) {
        E46_LOG_ERROR("Invalid CAN interface name: '%s'", ifname_.c_str());
        return false;
    }
    std::string cmd = "ip link set " + ifname_ + " type can bitrate " +
                      std::to_string(bitrate) +
                      " && ip link set up " + ifname_;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        E46_LOG_ERROR("Failed to bring up %s at %d bps", ifname_.c_str(), bitrate);
        return false;
    }
    E46_LOG_INFO("%s up at %d bps", ifname_.c_str(), bitrate);
    return true;
}

bool CanInterface::isUp() const
{
    if (fd_ < 0) return false;
    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, ifname_.c_str(), IFNAMSIZ - 1);
    if (ioctl(fd_, SIOCGIFFLAGS, &ifr) < 0) return false;
    return (ifr.ifr_flags & IFF_UP) != 0;
}

bool CanInterface::send(const can_frame& frame)
{
    if (fd_ < 0) return false;
    ssize_t n = write(fd_, &frame, sizeof(frame));
    if (n != sizeof(frame)) {
        E46_LOG_WARN("CAN send failed: %s (n=%zd)", strerror(errno), n);
        return false;
    }
    return true;
}

bool CanInterface::recv(can_frame& frame, int timeoutMs)
{
    if (fd_ < 0) return false;

    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeoutMs);
    if (ret <= 0) return false;

    ssize_t n = read(fd_, &frame, sizeof(frame));
    return n == sizeof(frame);
}

} // namespace can
} // namespace e46
