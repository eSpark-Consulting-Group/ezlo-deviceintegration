#pragma once

#include <cstdint>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

enum class PowerModel : uint8_t {
    AverageDischargePowerFromPreviousCycle = 0,
    PresentAverageDischargePower = 1,
    AverageCurrentxVoltage = 2,
    CurrentxVoltage = 3,
    DesignEnergyDiv5 = 4,
    Reserved1 = 5,
    User_RatemW = 6,
    Reserved2 = 7
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
