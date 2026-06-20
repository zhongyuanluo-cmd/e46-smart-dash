#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace e46 {
namespace common {

/**
 * Lock-free SPSC (Single-Producer, Single-Consumer) ring buffer.
 *
 * Thread safety: Exactly one thread may call push(), and exactly one
 * other thread may call pop(). Using push() or pop() from multiple
 * threads simultaneously is undefined behavior (data race).
 *
 * @tparam T        Element type (must be trivially copyable for correctness)
 * @tparam Capacity Total capacity; must be a power of 2 (enforced by static_assert)
 */
template<typename T, size_t Capacity>
class RingBuffer
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");

public:
    RingBuffer() = default;

    bool push(const T& item)
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) & mask_;

        if (next == tail_.load(std::memory_order_acquire)) {
            return false;  // Full
        }

        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    std::optional<T> pop()
    {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // Empty
        }

        T item = buffer_[tail];
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return item;
    }

    bool empty() const
    {
        return tail_.load(std::memory_order_acquire) ==
               head_.load(std::memory_order_acquire);
    }

    bool full() const
    {
        size_t head = head_.load(std::memory_order_relaxed);
        return ((head + 1) & mask_) == tail_.load(std::memory_order_acquire);
    }

    size_t capacity() const { return Capacity; }

    /**
     * Returns approximate element count. The value may be stale by the time
     * it is returned (producer or consumer may advance between loads).
     * Use for monitoring / debugging only; do not rely on it for control logic.
     */
    size_t size() const
    {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (head >= tail) return head - tail;
        return Capacity - tail + head;
    }

private:
    static constexpr size_t mask_ = Capacity - 1;
    T buffer_[Capacity]{};
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

} // namespace common
} // namespace e46
