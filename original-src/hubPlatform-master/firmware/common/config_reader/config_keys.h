#pragma once

namespace HubPlatform {

namespace ConfigKeys {

constexpr const char Serial[] = "firmware/dev/serial";
constexpr const char HubMac[] = "firmware/dev/mac";
constexpr const char fwVersion[] = "firmware/dev/fw_version";
constexpr const char Coordinator[] = "cloud/server/coordinator";
constexpr const char NmaUrlCacheOn[] = "cloud/server/server_url_cache";
constexpr const char CustomServices[] = "custom_services/services";

constexpr const char CloudTimeout[] = "cloud/server/timeout";
constexpr const char CloudReconnectPeriod[] = "cloud/server/reconnect_period";
constexpr const char NmaUrl[] = "cloud/server/url";
constexpr const char StopEmotionsOnShutdown[] = "firmware/general/stop_emotions_on_shutdown";
constexpr const char PluginStorageLimit[] = "firmware/general/plugin_storage_limit";
constexpr const char LogLevel[] = "firmware/general/log_level";
constexpr const char LogColor[] = "firmware/general/log_color";
constexpr const char LogIndent[] = "firmware/general/log_indent";
constexpr const char dbPathKey[] = "firmware/storage/db_path";
constexpr const char LogPathKey[] = "firmware/storage/log_path";
constexpr const char HouseModes[] = "house_modes/house_modes";
constexpr const char SwingerShutdownDefaults[] = "house_modes/swinger_shutdown/defaults";
constexpr const char HouseModesDefault[] = "house_modes/settings/default";
constexpr const char HouseModeSwitchToDelay[] = "house_modes/settings/switchToDelay";
constexpr const char HouseModeAlarmDelay[] = "house_modes/settings/alarmDelay";
constexpr const char HouseModeEntryDelay[] = "house_modes/settings/entryDelay";
constexpr const char HouseModeEntryDelayNormal[] = "house_modes/settings/entryDelay/normal";
constexpr const char HouseModeEntryDelayShort[] = "house_modes/settings/entryDelay/short";
constexpr const char HouseModeEntryDelayExtended[] = "house_modes/settings/entryDelay/extended";
constexpr const char HouseModeEntryDelayLongExtended[] = "house_modes/settings/entryDelay/longExtended";
constexpr const char HouseModeEntryDelayInstant[] = "house_modes/settings/entryDelay/instant";
constexpr const char HouseModeAbortWindowDefault[] = "house_modes/settings/abortWindow/default";
constexpr const char HouseModeAbortWindowMinimum[] = "house_modes/settings/abortWindow/minimum";
constexpr const char HouseModeAbortWindowMaximum[] = "house_modes/settings/abortWindow/maximum";
constexpr const char LuadScriptTimeout[] = "luad/limits/scriptTimeout";
constexpr const char LuadScriptMinTimeout[] = "luad/limits/scriptMinTimeout";
constexpr const char Settings[] = "settings/settings";
constexpr const char PreDefineItemGroups[] = "house_modes/predefined_groups/item_groups";
constexpr const char PreDefineDeviceGroups[] = "house_modes/predefined_groups/device_groups";
constexpr const char PreDefinePolicies[] = "policies/policyConfigs";

constexpr const char scriptRunTimeout[] = "backup/script/run_timeout_sec";
constexpr const char backupIntervalMin[] = "backup/daily/everyday_time_min";
constexpr const char backupOffsetTime[] = "backup/backup_scheduler/backup_offset";

constexpr const char CamerasRtspServerPort[] = "cameras/rtsp_server/port";
constexpr const char CamerasRtspServerPacketBufferSize[] = "cameras/rtsp_server/packet_buffer_size";
constexpr const char CamerasLocalStorageMaxSizeMb[] = "cameras/local_storage/max_size_mb";
constexpr const char CamerasRecordingBufferSize[] = "cameras/recording/packet_buffer_size";
constexpr const char CamerasPreRecordingBufferSize[] = "cameras/pre_recording/packet_buffer_size";
constexpr const char CamerasSnapshoterBufferSize[] = "cameras/snapshot/packet_buffer_size";
constexpr const char CamerasSnapshoterQueueLimit[] = "cameras/snapshot/queue_limit";
constexpr const char CamerasSnapshoterTimeout[] = "cameras/snapshot/timeout_sec";
constexpr const char CamerasSnapshotCacheTimeout[] = "cameras/snapshot/cache_timeout_sec";
constexpr const char CamerasStreamingMaxDelayMsec[] = "cameras/streaming/max_delay_msec";

constexpr const char MetricsCloudUrl[] = "metrics/cloud/url";
constexpr const char MetricsCloudPorts[] = "metrics/cloud/ports";
constexpr const char MetricsSendIntervalNetworkDefault[] = "metrics/send_interval/network/default";
constexpr const char MetricsSendIntervalNetworkAnomaly[] = "metrics/send_interval/network/anomaly";
constexpr const char MetricsSendIntervalCellularDefault[] = "metrics/send_interval/cellular/default";
constexpr const char MetricsSendIntervalCellularAnomaly[] = "metrics/send_interval/cellular/anomaly";
constexpr const char MetricsReporterPostInterval[] = "metrics/reporter/post_interval";
constexpr const char MetricsReporterGetInterval[] = "metrics/reporter/get_interval";
constexpr const char MetricsReporterCustomServices[] = "metrics/reporter/custom_services";
constexpr const char MetricsReporterServerCrt[] = "metrics/certs/serverCrtFilePath";
constexpr const char MetricsReporterClientCrt[] = "metrics/certs/clientCrtFilePath";
constexpr const char MetricsReporterClientKey[] = "metrics/certs/clientKeyFilePath";

constexpr const char LogReaderStdOutState[] = "logreader/stdout/state";
constexpr const char LogReaderFileState[] = "logreader/file/state";
constexpr const char LogReaderFileLogFilePath[] = "logreader/file/log_file_path";
constexpr const char LogReaderUbusState[] = "logreader/ubus/state";
constexpr const char LogReaderUbusServerName[] = "logreader/ubus/server_name";
constexpr const char LogReaderUbusCallbackName[] = "logreader/ubus/callback_name";

constexpr const char WalkTestExtendedPeriod[] = "walk_test/settings/period";

constexpr const char PreDefineContactIds[] = "protect/CMSSignalAlias";
constexpr const char PreDefineSecurityConfigs[] = "protect/securityConfigs";

}  // namespace ConfigKeys
}  // namespace HubPlatform
