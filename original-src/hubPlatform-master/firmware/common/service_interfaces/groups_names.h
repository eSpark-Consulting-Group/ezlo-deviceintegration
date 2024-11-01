#pragma once

namespace HubPlatform {

// if we use the bus this constants should be global
namespace GroupsNames {
constexpr const char CORED[] = "cored";
constexpr const char EXTENSIONS_MANAGER[] = "extensions_manager";
constexpr const char EXTERNAL_CONTROLLERS[] = "external_controllers";
constexpr const char SCENE_MANAGER[] = "scenes_manager";
constexpr const char HARDWARE_MANAGER[] = "hardware_manager";
constexpr const char UPDATE_MANAGER[] = "update_manager";
constexpr const char CLOUD_DEVICES[] = "cloud_devices";
constexpr const char WEBRTC[] = "webrtc";
constexpr const char WEBRTC_V2[] = "mqtt.webrtc";
constexpr const char NOTIFICATIONS[] = "notifications";
constexpr const char VIDOO[] = "vidoo";
constexpr const char METRICS[] = "metrics";
constexpr const char BACKUP[] = "backupd";
// separate group for broadcasts only
constexpr const char HARDWARE_BROADCASTS[] = "hardware_manager_broadcasts";
constexpr const char UPDATE_MANAGER_BROADCASTS[] = "update_manager_broadcasts";

constexpr const char BROADCAST_GROUP[] = "broadcast_group";
constexpr const char INTERNAL_BROADCAST_GROUP[] = "internal_broadcast_group";
constexpr const char DEVICE_GROUP[] = "device_watchers_group";
constexpr const char EXTERNAL_DEVICE_GROUP[] = "external_device_watchers_group";
constexpr const char UI_GROUP[] = "ui_service";
constexpr const char LOGD_GROUP[] = "logd";
constexpr const char NETWORK[] = "network";
constexpr const char LED_COMMAND[] = "led_command";
constexpr const char SYSTEM_GROUP[] = "system_change_group";
constexpr const char ADDON_LUA[] = "addon_lua";
constexpr const char CLOUD_GROUP[] = "cloud_group";
constexpr const char CAMERAS_BROADCASTS[] = "cameras_broadcasts";
constexpr const char BACKUPD_BROADCASTS[] = "backupd_broadcasts";
}  // namespace GroupsNames

}  // namespace HubPlatform
