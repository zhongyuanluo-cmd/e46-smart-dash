#pragma once

#include <cstdint>
#include "e46/common/types.h"

namespace e46 {
namespace adc {

class AdcSampler
{
public:
    AdcSampler();

    bool init(int i2cBus = 1, uint8_t i2cAddr = 0x48);
    void shutdown();

    // Read raw ADC value, convert to battery mV
    // Returns true if successful, stores result in data.battery_voltage_mv
    bool sample(common::VehicleData& data);

private:
    bool readRaw(int16_t& raw);
    uint16_t rawToBatteryMv(int16_t raw) const;

    int i2cFd_ = -1;
    int i2cBus_ = 1;
    uint8_t i2cAddr_ = 0x48;

    // Voltage divider: R1=10kΩ (upper), R2=3.3kΩ (lower)
    // Divider ratio = R2/(R1+R2) = 3300/13300 = 0.24812
    // Battery voltage = ADC_voltage / dividerRatio
    // ADS1115 FSR = ±4.096V at gain=1, 16-bit in two's complement
    // 1 LSB = 4.096V / 32768 = 0.125mV
    static constexpr float DIVIDER_RATIO = 3300.0f / 13300.0f;
    static constexpr float LSB_MV = 0.125f;

    // Low-voltage thresholds
    static constexpr uint16_t BATTERY_LOW_ENGINE_OFF_MV  = 11500;
    static constexpr uint16_t BATTERY_LOW_ENGINE_ON_MV   = 13000;
};

} // namespace adc
} // namespace e46
