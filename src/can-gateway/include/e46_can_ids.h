#pragma once

// E46 PT-CAN (500kbps) known ARBIDs
// Source: loopybunny.co.uk BMW CAN database, E46 community knowledge
// NOTE: These are preliminary and MUST be verified with candump on the actual vehicle.
// Byte offsets and scaling formulas are best-guess based on community references.

#include <cstdint>

namespace e46 {
namespace can {

// --- DME (Engine Control) Messages ---
constexpr uint32_t ARBID_ENGINE_RPM      = 0x0AA;  // Engine RPM + throttle position (KCan1)
constexpr uint32_t ARBID_ENGINE_TEMP     = 0x1D0;  // Coolant temp, oil pressure (KCan1)
constexpr uint32_t ARBID_TORQUE_BRAKE    = 0x0A8;  // Torque, clutch, brake status (KCan1)
constexpr uint32_t ARBID_THROTTLE_RPM2   = 0x0A5;  // Alternate throttle + RPM (KCan2 only)

// --- DSC/ABS (Dynamic Stability Control) Messages ---
constexpr uint32_t ARBID_WHEEL_SPEEDS    = 0x0CE;  // Individual wheel speeds x4 (KCan1)
constexpr uint32_t ARBID_BRAKE_COUNTER   = 0x0C0;  // ABS/Brake counter
constexpr uint32_t ARBID_ABS_BRAKING     = 0x19E;  // ABS braking force (KCan1)

// --- KOMBI (Instrument Cluster) Messages ---
constexpr uint32_t ARBID_SPEED_KOMBI     = 0x1A6;  // Speed as used by cluster (KCan1)
constexpr uint32_t ARBID_SPEED_MPH       = 0x1B4;  // Speed MPH + handbrake (KCan1)
constexpr uint32_t ARBID_ODOMETER        = 0x330;  // Odometer, avg fuel, range (KCan1)

// --- Electrical ---
constexpr uint32_t ARBID_BATTERY_VOLTAGE = 0x3B4;  // Battery voltage + charge status (KCan1)

// --- Status ---
constexpr uint32_t ARBID_IGNITION_KEY    = 0x130;  // Ignition and key status (Term 15/R)
constexpr uint32_t ARBID_IGNITION_STATUS = 0x26E;  // Ignition status (CAS)

// --- Steering wheel ---
constexpr uint32_t ARBID_STEERING_WHEEL  = 0x1D6;  // MFL steering wheel buttons (KCan1)
constexpr uint32_t ARBID_STEERING_ANGLE  = 0x0C4;  // Steering wheel position (KCan1)

} // namespace can
} // namespace e46
