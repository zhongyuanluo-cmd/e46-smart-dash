#include "GatewayDaemon.h"
#include "e46/common/logging.h"
#include "e46/common/config.h"
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/timerfd.h>
#include <iostream>

namespace e46 {
namespace gateway {

// Global for signal handler
static volatile sig_atomic_t g_running = 1;

static void signalHandler(int)
{
    g_running = 0;
}

GatewayDaemon::GatewayDaemon()
{
    canIf_ = std::make_unique<can::CanInterface>();
    decoder_ = std::make_unique<can::CanDecoder>();
    kbusIf_ = std::make_unique<kbus::KBusInterface>();
    kbusDecoder_ = std::make_unique<kbus::KBusDecoder>();
}

GatewayDaemon::~GatewayDaemon()
{
    running_ = false;
    // Close epoll first to stop event monitoring, then the monitored fds
    if (epollFd_ >= 0) { close(epollFd_); epollFd_ = -1; }
    if (adcTimerFd_ >= 0) { close(adcTimerFd_); adcTimerFd_ = -1; }
    if (kbusFd_ >= 0) { close(kbusFd_); kbusFd_ = -1; }
}

void GatewayDaemon::printUsage(const char* prog)
{
    std::cerr << "Usage: " << prog << " [options]\n"
              << "  --vcan           Use virtual CAN (vcan0) for testing\n"
              << "  --mock           Use mock CAN (socket pair, no hardware)\n"
              << "  --can <iface>    CAN interface name (default: can0)\n"
              << "  --config <path>  Config file path (default: /etc/e46/can-gateway.json)\n"
              << "  -h, --help       Show this help\n";
}

bool GatewayDaemon::parseArgs(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--vcan") {
            vcanMode_ = true;
            canIfname_ = "vcan0";
        } else if (arg == "--mock") {
            mockMode_ = true;
            canIfname_ = "mock0";
        } else if (arg == "--can" && i + 1 < argc) {
            canIfname_ = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            configPath_ = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return false;
        } else {
            E46_LOG_ERROR("Unknown option: %s", arg.c_str());
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

bool GatewayDaemon::init(int argc, char* argv[])
{
    // Logging
    e46::common::Logger::instance().init("e46-cangw", true, "/var/log/e46-cangw.log");

    if (!parseArgs(argc, argv)) return false;

    // Load config (optional)
    e46::common::Config cfg;
    if (cfg.load(configPath_)) {
        canIfname_ = cfg.get<std::string>("can.interface", canIfname_);
        kbusUart_  = cfg.get<std::string>("kbus.uart", kbusUart_);
        canBitrate_ = cfg.get<int>("can.bitrate", 500000);
        kbusBaud_   = cfg.get<int>("kbus.baud", 9600);
        adcIntervalSec_ = cfg.get<int>("adc.interval_sec", 1);
        logLevel_   = cfg.get<std::string>("logging.level", "INFO");
    }

    // Open CAN interface
    bool opened;
    if (mockMode_) {
        opened = canIf_->openMock();
    } else {
        opened = canIf_->open(canIfname_);
    }
    if (!opened) {
        E46_LOG_ERROR("Failed to open CAN interface %s", canIfname_.c_str());
        return false;
    }

    if (!mockMode_ && !vcanMode_) {
        // Check if already UP via ioctl (no system() call)
        if (!canIf_->isUp()) {
            canIf_->bringUp(canBitrate_);
        } else {
            E46_LOG_INFO("%s already UP, skipping bringUp", canIfname_.c_str());
        }
    } else {
        E46_LOG_INFO("%s mode: skipping bringUp", mockMode_ ? "mock" : "vcan");
    }

    // Open K-Bus UART (non-fatal: continue without K-Bus if fails)
    if (kbusIf_->open(kbusUart_, kbusBaud_)) {
        kbusFd_ = kbusIf_->fd();
        E46_LOG_INFO("K-Bus UART %s opened", kbusUart_.c_str());
    } else {
        E46_LOG_WARN("K-Bus UART %s unavailable, continuing without K-Bus", kbusUart_.c_str());
    }

    // Initialize ADC sampler (non-fatal: continue without battery monitoring)
    adcSampler_ = std::make_unique<adc::AdcSampler>();
    int adcBus = cfg.get<int>("adc.i2c_bus", 1);
    int adcAddr = cfg.get<int>("adc.i2c_addr", 0x48);
    if (adcSampler_->init(adcBus, static_cast<uint8_t>(adcAddr))) {
        E46_LOG_INFO("ADC sampler initialized on i2c-%d", adcBus);
    } else {
        E46_LOG_WARN("ADC sampler unavailable, continuing without battery monitoring");
    }

    // Setup epoll (uses adcIntervalSec_ from config)
    if (!setupEpoll()) return false;

    // Signal handlers
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    E46_LOG_INFO("Gateway daemon initialized (CAN: %s, vcan: %s)",
                 canIfname_.c_str(), vcanMode_ ? "yes" : "no");
    return true;
}

bool GatewayDaemon::setupEpoll()
{
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        E46_LOG_ERROR("epoll_create1 failed: %s", strerror(errno));
        return false;
    }

    // CAN socket
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = canIf_->fd();
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, canIf_->fd(), &ev) < 0) {
        E46_LOG_ERROR("epoll_ctl CAN failed: %s", strerror(errno));
        return false;
    }

    // K-Bus UART (if available)
    if (kbusFd_ >= 0) {
        std::memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.fd = kbusFd_;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, kbusFd_, &ev) < 0) {
            E46_LOG_WARN("epoll_ctl K-Bus failed: %s", strerror(errno));
        }
    }

    // ADC timer (1 second interval)
    adcTimerFd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (adcTimerFd_ < 0) {
        E46_LOG_WARN("timerfd_create failed: %s, ADC disabled", strerror(errno));
        return true;  // ADC is non-critical
    }
    struct itimerspec its;
    its.it_value.tv_sec = adcIntervalSec_;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = adcIntervalSec_;
    its.it_interval.tv_nsec = 0;
    timerfd_settime(adcTimerFd_, 0, &its, nullptr);

    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = adcTimerFd_;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, adcTimerFd_, &ev) < 0) {
        E46_LOG_WARN("epoll_ctl ADC timer failed: %s (continuing without ADC)", strerror(errno));
    }

    return true;
}

int GatewayDaemon::run()
{
    running_ = true;
    E46_LOG_INFO("Gateway daemon running...");

    struct epoll_event events[8];

    while (running_ && g_running) {
        int n = epoll_wait(epollFd_, events, 8, 1000);

        for (int i = 0; i < n && running_; ++i) {
            if (events[i].data.fd == canIf_->fd()) {
                processCanFrame();
            } else if (events[i].data.fd == adcTimerFd_) {
                processAdcTimer();
            } else if (events[i].data.fd == kbusFd_) {
                processKbusData();
            }
        }
    }

    E46_LOG_INFO("Gateway daemon shutting down...");
    canIf_->close();
    if (kbusFd_ >= 0) { kbusIf_->close(); kbusFd_ = -1; }
    if (adcSampler_) adcSampler_->shutdown();
    return 0;
}

void GatewayDaemon::processCanFrame()
{
    struct can_frame frame;
    int batch = 0;
    while (canIf_->recv(frame, 0) && batch < 64) {
        decoder_->decode(frame, vehicleData_);
        ++batch;
    }

    // Time-based throttling: publish at most once every 200ms
    int64_t now = common::VehicleData::now_ms();
    if (now - lastPublishMs_ > 200) {
        publishVehicleData();
        lastPublishMs_ = now;
    }
}

void GatewayDaemon::processKbusData()
{
    int byte;
    while ((byte = kbusIf_->recvByte(0)) >= 0) {
        kbusDecoder_->feedByte(static_cast<uint8_t>(byte));
    }

    // Decode complete frames
    while (kbusDecoder_->hasFrame()) {
        auto frame = kbusDecoder_->popFrame();
        kbusDecoder_->decode(frame, vehicleData_);
    }
}

void GatewayDaemon::processAdcTimer()
{
    if (adcTimerFd_ < 0) return;

    uint64_t expirations;
    if (read(adcTimerFd_, &expirations, sizeof(expirations)) < 0) {
        E46_LOG_WARN("ADC timer read failed: %s", strerror(errno));
        return;
    }

    if (adcSampler_) {
        adcSampler_->sample(vehicleData_);
    }
    vehicleData_.mark_adc_update();
}

void GatewayDaemon::publishVehicleData()
{
    // Placeholder: DBus publishing (Tasks 5.3-5.4)
    // Will use system bus or session bus with com.e46.can1.VehicleData
    if (++publishCount_ <= 3 || publishCount_ % 600 == 0) {
        E46_LOG_DEBUG("VehicleData: RPM=%u SPD=%u CTMP=%d THR=%u BAT=%umV",
                      vehicleData_.rpm.load(),
                      vehicleData_.speed_kmh.load(),
                      vehicleData_.coolant_temp_c.load(),
                      vehicleData_.throttle_pct.load(),
                      vehicleData_.battery_voltage_mv.load());
    }
}

} // namespace gateway
} // namespace e46
