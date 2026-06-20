#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int main() {
    int fd = open("/dev/ttyS4", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) { printf("FAIL open: %s\n", strerror(errno)); return 1; }
    struct termios t;
    tcgetattr(fd, &t);
    cfsetospeed(&t, B9600); cfsetispeed(&t, B9600);
    t.c_cflag |= PARENB; t.c_cflag &= ~PARODD; t.c_cflag &= ~CSTOPB;
    t.c_cflag = (t.c_cflag & ~CSIZE) | CS8;
    t.c_cflag &= ~CRTSCTS; t.c_cflag |= CREAD | CLOCAL;
    t.c_lflag &= ~(ICANON | ECHO | ISIG);
    t.c_iflag &= ~(IXON | IXOFF | IXANY);
    t.c_oflag &= ~OPOST;
    t.c_cc[VMIN]=0; t.c_cc[VTIME]=5;
    tcsetattr(fd, TCSANOW, &t);
    printf("PASS: ttyS4 9600 8E1 OK\n");
    uint8_t b = 0xFF;
    write(fd, &b, 1);
    printf("Sent 0xFF on K-Bus\n");
    close(fd);
    return 0;
}
