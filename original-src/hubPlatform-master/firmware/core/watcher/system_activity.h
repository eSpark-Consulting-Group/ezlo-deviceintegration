#pragma once
#include "common_utils/cpp14.h"
#include "ubus_interface/ubus_impl.h"
#include <vector>
#include <cstdint>

extern "C" {
#include <libubox/blobmsg_json.h>
#include <libubus.h>
}

namespace HubPlatform {

#define SI_LOAD_SHIFT 16

struct Memory {
    uint totalBytes = 0;
    uint freeBytes = 0;
    uint cachedBytes = 0;
    uint availableBytes = 0;
};

struct SysLoad {
    float load1 = 0;
    float load5 = 0;
    float load15 = 0;
};

struct TimeInfo {
    int32_t timezone = 0;
    std::string timezoneIdentifier;
    uint utctime = 0;
    uint localtime = 0;
    std::string timezoneOffset;
    uint uptime = 0;
    uint ntpLastSync = 0;
};

struct FileSystemInfo {
    std::string mountPoint;
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    uint64_t usedBytes = 0;
};

struct SystemInfo {
    Memory memInfo;
    SysLoad sysLoad;
    TimeInfo timeInfo;
    std::vector< FileSystemInfo > fileSystemInfo;
};

struct NetworkInfo {
    uint rxCount = 0;
    uint rxBytes = 0;
    uint rxError = 0;
    uint txCount = 0;
    uint txBytes = 0;
    uint txError = 0;
    uint uptime = 0;
    bool status = false;
    std::string interfaceName = "na";
    std::string deviceName = "na";
};

class SystemActivity {
public:
    SystemActivity();
    void getSystemReportData( std::string& systemReportData );
    void setInstance( SystemActivity* instance );
    blobmsg_policy* getMessagePolicy();

private:
    void collectLoad();
    void collectMemory();
    void collectTime();
    void collectFileSystem();
    void collectNetwork();
    void collectInterfaceStatus( NetworkInfo& netInf );
    void collectInterfaceNames( std::vector< NetworkInfo >& networkInfo );
    void clearSystemInfo();
    void fillFileSystemInfo( const std::string& mountPoint, FileSystemInfo& fileSystemInfo );
    uint getKillCount();
    void handleRebootStatus();
    void setRebootCount();
    void prepareSystemReportData( std::string& systemReportData );
    void collectSystemInfo();

private:
    SystemInfo m_sysInfo;
    std::vector< NetworkInfo > m_networkInfo;
    std::unique_ptr< Ubus::UBusInterface > m_uBusObject;
    const char* m_serviceName = "ha-watcher";
    const char* m_rebootFile = "/reboot_count";
    const char* mountsPath = "/proc/mounts";
    const char* vmstatPath = "/proc/vmstat";
    const char* ntpSyncFile = "ntp_last_sync";
    uint32_t m_hubRebootCount = 0;
};
}  // namespace HubPlatform
