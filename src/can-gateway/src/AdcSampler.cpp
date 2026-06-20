#include "AdcSampler.h"
#include "e46/common/logging.h"
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

namespace e46 {
namespace adc {

AdcSampler::AdcSampler() = default;

bool AdcSampler::init(int i2cBus, uint8_t i2cAddr)
{
    i2cBus_ = i2cBus;
    i2cAddr_ = i2cAddr;

    char devPath[32];
    snprintf(devPath, sizeof(devPath), "/dev/i2c-%d", i2cBus_);
    i2cFd_ = open(devPath, O_RDWR);
    if (i2cFd_ < 0) {
        E46_LOG_ERROR("Failed to open %s: %s", devPath, strerror(errno));
        return false;
    }

    if (ioctl(i2cFd_, I2C_SLAVE, i2cAddr_) < 0) {
        E46_LOG_ERROR("Failed to set I2C slave addr 0x%02X: %s", i2cAddr_, strerror(errno));
        close(i2cFd_);
        i2cFd_ = -1;
        return false;
    }

    E46_LOG_INFO("ADC ADS1115 initialized on i2c-%d addr 0x%02X", i2cBus_, i2cAddr_);
    return true;
}

void AdcSampler::shutdown()
{
    if (i2cFd_ >= 0) {
        close(i2cFd_);
        i2cFd_ = -1;
        E46_LOG_INFO("ADC sampler shutdown");
    }
}

bool AdcSampler::sample(common::VehicleData& data)
{
    if (i2cFd_ < 0) return false;

    int16_t raw;
    if (!readRaw(raw)) return false;

    uint16_t mv = rawToBatteryMv(raw);
    data.battery_voltage_mv.store(mv, std::memory_order_relaxed);

    bool engineRunning = data.engine_running.load();
    uint16_t threshold = engineRunning ? BATTERY_LOW_ENGINE_ON_MV : BATTERY_LOW_ENGINE_OFF_MV;

    if (mv < threshold) {
        if (engineRunning) {
            data.charging_fault.store(true, std::memory_order_relaxed);
            E46_LOG_WARN("Charging fault: battery %u mV < %u mV (engine running)", mv, threshold);
        } else {
            data.battery_low.store(true, std::memory_order_relaxed);
            E46_LOG_WARN("Low battery: %u mV < %u mV (engine off)", mv, threshold);
        }
    } else {
        data.battery_low.store(false, std::memory_order_relaxed);
        if (engineRunning) {
            data.charging_fault.store(false, std::memory_order_relaxed);
        }
    }

    return true;
}

bool AdcSampler::readRaw(int16_t& raw)
{
    // ADS1115 registers
    // 0x00 = Conversion register (read-only result)
    // 0x01 = Config register (write to trigger + configure)
    //
    // Config: AIN0-GND, ±4.096V (PGA gain=1), single-shot mode, 128 SPS
    // 16-bit config: 0xC383 = MSB 0xC3, LSB 0x83
    //   bit 15: OS=1 (start conversion)
    //   bits 14-12: MUX=100 (AIN0-GND)
    //   bits 11-9: PGA=001 (±4.096V)
    //   bit 8: MODE=1 (single-shot)
    //   bits 7-5: DR=100 (128 SPS)
    //   bits 4-0: default (no comparator)

    uint8_t config[3] = {0x01, 0xC3, 0x83};  // register pointer + config MSB + LSB
    if (write(i2cFd_, config, 3) != 3) {
        E46_LOG_WARN("ADC: failed to write config register");
        return false;
    }

    // Wait for conversion (1/128 SPS ≈ 8ms, poll with timeout)
    usleep(10000);  // 10ms — more than enough

    // Write pointer to conversion register
    uint8_t reg = 0x00;
    if (write(i2cFd_, &reg, 1) != 1) {
        E46_LOG_WARN("ADC: failed to set conversion register pointer");
        return false;
    }

    // Read 2 bytes (MSB first, big-endian)
    uint8_t buf[2];
    if (read(i2cFd_, buf, 2) != 2) {
        E46_LOG_WARN("ADC: failed to read conversion data");
        return false;
    }

    raw = (static_cast<int16_t>(buf[0]) << 8) | buf[1];
    return true;
}

uint16_t AdcSampler::rawToBatteryMv(int16_t raw) const
{
    // raw is 16-bit two's complement signed
    // Positive voltage = raw * LSB_MV (mV at ADC input)
    float adcMv = static_cast<float>(raw) * LSB_MV;
    // Scale through voltage divider
    float batteryMv = adcMv / DIVIDER_RATIO;
    if (batteryMv < 0) batteryMv = 0;
    return static_cast<uint16_t>(batteryMv);
}

} // namespace adc
} // namespace e46
