#pragma once

namespace HubPlatform {
namespace CoreD {
namespace Resources {

namespace ModuleNames {

constexpr const char* const Core = "core";
constexpr const char* const HouseModes = "house_modes";
constexpr const char* const WalkTest = "walk_test";

}  // namespace ModuleNames

namespace DeviceCategories {
constexpr const char* const Siren = "siren";
}

namespace Settings {
constexpr const char* const LogsRotationInterval = "logs.rotation.interval";
}  // namespace Settings

namespace PendingUpdateFields {

constexpr const char FirmwareCfgModificationTime[] = "firmwareCfgModificationTime";
constexpr const char Process[] = "process";

}  // namespace PendingUpdateFields

}  // namespace Resources
}  // namespace CoreD
}  // namespace HubPlatform
