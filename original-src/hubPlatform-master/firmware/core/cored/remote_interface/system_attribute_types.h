#pragma once

namespace HubPlatform {
namespace CoreDRemoteInterface {

namespace SettingsFields {
// internal format usage !!!
constexpr const char LAT_NAME[] = "latitude";
constexpr const char LNG_NAME[] = "longitude";
constexpr const char ZONE_NAME[] = "zonename";
}  // namespace SettingsFields

struct GeoPosition {
    float latitude;
    float longitude;
};

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
