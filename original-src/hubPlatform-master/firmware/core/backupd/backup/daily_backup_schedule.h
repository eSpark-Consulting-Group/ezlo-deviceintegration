#pragma once

#include "service_interfaces/service.h"
#include "base_utils/native_timer.h"

class IDbClient;

namespace HubPlatform {
namespace BackupUpdateManager {
class DailyBackupScheduler {
public:
    DailyBackupScheduler( IDbClient& dbClient, NativeTimer& dailyBackupTimer, int backupOffset );
    virtual ~DailyBackupScheduler() = default;
    void setUp();
    std::chrono::seconds getPreviousBackupRunTime();
    void setPreviousBackupRunTime( std::chrono::time_point< std::chrono::system_clock > previousRunTime );
    std::chrono::seconds getBackupSchedule();
    void setBackupSchedule( std::chrono::seconds dailyRunTime );
    std::chrono::seconds processBackupOffsetTime( std::chrono::seconds hubUptime ) const;

private:
    IDbClient& m_dbClient;
    NativeTimer& m_dailyBackupTimer;
    int m_backupOffset;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform