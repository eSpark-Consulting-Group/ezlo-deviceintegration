#pragma once

#include "hardware_manager/battery/battery_info.h"

namespace HubPlatform {
namespace HardwareManager {

// BatteryManager specialization should:
//   - setup everything needed for functioning in setUp() method
//   - setup whether specialization is hardware based
//   - when battery info change detected call corresponding callback
//   - stop all activity in tearDown() method

class BatteryManager {
public:
    virtual ~BatteryManager() = default;
    virtual void setUp( BatteryInfoChangedCallback callback ) = 0;
    virtual void tearDown() = 0;
    virtual void reset() = 0;
    virtual BatteryInfo getInfo() const = 0;
    virtual bool isHardwareBased() const = 0;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
