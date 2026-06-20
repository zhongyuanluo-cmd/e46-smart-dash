#include "e46/common/types.h"
#include <time.h>

namespace e46 {
namespace common {

void VehicleData::mark_can_update()
{
    last_can_update_ms.store(now_ms(), std::memory_order_relaxed);
}

void VehicleData::mark_kbus_update()
{
    last_kbus_update_ms.store(now_ms(), std::memory_order_relaxed);
}

void VehicleData::mark_adc_update()
{
    last_adc_update_ms.store(now_ms(), std::memory_order_relaxed);
}

int64_t VehicleData::now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

} // namespace common
} // namespace e46
