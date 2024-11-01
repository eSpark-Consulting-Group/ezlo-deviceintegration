#pragma once

namespace HubPlatform {
namespace UpdateManager {
namespace Resources {

constexpr const char UpdateJournalFileName[] = ".update.journal";
constexpr const char LatestVersion[] = "latest";
constexpr const char ResetZwaveRequiredFileName[] = ".zwave_reset_required";
#if defined( __HUB_TYPE_H2_SECURE__ )
constexpr const char ResetBatteryRequiredFileName[] = ".battery_reset_required";
#endif

namespace DBTableNames {

constexpr const char UnavailablePlugins[] = "temp.db.firmware.update_manager.unavailablePlugins";

}  // namespace DBTableNames

namespace PackageNames {
constexpr const char Firmware[] = "hub-platform-firmware";
constexpr const char System[] = "hub-platform-system";
constexpr const char AddonPrefix[] = "firmware-addons-";
constexpr const char PluginPrefix[] = "firmware-plugins-";
// custom packages not equal to custom plugins: custom plugins are not packages, but tgz archives, not managed by opkg.
constexpr const char CustomPackagePrefix[] = "firmware-custom-";
}  // namespace PackageNames

namespace Commands {
constexpr const char Upgrade[] = "upgrade";
constexpr const char Sysupgrade[] = "sysupgrade";
constexpr const char Install[] = "install";
constexpr const char Uninstall[] = "uninstall";
}  // namespace Commands

namespace Statuses {
constexpr const char Scheduled[] = "scheduled";
constexpr const char Failed[] = "failed!!!";
constexpr const char Executing[] = "executing";
constexpr const char Completed[] = "completed";
}  // namespace Statuses

namespace CacheDirectories {
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ ) || defined( __HUB_TYPE_G150__ )
constexpr const char Update[] = "";
constexpr const char SystemImage[] = "";
constexpr const char Packages[] = "";
constexpr const char OpkgLists[] = "";
constexpr const char Distfeeds[] = "";
#else
constexpr const char Update[] = "/home/data/update";
constexpr const char SystemImage[] = "/home/data/update";
constexpr const char Packages[] = "/home/data/update/packages";
constexpr const char OpkgLists[] = "/home/data/update/opkg-lists";
constexpr const char Distfeeds[] = "/home/data/update";
#endif
}  // namespace CacheDirectories

namespace PendingUpdateFields {

constexpr const char PendingUpdate[] = "pending_update";
constexpr const char Started[] = "started";
constexpr const char Process[] = "process";
constexpr const char Finished[] = "finished";

}  // namespace PendingUpdateFields

}  // namespace Resources
}  // namespace UpdateManager
}  // namespace HubPlatform
