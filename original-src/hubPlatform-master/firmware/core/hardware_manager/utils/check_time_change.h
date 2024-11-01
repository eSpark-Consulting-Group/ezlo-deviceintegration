#pragma once

namespace HubPlatform {
namespace HardwareManager {
namespace Utils {

extern const bool HasTimerExitOnClose;
int checkTimeChange();
long getSystemUptime();

}  // namespace Utils
}  // namespace HardwareManager
}  // namespace HubPlatform