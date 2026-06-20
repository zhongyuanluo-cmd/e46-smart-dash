#include "KBusInterface.h"
#include "e46/common/logging.h"
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>

namespace e46 {
namespace kbus {

KBusInterface::KBusInterface() = default;

KBusInterface::~KBusInterface()
{
    close();
}

bool KBusInterface::open(const std::string& device, int baud)
{
    device_ = device;
    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        E46_LOG_ERROR("Failed to open %s: %s", device.c_str(), strerror(errno));
        return false;
    }

    if (!configureUart(baud)) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    E46_LOG_INFO("K-Bus UART %s opened (fd=%d, %d baud 8E1)", device.c_str(), fd_, baud);
    return true;
}

void KBusInterface::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        E46_LOG_INFO("K-Bus UART %s closed", device_.c_str());
        fd_ = -1;
    }
}

bool KBusInterface::configureUart(int baud)
{
    struct termios tty;
    std::memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd_, &tty) != 0) {
        E46_LOG_ERROR("tcgetattr failed: %s", strerror(errno));
        return false;
    }

    // Map baud rate to speed_t constant
    speed_t speed;
    switch (baud) {
        case 9600:  speed = B9600;  break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default:
            E46_LOG_ERROR("Unsupported K-Bus baud rate: %d", baud);
            return false;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag &= ~PARENB;  // No parity first
    tty.c_cflag |= PARENB;   // Enable parity
    tty.c_cflag &= ~PARODD;  // Even parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_cflag &= ~CRTSCTS; // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR);
    tty.c_oflag &= ~OPOST;

    // Read timeout: 0.5 seconds (enough for 4 bytes at 9600 baud)
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        E46_LOG_ERROR("tcsetattr failed: %s", strerror(errno));
        return false;
    }

    // Flush buffers
    tcflush(fd_, TCIOFLUSH);
    return true;
}

bool KBusInterface::waitForBusIdle(int timeoutUs)
{
    if (fd_ < 0) return false;

    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;

    // Poll with sub-millisecond timeout: if data arrives, bus is NOT idle
    int ret = poll(&pfd, 1, 0);  // immediate check first
    if (ret > 0) return false;    // RX data pending, bus is busy
    if (ret < 0) return false;    // error

    // No immediate data — wait for guaranteed idle window
    // At 9600 baud, one byte = ~1.04ms, 2 byte times = ~2.1ms
    usleep(timeoutUs);
    return true;
}

bool KBusInterface::sendByte(uint8_t byte)
{
    if (fd_ < 0) return false;

    // Wait for bus idle before transmitting (K-Bus collision avoidance)
    if (!waitForBusIdle()) {
        E46_LOG_WARN("K-Bus busy, deferring send");
        return false;
    }

    ssize_t n = write(fd_, &byte, 1);
    if (n != 1) {
        E46_LOG_WARN("K-Bus send failed: %s", strerror(errno));
        return false;
    }
    // Wait for byte to be transmitted (at 9600 baud: ~1ms per byte)
    tcdrain(fd_);
    return true;
}

int KBusInterface::recvByte(int timeoutMs)
{
    if (fd_ < 0) return -1;

    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeoutMs);
    if (ret <= 0) return -1;

    uint8_t byte;
    ssize_t n = read(fd_, &byte, 1);
    if (n == 1) return byte;
    return -1;
}

} // namespace kbus
} // namespace e46
