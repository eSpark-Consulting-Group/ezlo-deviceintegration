#pragma once

namespace HubPlatform {
namespace Metrics {
namespace Resources {

namespace DataReceiver {
constexpr const char* uBusTarget = "watcher";
constexpr const char* uBusRequestMethod = "get_info";
}  // namespace DataReceiver

namespace DataSender {
constexpr const char* system = "System";

constexpr const char* free = "free";
constexpr const char* total = "total";
constexpr const char* cached = "cached";
constexpr const char* memory = "Memory";

constexpr const char* fileSystem = "FileSystem";
constexpr const char* totalSpace = "total_space";
constexpr const char* availableSpace = "available_space";

constexpr const char* load1 = "load_1";
constexpr const char* load5 = "load_5";
constexpr const char* load15 = "load_15";
constexpr const char* loadAverage = "LoadAverage";

constexpr const char* oomKill = "oom_kill";

constexpr const char* rebootCount = "reboot_count";

constexpr const char* modems = "modems";
constexpr const char* ssh_tunnels = "ssh_tunnels";
constexpr const char* ping_statistics = "ping_statistics";
constexpr const char* network = "Network";
constexpr const char* serviceNetwork = "network";
constexpr const char* interfaceStatus = "interface_status";
constexpr const char* rxBytes = "rx_bytes";
constexpr const char* rxCount = "rx_count";
constexpr const char* rxError = "rx_errors";
constexpr const char* txBytes = "tx_bytes";
constexpr const char* txCount = "tx_count";
constexpr const char* txError = "tx_errors";
constexpr const char* rssi = "rssi";
constexpr const char* frequencyBand = "frequency_band";
constexpr const char* signalQuality = "signal_quality";
constexpr const char* interfaceName = "interface_name";
constexpr const char* deviceName = "device_name";
constexpr const char* internetAvailability = "internet_availability";
constexpr const char* uiNetworkConfiguration = "ui_network_configuration";
constexpr const char* activeInterface = "active_interface";
constexpr const char* wifiAvailability = "wifi_availability";
constexpr const char* nonMobileInternetAvailability = "non_mobile_internet_availability";
constexpr const char* activeWifiDevice = "active_wifi_device";
constexpr const char* NmaConnectionCumulativeUptime = "cumulativeUptime";
constexpr const char* NmaConnectionUptime = "uptime";
constexpr const char* NmaReconnects = "reconnects";
constexpr const char* NmaReconnectsCumulative = "cumulativeReconnects";
constexpr const char* EthernetInterfaceUptime = "ethernet_interface_uptime";
constexpr const char* WifiStaInterfacesUptime = "wifi_sta_interface_uptime";
constexpr const char* WifiApInterfacesUptime = "wifi_ap_interface_uptime";
constexpr const char* MobileInterfacesUptime = "mobile_interface_uptime";

// namespace reportFields
namespace StatusReportFields {
constexpr const char* service = "service";
}  // namespace StatusReportFields

namespace TimeFields {
constexpr const char* time = "Time";
constexpr const char* utctime = "utctime";
constexpr const char* timezone = "timezone";
constexpr const char* timezoneIdentifier = "timezone_identifier";
constexpr const char* ntpLastSync = "ntp_last_sync";
constexpr const char* timezoneOffset = "timezone_offset";
}  // namespace TimeFields

// namespace ServiceFields
namespace ServiceFields {
constexpr const char* reportDate = "report_date";
constexpr const char* uptime = "uptime";
constexpr const char* pid = "pid";

constexpr const char* cpuTime = "cpu_time";
constexpr const char* memUsage = "mem_usage";
constexpr const char* respawnCount = "respawn_count";
constexpr const char* customReport = "custom_report";
constexpr const char* openFileDescriptors = "open_file_descriptors";
}  // namespace ServiceFields

constexpr const char* serviceStatus = "service_status";
constexpr const char* healthy = "healthy";
constexpr const char* stopped = "stopped";
constexpr const char* corrupt = "corrupt";

constexpr const char* uiService = "ui_service";
constexpr const char* nmaConnection = "nma_connection";
constexpr const char* zWaveCalibration = "zwave_calibration";
constexpr const char* hubErrors = "hub_errors";
constexpr const char* cloudCoordinatorURL = "cloud_coordinator_url";
constexpr const char* explicitCloudCoordinatorURL = "explicit_cloud_coordinator_url";
constexpr const char* nmaServerUrl = "nma_server_url";
constexpr const char* busConnectionStatus = "bus_connection_status";
constexpr const char* hubRegistrationStatus = "hub_registration_status";
constexpr const char* hubConnectingStatus = "hub_connecting_status";

constexpr const char* HttpServer = "HttpServer";
constexpr const char* Nma = "Nma";
constexpr const char* Zwave = "zwave";

namespace HttpServerFields {
constexpr const char* Received = "received";
constexpr const char* Success = "success";
constexpr const char* Errors = "errors";
constexpr const char* WrongAuth = "wrongAuth";
constexpr const char* TimedOut = "timedOut";
constexpr const char* LimitReached = "limitReached";
constexpr const char* RxBytes = "rxBytes";
constexpr const char* TxBytes = "txBytes";
constexpr const char* Unhandled = "unhandled";
constexpr const char* ParallelRequests = "parallelRequests";
}  // namespace HttpServerFields

namespace ZwaveCommandsStatisticsFields {
constexpr const char* ZwaveCommandsStatistics = "zwave_commands_statistics";
constexpr const char* CompletedCommands = "completed_commands";
constexpr const char* FailedCommands = "failed_commands";
constexpr const char* NodeId = "node_id";
constexpr const char* When = "when";
constexpr const char* Delay = "delay";
}  // namespace ZwaveCommandsStatisticsFields

}  // namespace DataSender

}  // namespace Resources
}  // namespace Metrics
}  // namespace HubPlatform
