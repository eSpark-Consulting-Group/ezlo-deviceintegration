namespace HubPlatform {
namespace Watcher {
namespace Resources {

namespace ServiceNames {
constexpr const char* ZWave = "zwave";
constexpr const char* UpdateManager = "update_manager";
}  // namespace ServiceNames

namespace ConfigKeys {
constexpr const char WatcherIssueHandlersUninterruptibleSleepCheckInterval[] =
    "watcher/issue_handlers/uninterruptible_sleep/check_interval";
constexpr const char WatcherIssueHandlersUninterruptibleSleepTimeout[] =
    "watcher/issue_handlers/uninterruptible_sleep/timeout";

constexpr const char WatcherIssueHandlersUnresponsiveServicesCheckInterval[] =
    "watcher/issue_handlers/unresponsive_services/check_interval";
constexpr const char WatcherIssueHandlersUnresponsiveServicesTimeout[] =
    "watcher/issue_handlers/unresponsive_services/timeout";
constexpr const char WatcherIssueHandlersUnresponsiveServicesServiceTimeout[] =
    "watcher/issue_handlers/unresponsive_services/service_timeout";

constexpr const char WatcherIssueHandlersZWaveNodeExpiredCheckInterval[] =
    "watcher/issue_handlers/zwave_node_expired/check_interval";
constexpr const char WatcherIssueHandlersZWaveNodeExpiredTimeout[] =
    "watcher/issue_handlers/zwave_node_expired/timeout";

constexpr const char WatcherIssueHandlersWifiBcdcErrorCheckInterval[] =
    "watcher/issue_handlers/wifi_bcdc_error/check_interval";

constexpr const char WatcherSettingsPeriodicReportInterval[] = "watcher/settings/periodic_report_interval";
}  // namespace ConfigKeys

namespace ServiceActivity {
constexpr const char* service = "service";
constexpr const char* list = "list";
constexpr const char* instances = "instances";
constexpr const char* instance1 = "instance1";
constexpr const char* pid = "pid";
constexpr const char* respawncount = "respawn_count";
constexpr const char* reportDate = "report_date";
constexpr const char* serviceStatus = "service_status";
constexpr const char* corrupt = "corrupt";
constexpr const char* stopped = "stopped";
constexpr const char* data = "data";
constexpr const char* serviceName = "service_name";
constexpr const char* zwaveNodeLastUpdate = "last_update";
constexpr const char* isWakeUp = "is_wakeup";
constexpr const char* wakeUpInterval = "wakeup_interval";
constexpr const char* alwaysOnDevicesInterval = "always_on_devices_interval";
constexpr const char* zwaveNodeUpdateTimes = "zwave_node_update_times";
constexpr const char* serviceRunningStatus = "running_status";
constexpr const char* notAvailable = "NA";
constexpr const char* serviceUptime = "uptime";
}  // namespace ServiceActivity

namespace SystemActivity {
constexpr const char* info = "info";
constexpr const char* system = "system";
constexpr const char* dump = "dump";

constexpr const char* uptime = "uptime";

constexpr const char* excludeSys = "/sys";
constexpr const char* excludeRom = "/rom";
constexpr const char* excludeMnt = "/mnt";
constexpr const char* excludeTmp = "/tmp";
constexpr const char* excludedev = "/dev";
constexpr const char* excludeProc = "/proc";
constexpr const char* excludeOverlay = "/overlay";

constexpr const char* load = "load";
constexpr const char* load1 = "load_1";
constexpr const char* load5 = "load_5";
constexpr const char* load15 = "load_15";

constexpr const char* memory = "memory";
constexpr const char* memTotal = "total";
constexpr const char* memFree = "free";
constexpr const char* cached = "cached";
constexpr const char* available = "available";
constexpr const char* membuffered = "buffered";

constexpr const char* utctime = "utctime";
constexpr const char* timezone = "timezone";
constexpr const char* timezoneIdentifier = "timezone_identifier";
constexpr const char* localTime = "localtime";
constexpr const char* ntpLastSync = "ntp_last_sync";
constexpr const char* timezoneOffset = "timezone_offset";

constexpr const char* fileSysTotalSpace = "total_space";
constexpr const char* fileSysUsedSpace = "used_space";
constexpr const char* fileSysAvailabledSpace = "available_space";

constexpr const char* networkRXCount = "rx_count";
constexpr const char* networkTXCount = "tx_count";
constexpr const char* networkRXError = "rx_errors";
constexpr const char* networkTXError = "tx_errors";
constexpr const char* networkRXBytes = "rx_bytes";
constexpr const char* networkTXBytes = "tx_bytes";

constexpr const char* networkRXPackets = "rx_packets";
constexpr const char* networkTXPackets = "tx_packets";

constexpr const char* networkInterfaceStatus = "interface_status";
constexpr const char* deviceName = "device_name";

constexpr const char* networkInterface = "network.interface";
constexpr const char* interface = "interface";
constexpr const char* networkUp = "up";
constexpr const char* networkUptime = "uptime";
constexpr const char* device = "device";

constexpr const char* networkDevice = "network.device";
constexpr const char* status = "status";
constexpr const char* name = "name";
constexpr const char* statistics = "statistics";
}  // namespace SystemActivity

namespace IssueHandlers {
const char* const WifiBcdcErrorPattern = "brcmf_proto_bcdc_msg failed";
}  // namespace IssueHandlers

}  // namespace Resources
}  // namespace Watcher
}  // namespace HubPlatform
