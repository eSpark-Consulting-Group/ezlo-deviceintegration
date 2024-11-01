#pragma once

#include "base_utils/gpio.h"

#include <stdexcept>

namespace HubPlatform {
namespace HardwareManager {

class BatteryChargerException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class BatteryCharger final {
public:
    BatteryCharger();

    bool isCharging() const;
    bool enableCharger();
    bool disableCharger();

private:
    common::Gpio m_chargerEnablePin;
    common::Gpio m_chargerStatusPin;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
