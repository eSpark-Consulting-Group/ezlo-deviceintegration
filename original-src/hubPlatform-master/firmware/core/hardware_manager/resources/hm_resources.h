#pragma once

#include <chrono>

namespace HubPlatform {
namespace HardwareManager {
namespace Resources {

namespace Settings {
extern const char* const BatteryPeriodSetting;
}

#if defined( __HUB_TYPE_H2_SECURE__ )
namespace SpeakerButton {
constexpr const char* speakerButtonPath = "/sys/class/gpio/gpio67/value";
namespace Reboot {
constexpr const std::chrono::seconds minThreshold = std::chrono::seconds( 10 );
constexpr const std::chrono::seconds maxThreshold = std::chrono::seconds( 15 );
}  // namespace Reboot
}  // namespace SpeakerButton
#endif

extern const char* const LedI2CBus;
extern const char* const BatteryI2CBus;

}  // namespace Resources
}  // namespace HardwareManager
}  // namespace HubPlatform
