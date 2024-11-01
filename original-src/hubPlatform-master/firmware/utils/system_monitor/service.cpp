#include "system_monitor/service.h"

#include "system_monitor/process.h"
#include "system_monitor/utils.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "base_utils/utils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <thread>
#include <atomic>
#include <condition_variable>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace SystemMonitor {

constexpr std::chrono::seconds RepeatInterval{ 30 };

static Service::ProcessInfoList defaultProcessInfoList() {
    Service::ProcessInfoList list = {
        { "sys_mon_ifconfig", "ifconfig" },
        { "sys_mon_ip_route", "ip route" },
        { "sys_mon_ip_monitor", "ip -ts monitor" },
        { "sys_mon_iw_dev", "iw dev" },
        { "sys_mon_iw_dev_wlan0_link", "iw dev wlan0 link" },
        { "sys_mon_netstat_established", "netstat -tunp" },
        { "sys_mon_netstat_listen", "netstat -tunpl" },
        { "sys_mon_df", "df" },
        { "sys_mon_ps", "ps -w" },
        { "sys_mon_free", "free" },
        { "sys_mon_wireless_config", "cat /etc/config/wireless" },
        { "sys_mon_logread_stream", "logread -f" },
    };
    return list;
}

Service& Service::serviceInstance() {
    static Service service( defaultProcessInfoList() );
    return service;
}

void Service::signalHandler( int signalNumber ) {
    serviceInstance().m_canRun = false;
}

Service::Service( ProcessInfoList processInfoList )
: m_processInfoList( std::move( processInfoList ) ) {
    ::signal( SIGINT, Service::signalHandler );
    ::signal( SIGTERM, Service::signalHandler );
}

void Service::setUp() {
    for( auto& processInfo : m_processInfoList ) {
        const auto& processLabel = processInfo.first;
        const auto& processCommand = processInfo.second;

        auto logPath = Environment::getLogsFolder() + "/" + processLabel + ".log";
        auto pidPath = Environment::getTempDataFolder() + "/" + processLabel + ".pid";

        if( filesystem::isFileExist( pidPath ) ) {
            auto previousPid = Utils::loadProcessPid( pidPath );
            if( previousPid > 0 && Utils::isProcessRunning( previousPid ) ) {
                LOGGER_INFO( cc::info( "Killing previous process pid: " ) + cc::num10( previousPid ) );
                Utils::sendTerminateSignal( previousPid );
            }
        }

        LOGGER_INFO_RAW( cc::info( "Start monitoring cmd: " ) + cc::note( processCommand ) + cc::info( ", log: " ) +
                         cc::note( logPath ) );
        m_processList.emplace_back(
            std::move( processCommand ), std::move( pidPath ), std::move( logPath ), RepeatInterval );
    }
}

void Service::tearDown() {
    auto& processList = serviceInstance().m_processList;

    std::vector< std::future< void > > futureList;
    futureList.reserve( processList.size() );

    for( auto& process : processList ) {
        futureList.emplace_back( std::async( [&process]() { return process.stop(); } ) );
    }

    for( auto& future : futureList ) {
        future.wait();
    }
}

int Service::run() {
    try {
        setUp();
        while( m_canRun ) {
            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        }
        tearDown();
        return 0;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception (" + std::string( typeid( e ).name() ) + "): " ) +
                          cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Unknown exception was caught" ) );
    }

    tearDown();
    return -1;
}

}  // namespace SystemMonitor
}  // namespace HubPlatform
