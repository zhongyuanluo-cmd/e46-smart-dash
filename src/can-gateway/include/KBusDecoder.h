#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <chrono>
#include "e46/common/types.h"

namespace e46 {
namespace kbus {

// K-Bus frame: [src] [len] [dst] [data...] [checksum]
// Checksum = XOR of all preceding bytes
struct KBusFrame
{
    uint8_t src;
    uint8_t len;   // total frame length including checksum
    uint8_t dst;
    std::vector<uint8_t> data;
    uint8_t checksum;
    bool valid = false;
};

// Known K-Bus device addresses (E46)
constexpr uint8_t KBUS_ADDR_GM5    = 0x00;  // General Module (body computer)
constexpr uint8_t KBUS_ADDR_CCM    = 0x08;  // Check Control Module
constexpr uint8_t KBUS_ADDR_IKE    = 0x80;  // Instrument Cluster
constexpr uint8_t KBUS_ADDR_LCM    = 0xD0;  // Light Control Module
constexpr uint8_t KBUS_ADDR_MFL    = 0x50;  // Multi-Function Steering Wheel
constexpr uint8_t KBUS_ADDR_CD     = 0x68;  // CD Changer
constexpr uint8_t KBUS_ADDR_RAD    = 0x68;  // Radio (same as CD on some)
constexpr uint8_t KBUS_ADDR_TEL    = 0xC8;  // Telephone module
constexpr uint8_t KBUS_ADDR_DIA    = 0xF0;  // Diagnostics
constexpr uint8_t KBUS_ADDR_BROADCAST = 0xFF; // Broadcast to all

using FrameHandler = std::function<void(const KBusFrame&, common::VehicleData&)>;

class KBusDecoder
{
public:
    KBusDecoder();

    // Feed raw bytes, returns complete frames when available
    void feedByte(uint8_t byte);
    bool hasFrame() const { return !frameQueue_.empty(); }
    KBusFrame popFrame();

    void registerHandler(uint8_t src, uint8_t dst, FrameHandler handler);
    void decode(const KBusFrame& frame, common::VehicleData& data);

private:
    enum class State { IDLE, SRC, LEN, DST, DATA, CHECKSUM };

    State state_ = State::IDLE;
    KBusFrame currentFrame_;
    uint8_t dataIdx_ = 0;
    uint8_t checksumCalc_ = 0;
    std::deque<KBusFrame> frameQueue_;

    struct HandlerKey {
        uint8_t src;
        uint8_t dst;
        bool operator==(const HandlerKey& o) const { return src == o.src && dst == o.dst; }
    };
    struct HandlerKeyHash {
        size_t operator()(const HandlerKey& k) const { return (k.src << 8) | k.dst; }
    };
    std::unordered_map<HandlerKey, FrameHandler, HandlerKeyHash> handlers_;

    // MFL steering wheel button decode
    void reset();
    void decodeMflButtons(const KBusFrame& frame, common::VehicleData& data);

    // Partial frame timeout: 50ms (spec requirement)
    static constexpr int PARTIAL_FRAME_TIMEOUT_MS = 50;
    using Clock = std::chrono::steady_clock;
    Clock::time_point lastByteTime_;
};

} // namespace kbus
} // namespace e46
