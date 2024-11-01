#pragma once

namespace HubPlatform {
namespace HardwareManager {
namespace Utils {

void setupNTPSyncIntervals();
void syncNTPTime( bool force = false );

}  // namespace Utils
}  // namespace HardwareManager
}  // namespace HubPlatform
