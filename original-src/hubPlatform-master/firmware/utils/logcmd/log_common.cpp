#include "log_common.h"

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <thread>

#include "common_utils/firmware_info_helper.h"
#include "common_utils/hub_info_helper.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#include "logcmd/filesystem/filesystem.h"
#include "logcmd/log_exceptions.h"
#include "system_utils/environment.h"

namespace HubPlatform {
namespace LogCmd {

std::string getHubSerial() {
    return HubInfoHelper::getSerial();
}

std::string generateTimeString( std::time_t timestamp ) {
    std::time_t time = timestamp != 0 ? timestamp : std::time( nullptr );
    std::tm* tm = std::localtime( &time );
    if( tm ) {
        std::vector< char > buffer( 64, 0 );
        strftime( buffer.data(), buffer.size(), "%Y.%m.%d_%H.%M.%S_%z", tm );
        return std::string( buffer.data() );
    }
    return {};
}

/** Produce log archive name in form:
 *      - hub serial _ archive creation date _ fw version _ user string _logs.tgz
 *  Order has been changed to make files sortable:
 *      - 92xxxxxx_datexxxx_verxxxx_user_logs.tgz
 * */
std::string generateArchiveName( const std::string& userString, std::time_t timestamp ) {
    auto deviceSerial = getHubSerial();
    auto firmwareVersion = FirmwareInfoHelper::getFirmwareVersionString();

    std::stringstream ss;
    ss << deviceSerial;

    auto date = generateTimeString( timestamp );
    if( !date.empty() )
        ss << "_date-" << date;

    ss << "_ver-" << firmwareVersion;

    if( !userString.empty() )
        ss << "_" << userString;

    ss << "_logs.tgz";
    return ss.str();
}

std::array< std::string, 5 > generateCronSchedule( size_t interval, size_t seed ) {
    const size_t minutesInHour = 60;
    const size_t hoursInDay = 24;

    std::string minutes = std::to_string( seed % minutesInHour );

    std::string hours;
    if( interval <= 1 ) {
        hours = "*/1";
    }
    else if( interval > 12 ) {
        LOGGER_WARN_RAW( cc::warn( "Intervals > 12 will be triggered once per day at fixed time" ) );
        hours = std::to_string( ( interval + seed ) % hoursInDay );
    }
    else if( interval > 1 ) {
        const size_t intervalsInDay = hoursInDay / interval;

        std::vector< size_t > hoursList;
        hoursList.reserve( intervalsInDay );

        for( size_t i = 0; i < intervalsInDay; ++i ) {
            size_t intervalTime = i * interval;
            intervalTime = ( intervalTime + seed ) % hoursInDay;
            hoursList.emplace_back( intervalTime );
        }

        std::sort( hoursList.begin(), hoursList.end() );
        for( auto& time : hoursList )
            hours += std::to_string( time ) + ",";
        hours.pop_back();
    }

    std::array< std::string, 5 > schedule;
    schedule.fill( "*" );
    schedule[ 0 ] = minutes;
    schedule[ 1 ] = hours;
    return schedule;
}

void updateSystemLogs( const std::string& logDirectory ) {
    const std::string headerCmd = "echo \"==================== $(date +%Y.%m.%d_%H.%M) ====================\"";

    Filesystem filesystem;
    if( !filesystem.isDirectoryExist( logDirectory ) )
        throw LogDirectoryDoesNotExistError( "Log directory doesn't exist" );

    auto fullPath = [&logDirectory]( const std::string& logName ) {
        const auto auxSlash = ( logDirectory.back() != '/' ) ? "/" : "";
        return logDirectory + auxSlash + logName;
    };

    auto updateLog = [&fullPath, &headerCmd](
                         const std::string& logName, const std::string& cmd, bool append = false ) {
        auto logPath = fullPath( logName );
        const char* const appendWriteOp = " 2>&1 >> ";

        std::string writeOp = append ? appendWriteOp : " > ";
        std::system( ( headerCmd + writeOp + logPath ).c_str() );
        std::system( ( cmd + appendWriteOp + logPath ).c_str() );
    };

    updateLog( "logread.log", "logread" );
    updateLog( "storage.log", "df -h" );
    updateLog( "top.log", "top -b -n 1" );
    updateLog( "ps-d-state.log", "ps | awk '$4 ~ /D/  { print $0 }'" );
    updateLog( "free.log", "free" );
    updateLog( "network_config.log", "cat /etc/config/network" );
    updateLog( "wireless_config.log", "sed '/option key/d' /etc/config/wireless" );
    updateLog( "resolv.conf.auto.log", "cat /tmp/resolv.conf.auto" );

    updateLog( "ifconfig.log", "timeout --preserve-status -s 9 10s ifconfig" );
    updateLog( "route.log", "echo -e \"$(route -A inet)\\n\\n\\n$(route -A inet6)\"" );

    std::string firmwareStatusScript = Environment::getFirmwareRootFolder() + "/scripts/firmware_status.sh";
    // Pass SHELL env variable because even if the ash is called, SHELL variable points to the "sh" shell when called
    // from the system(). The firmware_status relies on bash or ash shell functions, can't operate in "sh" shell.
    if( Environment::isPhysicalHub() ) {
        updateLog( "firmware_status.log", "SHELL=/bin/ash /bin/ash -c " + firmwareStatusScript );
        updateLog( "firmware_watcher_get_info.log", "ubus call watcher get_info" );

        updateLog( "firmware_folders.log", "ls -lA /home/cores" );
        updateLog( "firmware_folders.log", "du -hcd1 " + Environment::getPendingLogsFolder(), true );
        updateLog( "firmware_folders.log", "ls -lA " + Environment::getPendingLogsFolder(), true );
        updateLog( "firmware_folders.log", "du -hcd1 " + Environment::getLogsFolder(), true );
        updateLog( "firmware_folders.log", "ls -lA " + Environment::getLogsFolder(), true );
        updateLog( "firmware_folders.log", "du -hcd1 " + Environment::getMediaFolder(), true );
        updateLog( "firmware_folders.log", "ls -laR " + Environment::getMediaFolder(), true );
        updateLog( "firmware_folders.log", "du -hcd1 " + Environment::getSnapshotsFolder(), true );
        updateLog( "firmware_folders.log", "ls -laR " + Environment::getSnapshotsFolder(), true );
    }
    else if( Environment::isLogicalHub() ) {
        updateLog( "firmware_status.log", "SHELL=/bin/bash /bin/bash -c " + firmwareStatusScript );
    }

    if( Environment::isPhysicalHub() ) {
        const bool append = true;
        updateLog( "wifi_status.log", "echo 'wlan0 dev status'" );
        updateLog( "wifi_status.log", "iw dev wlan0 info", append );
        updateLog( "wifi_status.log", "echo 'wlan0 dev link'", append );
        updateLog( "wifi_status.log", "iw dev wlan0 link", append );
        updateLog( "wifi_status.log", "echo 'Dump network.interface statuses'", append );
        updateLog( "wifi_status.log", "ubus call network.interface dump", append );
        updateLog( "wifi_status.log", "echo 'ip neighbor'", append );
        updateLog( "wifi_status.log", "ip neighbor", append );
        updateLog( "wifi_status.log", "echo 'ip addr'", append );
        updateLog( "wifi_status.log", "ip addr", append );
        updateLog( "wifi_status.log", "echo 'ip route'", append );
        updateLog( "wifi_status.log", "ip route", append );

        updateLog( "netstat.log", "echo 'netstat -tu'" );
        updateLog( "netstat.log", "netstat -tunap", append );
    }
}

bool waitForCondition( const std::function< bool() >& condition,
                       const std::chrono::milliseconds& timeout,
                       const std::chrono::milliseconds& checkInterval ) {
    const auto start = std::chrono::steady_clock::now();
    while( std::chrono::steady_clock::now() - start < timeout ) {
        if( condition() )
            return true;
        std::this_thread::sleep_for( checkInterval );
    }
    return false;
}

}  // namespace LogCmd
}  // namespace HubPlatform
