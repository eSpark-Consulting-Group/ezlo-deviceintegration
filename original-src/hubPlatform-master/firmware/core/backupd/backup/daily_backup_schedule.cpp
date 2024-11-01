#include "daily_backup_schedule.h"
#include "backupd/resources/backup_resources.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "base_utils/timeutils.h"

namespace HubPlatform {
namespace BackupUpdateManager {

DailyBackupScheduler::DailyBackupScheduler( IDbClient& dbClient, NativeTimer& dailyBackupTimer, int backupOffset )
: m_dbClient( dbClient )
, m_dailyBackupTimer( dailyBackupTimer )
, m_backupOffset( backupOffset ) {}

void DailyBackupScheduler::setUp() {
    std::chrono::seconds intervalTimeoutSeconds( 24 * 60 * 60 );

    auto dailyBackupRunTime = getBackupSchedule();
    auto currentTime = std::chrono::system_clock::now();
    auto dailyRunTime = std::chrono::seconds(
        std::chrono::duration_cast< std::chrono::seconds >( currentTime.time_since_epoch() ).count() %
        intervalTimeoutSeconds.count() );

    if( dailyBackupRunTime.count() == 0 ) {
        setBackupSchedule( dailyRunTime );
    }

    auto previousBackupRunTime = getPreviousBackupRunTime();
    auto timeDifference = std::chrono::duration_cast< std::chrono::seconds >( currentTime.time_since_epoch() ).count() -
                          previousBackupRunTime.count();

    if( timeDifference >= 86400 || dailyBackupRunTime.count() == 0 ) {
        std::chrono::seconds startTimeoutSeconds =
            processBackupOffsetTime( std::chrono::seconds( TimeUtils::getSystemUptime() ) );
        m_dailyBackupTimer.start( startTimeoutSeconds, intervalTimeoutSeconds + startTimeoutSeconds );
    }
    else if( dailyRunTime.count() < dailyBackupRunTime.count() ) {
        auto startTimeoutSeconds = std::chrono::seconds( dailyBackupRunTime.count() - dailyRunTime.count() );
        m_dailyBackupTimer.start( startTimeoutSeconds, intervalTimeoutSeconds + startTimeoutSeconds );
    }
    else {
        auto startTimeoutSeconds =
            intervalTimeoutSeconds - std::chrono::seconds( dailyRunTime.count() - dailyBackupRunTime.count() );
        m_dailyBackupTimer.start( startTimeoutSeconds, intervalTimeoutSeconds + startTimeoutSeconds );
    }
}

std::chrono::seconds DailyBackupScheduler::getPreviousBackupRunTime() {
    using namespace Resources;
    auto dailyBackupRecord =
        m_dbClient.findOne( DBNames::BackupDbName, JSON( db::ID_FIELD_NAME << BackupSchedule::IdName ), nullptr );

    int64_t previousRunTime = json_get_int64( dailyBackupRecord, BackupSchedule::PreviousBackupRunTime, 0 );
    return std::chrono::seconds( previousRunTime );
}

void DailyBackupScheduler::setPreviousBackupRunTime(
    std::chrono::time_point< std::chrono::system_clock > previousRunTime ) {
    auto previousRuntimeSeconds =
        std::chrono::duration_cast< std::chrono::seconds >( previousRunTime.time_since_epoch() );
    using namespace Resources;
    auto dailyBackupRecord =
        m_dbClient.findOne( DBNames::BackupDbName, JSON( db::ID_FIELD_NAME << BackupSchedule::IdName ), nullptr );

    if( !dailyBackupRecord.empty() ) {
        dailyBackupRecord[ BackupSchedule::PreviousBackupRunTime ] = previousRuntimeSeconds.count();
        m_dbClient.update( Resources::DBNames::BackupDbName, JSON_EMPTY_OBJ, dailyBackupRecord );
    }
    else {
        m_dbClient.insert( DBNames::BackupDbName,
                           JSON( db::ID_FIELD_NAME << BackupSchedule::IdName << BackupSchedule::PreviousBackupRunTime
                                                   << previousRuntimeSeconds.count() ) );
    }
}

std::chrono::seconds DailyBackupScheduler::getBackupSchedule() {
    using namespace Resources;
    if( m_dbClient.exist( DBNames::BackupDbName ) ) {
        auto dailyBackupRecord =
            m_dbClient.findOne( DBNames::BackupDbName, JSON( db::ID_FIELD_NAME << BackupSchedule::IdName ), nullptr );

        int64_t dailyRunTime = json_get_int64( dailyBackupRecord, BackupSchedule::DailyBackupRunTime, 0 );
        return std::chrono::seconds( dailyRunTime );
    }
    return std::chrono::seconds( 0 );
}

void DailyBackupScheduler::setBackupSchedule( std::chrono::seconds dailyRunTime ) {
    using namespace Resources;
    auto dailyBackupRecord =
        m_dbClient.findOne( DBNames::BackupDbName, JSON( db::ID_FIELD_NAME << BackupSchedule::IdName ), nullptr );

    if( !dailyBackupRecord.empty() ) {
        dailyBackupRecord[ BackupSchedule::DailyBackupRunTime ] = dailyRunTime.count();
        m_dbClient.update( Resources::DBNames::BackupDbName, JSON_EMPTY_OBJ, dailyBackupRecord );
    }
    else {
        m_dbClient.insert( DBNames::BackupDbName,
                           JSON( db::ID_FIELD_NAME << BackupSchedule::IdName << BackupSchedule::DailyBackupRunTime
                                                   << dailyRunTime.count() ) );
    }
}

std::chrono::seconds DailyBackupScheduler::processBackupOffsetTime( std::chrono::seconds hubUptime ) const {
    std::chrono::seconds startTimeoutSeconds;

    if( hubUptime >= std::chrono::seconds( m_backupOffset ) ) {
        startTimeoutSeconds = std::chrono::seconds( 30 );
    }
    else {
        startTimeoutSeconds = std::chrono::seconds( m_backupOffset ) - std::chrono::seconds( hubUptime.count() );
    }
    return startTimeoutSeconds;
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
