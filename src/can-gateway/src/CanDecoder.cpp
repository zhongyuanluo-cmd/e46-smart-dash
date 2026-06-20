#include "CanDecoder.h"
#include "e46_can_ids.h"
#include "e46/common/logging.h"
#include <cstring>

namespace e46 {
namespace can {

CanDecoder::CanDecoder()
{
    // Register known ARBID handlers
    registerHandler(ARBID_ENGINE_RPM,      [this](auto& f, auto& d) { decodeEngineRpm(f, d); });
    registerHandler(ARBID_ENGINE_TEMP,     [this](auto& f, auto& d) { decodeEngineTemp(f, d); });
    registerHandler(ARBID_WHEEL_SPEEDS,    [this](auto& f, auto& d) { decodeWheelSpeeds(f, d); });
    registerHandler(ARBID_SPEED_KOMBI,     [this](auto& f, auto& d) { decodeSpeed(f, d); });
    registerHandler(ARBID_SPEED_MPH,       [this](auto& f, auto& d) { decodeSpeed(f, d); });
    registerHandler(ARBID_BATTERY_VOLTAGE, [this](auto& f, auto& d) { decodeBatteryVoltage(f, d); });
    registerHandler(ARBID_IGNITION_KEY,    [this](auto& f, auto& d) { decodeIgnition(f, d); });
    registerHandler(ARBID_TORQUE_BRAKE,    [this](auto& f, auto& d) { decodeTorqueBrake(f, d); });
}

void CanDecoder::decode(const can_frame& frame, common::VehicleData& data)
{
    // E46 PT-CAN uses 11-bit standard frames (SFF), not 29-bit extended (EFF)
    auto it = handlers_.find(frame.can_id & CAN_SFF_MASK);
    if (it != handlers_.end()) {
        it->second(frame, data);
        data.can_connected.store(true, std::memory_order_relaxed);
        data.mark_can_update();
    } else {
        // Silent ignore: unknown ARBIDs are normal during initial CAN discovery
        if (++unknownCount_ == 1 || unknownCount_ % 1000 == 0) {
            E46_LOG_DEBUG("Unknown CAN ARBID: 0x%03X (count: %llu)",
                          frame.can_id & CAN_SFF_MASK,
                          static_cast<unsigned long long>(unknownCount_));
        }
    }
}

void CanDecoder::registerHandler(uint32_t arbid, CanFrameHandler handler)
{
    handlers_[arbid] = std::move(handler);
}

// --- Decoding Functions ---

void CanDecoder::decodeEngineRpm(const can_frame& frame, common::VehicleData& data)
{
    if (frame.can_dlc < 4) return;
    // Bytes 0-1: RPM = (byte0 * 256 + byte1) / 6.4  (community best-guess)
    // Adjust after real CAN capture
    uint16_t raw = (frame.data[0] << 8) | frame.data[1];
    uint16_t rpm = static_cast<uint16_t>(raw / 6.4f);
    data.rpm.store(rpm, std::memory_order_relaxed);

    // Byte 2: Throttle position, 0-255 → 0-100%
    if (frame.can_dlc >= 3) {
        uint8_t throttle = static_cast<uint8_t>(frame.data[2] * 100 / 255);
        data.throttle_pct.store(throttle, std::memory_order_relaxed);
    }

    // TODO: Verify with candump — what RPM does DME report when engine is off?
    // Starter cranking speed is typically 200-300 RPM; threshold 200 avoids
    // misidentifying starter engagement as engine running.
    data.engine_running.store(rpm > 200, std::memory_order_relaxed);
}

void CanDecoder::decodeEngineTemp(const can_frame& frame, common::VehicleData& data)
{
    if (frame.can_dlc < 3) return;
    // Byte 0: Coolant temp in °C = raw - 48  (BMW standard offset)
    int16_t raw = frame.data[0];
    int8_t temp = static_cast<int8_t>(raw - 48);
    data.coolant_temp_c.store(temp, std::memory_order_relaxed);
}

void CanDecoder::decodeWheelSpeeds(const can_frame& frame, common::VehicleData& data)
{
    if (frame.can_dlc < 8) return;
    // Bytes 0-1: FL speed, 2-3: FR, 4-5: RL, 6-7: RR
    // Each pair: (byteH << 8 | byteL) * 0.05625 = km/h  (community estimate)
    // We take front-left as primary speed
    uint16_t raw = (frame.data[1] << 8) | frame.data[0];
    uint8_t speed = static_cast<uint8_t>(raw * 0.05625f);
    if (speed > 0 && speed < 300) {
        data.speed_kmh.store(speed, std::memory_order_relaxed);
    }
}

void CanDecoder::decodeSpeed(const can_frame& frame, common::VehicleData& data)
{
    if (frame.can_dlc < 2) return;
    // Byte 0: Speed in km/h (direct value for some KOMBI messages)
    uint8_t speed = frame.data[0];
    if (speed > 0 && speed < 300) {
        data.speed_kmh.store(speed, std::memory_order_relaxed);
    }
}

void CanDecoder::decodeBatteryVoltage(const can_frame& frame, common::VehicleData& data)
{
    if (frame.can_dlc < 2) return;
    // Bytes 0-1: Battery voltage = raw * 0.1V → mV
    uint16_t raw = (frame.data[0] << 8) | frame.data[1];
    uint16_t mv = static_cast<uint16_t>(raw * 100);  // 0.1V resolution → mV
    data.battery_voltage_mv.store(mv, std::memory_order_relaxed);

    // Low battery detection: < 11.5V engine off
    if (!data.engine_running.load() && mv < 11500) {
        data.battery_low.store(true, std::memory_order_relaxed);
    } else if (data.engine_running.load() && mv > 13000) {
        data.battery_low.store(false, std::memory_order_relaxed);
    }
}

void CanDecoder::decodeIgnition(const can_frame& frame, common::VehicleData& data)
{
    if (frame.can_dlc < 2) return;
    // Byte 0 bit patterns indicate ignition state (community best-guess)
    bool ign = (frame.data[0] & 0x01) != 0;
    data.ignition_on.store(ign, std::memory_order_relaxed);
}

void CanDecoder::decodeTorqueBrake(const can_frame& frame, common::VehicleData& data)
{
    // Placeholder: torque, clutch, brake status
    // Will be refined after CAN capture
    (void)frame;
    (void)data;
}

} // namespace can
} // namespace e46
