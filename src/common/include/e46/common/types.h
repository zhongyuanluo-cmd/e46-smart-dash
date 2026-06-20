#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include <chrono>

namespace e46 {
namespace common {

// Compile-time guarantee: lock-free atomics on the target platform.
// On aarch64 (Cortex-A55, GCC 10.3) these are lock-free; this assert
// catches regressions if the toolchain or platform changes.
static_assert(std::atomic<uint8_t>::is_always_lock_free,
              "atomic<uint8_t> must be lock-free on target platform");
static_assert(std::atomic<uint16_t>::is_always_lock_free,
              "atomic<uint16_t> must be lock-free on target platform");
static_assert(std::atomic<uint32_t>::is_always_lock_free,
              "atomic<uint32_t> must be lock-free on target platform");
static_assert(ATOMIC_BOOL_LOCK_FREE == 2,
              "atomic bool must be always lock-free");

struct VehicleData
{
    // --- Engine ---
    std::atomic<uint16_t> rpm{0};
    std::atomic<uint8_t>  throttle_pct{0};
    std::atomic<int8_t>   coolant_temp_c{-40};  // sentinel: -40 = no data
    std::atomic<uint8_t>  oil_temp_c{0};

    // --- Speed & Odometer ---
    std::atomic<uint16_t> speed_kmh{0};   // uint16: 0-65535 (track mode >255)
    std::atomic<uint32_t> odometer_km{0};

    // --- Electrical ---
    std::atomic<uint16_t> battery_voltage_mv{0};  // millivolts
    std::atomic<bool>     battery_low{false};
    std::atomic<bool>     charging_fault{false};

    // --- Status ---
    std::atomic<bool>     engine_running{false};
    std::atomic<bool>     ignition_on{false};
    std::atomic<uint8_t>  fuel_level_pct{0};

    // --- CAN Interface Status ---
    std::atomic<bool>     can_connected{false};
    std::atomic<bool>     kbus_connected{false};

    // --- Timestamps (monotonic ms) ---
    std::atomic<int64_t>  last_can_update_ms{0};
    std::atomic<int64_t>  last_kbus_update_ms{0};
    std::atomic<int64_t>  last_adc_update_ms{0};

    // --- Helpers ---
    void mark_can_update();
    void mark_kbus_update();
    void mark_adc_update();

    static int64_t now_ms();
};

} // namespace common
} // namespace e46
