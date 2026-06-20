#pragma once

#include <string>
#include <memory>
#include <sys/epoll.h>
#include "e46/common/types.h"
#include "CanInterface.h"
#include "CanDecoder.h"
#include "KBusInterface.h"
#include "KBusDecoder.h"
#include "AdcSampler.h"

namespace e46 {
namespace gateway {

class GatewayDaemon
{
public:
    GatewayDaemon();
    ~GatewayDaemon();

    bool init(int argc, char* argv[]);
    int run();

private:
    void printUsage(const char* prog);
    bool parseArgs(int argc, char* argv[]);

    bool setupEpoll();
    void processCanFrame();
    void processKbusData();
    void processAdcTimer();
    void publishVehicleData();

    // Configuration
    std::string canIfname_ = "can0";
    std::string kbusUart_ = "/dev/ttyS4";
    std::string configPath_ = "/etc/e46/can-gateway.json";
    int canBitrate_ = 500000;
    int kbusBaud_ = 9600;
    int adcIntervalSec_ = 1;
    std::string logLevel_ = "INFO";
    bool vcanMode_ = false;
    bool mockMode_ = false;

    // Components
    std::unique_ptr<can::CanInterface> canIf_;
    std::unique_ptr<can::CanDecoder> decoder_;
    std::unique_ptr<kbus::KBusInterface> kbusIf_;
    std::unique_ptr<kbus::KBusDecoder> kbusDecoder_;
    std::unique_ptr<adc::AdcSampler> adcSampler_;
    e46::common::VehicleData vehicleData_;

    // Epoll
    int epollFd_ = -1;
    int adcTimerFd_ = -1;
    int kbusFd_ = -1;
    [[maybe_unused]] int dbusFd_ = -1;  // placeholder until DBus phase

    bool running_ = false;

    // Publish throttling (member, not static — one per instance)
    int publishCount_ = 0;
    int64_t lastPublishMs_ = 0;
};

} // namespace gateway
} // namespace e46
