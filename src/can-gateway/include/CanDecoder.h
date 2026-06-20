#pragma once

#include <functional>
#include <unordered_map>
#include <linux/can.h>
#include "e46/common/types.h"

namespace e46 {
namespace can {

using CanFrameHandler = std::function<void(const can_frame&, common::VehicleData&)>;

class CanDecoder
{
public:
    CanDecoder();

    void decode(const can_frame& frame, common::VehicleData& data);

    void registerHandler(uint32_t arbid, CanFrameHandler handler);

private:
    std::unordered_map<uint32_t, CanFrameHandler> handlers_;
    uint64_t unknownCount_ = 0;

    void decodeEngineRpm(const can_frame& frame, common::VehicleData& data);
    void decodeEngineTemp(const can_frame& frame, common::VehicleData& data);
    void decodeWheelSpeeds(const can_frame& frame, common::VehicleData& data);
    void decodeSpeed(const can_frame& frame, common::VehicleData& data);
    void decodeBatteryVoltage(const can_frame& frame, common::VehicleData& data);
    void decodeIgnition(const can_frame& frame, common::VehicleData& data);
    void decodeTorqueBrake(const can_frame& frame, common::VehicleData& data);
};

} // namespace can
} // namespace e46
