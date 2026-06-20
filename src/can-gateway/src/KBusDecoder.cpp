#include "KBusDecoder.h"
#include "e46/common/logging.h"
#include <algorithm>

namespace e46 {
namespace kbus {

// Wildcard for "any source" — use 0xFE rather than 0x00
// because 0x00 is the real address of GM5 (General Module).
constexpr uint8_t KBUS_SRC_ANY = 0xFE;

KBusDecoder::KBusDecoder()
    : lastByteTime_(Clock::now())
{
}

void KBusDecoder::reset()
{
    state_ = State::IDLE;
    currentFrame_ = KBusFrame{};
    dataIdx_ = 0;
    checksumCalc_ = 0;
}

void KBusDecoder::feedByte(uint8_t byte)
{
    // Partial frame timeout: if we've been stuck in a non-IDLE state for >50ms,
    // discard the partial frame and reset. Prevents permanent state machine stall.
    if (state_ != State::IDLE) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - lastByteTime_).count();
        if (elapsed > PARTIAL_FRAME_TIMEOUT_MS) {
            E46_LOG_WARN("K-Bus partial frame timeout (state=%d), resetting",
                         static_cast<int>(state_));
            reset();
        }
    }
    lastByteTime_ = Clock::now();

    checksumCalc_ ^= byte;

    switch (state_) {
    case State::IDLE:
        // First byte should be source address (> 0)
        if (byte > 0) {
            currentFrame_ = KBusFrame{};
            currentFrame_.src = byte;
            state_ = State::LEN;
        } else {
            checksumCalc_ = 0;  // Reset — this byte wasn't part of a frame
        }
        break;

    case State::LEN:
        currentFrame_.len = byte;
        state_ = State::DST;
        break;

    case State::DST:
        currentFrame_.dst = byte;
        dataIdx_ = 0;
        if (currentFrame_.len > 3) {
            state_ = State::DATA;
        } else {
            // No data payload, go straight to checksum
            state_ = State::CHECKSUM;
        }
        break;

    case State::DATA: {
        currentFrame_.data.push_back(byte);
        dataIdx_++;
        // len includes src(1) + len(1) + dst(1) + checksum(1) = 4 overhead
        uint8_t dataLen = (currentFrame_.len >= 4) ? currentFrame_.len - 4 : 0;
        if (dataIdx_ >= dataLen) {
            state_ = State::CHECKSUM;
        }
        break;
    }

    case State::CHECKSUM:
        // XOR of all bytes (including checksum itself) should be 0xFF
        // The last byte received IS the checksum, so checksumCalc_ already includes it
        currentFrame_.checksum = byte;
        currentFrame_.valid = (checksumCalc_ == 0xFF);
        if (!currentFrame_.valid) {
            E46_LOG_WARN("K-Bus checksum failed: src=0x%02X dst=0x%02X calc=0x%02X",
                         currentFrame_.src, currentFrame_.dst, checksumCalc_);
        }
        frameQueue_.push_back(currentFrame_);
        checksumCalc_ = 0;
        state_ = State::IDLE;
        break;
    }
}

KBusFrame KBusDecoder::popFrame()
{
    KBusFrame f = std::move(frameQueue_.front());
    frameQueue_.pop_front();
    return f;
}

void KBusDecoder::registerHandler(uint8_t src, uint8_t dst, FrameHandler handler)
{
    handlers_[{src, dst}] = std::move(handler);
}

void KBusDecoder::decode(const KBusFrame& frame, common::VehicleData& data)
{
    if (!frame.valid) return;

    // Try exact match first
    HandlerKey key{frame.src, frame.dst};
    auto it = handlers_.find(key);
    if (it == handlers_.end()) {
        // Try broadcast match
        key.dst = KBUS_ADDR_BROADCAST;
        it = handlers_.find(key);
    }
    if (it == handlers_.end()) {
        // Try any source to this destination (wildcard)
        key.src = KBUS_SRC_ANY;
        key.dst = frame.dst;
        it = handlers_.find(key);
    }

    if (it != handlers_.end()) {
        it->second(frame, data);
        data.mark_kbus_update();
    }
}

void KBusDecoder::decodeMflButtons(const KBusFrame& frame, common::VehicleData& data)
{
    if (frame.data.size() < 1) return;
    // MFL button codes: byte 0 bitfield
    // bit 0: VOL+, bit 1: VOL-, bit 2: Track+, bit 3: Track-,
    // bit 4: Phone, bit 5: R/T (mode)
    uint8_t buttons = frame.data[0];
    (void)buttons;  // Placeholder — extend VehicleData later
    (void)data;
    E46_LOG_DEBUG("MFL buttons: 0x%02X", buttons);
}

} // namespace kbus
} // namespace e46
