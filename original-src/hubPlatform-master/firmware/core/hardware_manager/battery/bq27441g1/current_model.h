#pragma once

#include <cstdint>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

enum class CurrentModel : uint8_t {
    AverageDischargeCurrentFromPreviousCycle = 0,
    PresentAverageDischargeCurrent = 1,
    AverageCurrent = 2,
    Current = 3,
    DesignCapacityDiv5 = 4,
    Reserved1 = 5,
    User_RatemA = 6,
    Reserved2 = 7
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
