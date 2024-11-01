#include "reporter.h"
#include "json_utils/json_utils.h"
#include "config_reader/config_keys.h"
#include "config_reader/config_reader.h"
#include "system_utils/system.h"
#include <iostream>
#include <string>
#include <utility>

#if !defined( __HUB_TYPE_G150__ ) && !defined( __HUB_TYPE_VHUB__ )
extern "C" {
#include <libubus.h>
}
#include "ubus_interface/ubus_impl.h"
#else
#define UBUS_STATUS_OK 1
#endif

#include "common_utils/cpp14.h"
#include "base_utils/timeutils.h"

#include <unistd.h>
#include <sys/resource.h>
#include "system_utils/filesystem.h"

const std::string DefaultSupervisorUrl = "0.0.0.0";
constexpr int DefaultSupervisorPort = 18080;

Reporter::Reporter( std::string name )
: m_serviceName( std::move( name ) )
, m_postInterval( DefaultPostInterval ) {
    HubPlatform::ConfigReader configReader;

    try {
        if( configReader.hasValue( HubPlatform::ConfigKeys::MetricsReporterPostInterval ) ) {
            m_postInterval = configReader.getInt( HubPlatform::ConfigKeys::MetricsReporterPostInterval );
        }
    }
    catch( std::exception& ex ) {
        Log::error( cc::error( "Unable found config option " ) + cc::error( ": " ) + cc::attention( ex.what() ) );
    }

#if !defined( __HUB_TYPE_G150__ ) && !defined( __HUB_TYPE_VHUB__ )
    m_uBusObject = std::make_unique< HubPlatform::Ubus::UBusImpl >();
    m_uBusObject->connect( nullptr );
#endif
}

std::string Reporter::prepareReportData() {
    int pid = getpid();

    std::time_t curTime = std::time( nullptr );
    int64_t processUptimeTicks = getProcessUptimeTicks( std::to_string( pid ) );
    auto processUptimeSeconds = getProcessUptimeSeconds( processUptimeTicks );
    using namespace HubPlatform::filesystem;
    std::string fdPath = "/proc/" + std::to_string( pid ) + "/fd/";
    int openFdCount = 0;

    iterateFileSystemEntry( fdPath,
                            [&openFdCount]( const std::string& /*basePath*/, const std::string& /*entryName*/ ) {
                                ++openFdCount;
                                return true;
                            } );
    JsonObject params = JSON(
        "service" << JSON( m_serviceName << JSON(
                               "report_date" << curTime << "uptime" << processUptimeSeconds.count() << "service_status"
                                             << "healthy"
                                             << "cpu_time" << getCPUTime() << "pid" << pid << "mem_usage"
                                             << getProcessMemory() << "open_file_descriptors" << openFdCount ) ) );

    return params.dump();
}

void Reporter::sendReportData() {
    if( !m_uBusObject || !m_uBusObject->isConnected() ) {
        return;
    }

    std::string data = prepareReportData();

    bool postSuccess = postRequest( data );

    if( !postSuccess ) {
        LOGGER_TRACE_RAW( cc::trace( "Failed to send report message " ) );
    }
}

int Reporter::postInterval() const {
    return m_postInterval;
}

bool Reporter::postRequest( const std::string& data ) {
    bool ret = false;

    if( m_uBusObject && m_uBusObject->isConnected() ) {
        if( m_uBusObject->checkConnection( "watcher" ) != UBUS_STATUS_OK ) {
            LOGGER_DEBUG_RAW( cc::debug( "Watcher could not be found in ubus, re-connecting.." ) );
            if( !m_uBusObject->connect( nullptr ) ) {
                LOGGER_ERROR( "Couldn't reconnect to UBus, but continue to sending data..." );
            }
        }
        ret = m_uBusObject->sendCommand( "watcher", "send_status", "msg", data, nullptr );
        if( !ret ) {
            LOGGER_ERROR( "Failed to send data to supervisor: call to watcher.send_status has failed!" );
        }
    }

    return ret;
}

uint64_t Reporter::getCPUTime() {
    rusage resourceUsage = { 0 };

    getrusage( RUSAGE_SELF, &resourceUsage );

    return resourceUsage.ru_utime.tv_sec + resourceUsage.ru_stime.tv_sec;
}

JsonObject Reporter::getProcessMemory() {
    JsonObject memJson;
    std::string procStat;
    std::string line, column1, column2;
    std::array< std::pair< std::string, uint64_t >, 15 > memArr = { { { "VmSize", 0 },
                                                                      { "VmRSS", 0 },
                                                                      { "RssAnon", 0 },
                                                                      { "VmPeak", 0 },
                                                                      { "VmLck", 0 },
                                                                      { "VmPin", 0 },
                                                                      { "VmHWM", 0 },
                                                                      { "RssFile", 0 },
                                                                      { "RssShmem", 0 },
                                                                      { "VmData", 0 },
                                                                      { "VmStk", 0 },
                                                                      { "VmExe", 0 },
                                                                      { "VmLib", 0 },
                                                                      { "VmPTE", 0 },
                                                                      { "VmSwap", 0 } } };

    std::istringstream iss;
    std::stringstream procStatStream;
    const uint16_t byteUnit = 1024;

    procStat = getProcStatus();
    if( procStat.empty() ) {
        LOGGER_TRACE_RAW( cc::debug( "Could not read status file of process" ) );
        return 0;
    }
    procStatStream.str( procStat );

    while( std::getline( procStatStream, line ) ) {
        for( auto& memField : memArr ) {
            if( line.find( memField.first ) != std::string::npos ) {
                iss.str( line );
                iss >> column1 >> column2;
                memField.second = std::stoi( column2 ) * byteUnit;
                memJson[ memField.first ] = memField.second;
            }
        }
    }

    return memJson;
}

Reporter::~Reporter() {
    if( m_uBusObject ) {
        m_uBusObject->breakUloop();
    }
}

std::string Reporter::getProcStatus() {
    return HubPlatform::System::readFile( "/proc/self/status" );
}

int64_t Reporter::getProcessUptimeTicks( const std::string& pid ) {
    const std::string command = "awk '{print $22}' /proc/" + pid + "/stat";
    int processUptimeTicks = 0;

    auto callResult = HubPlatform::System::getCommandOutput( command.c_str() );
    auto& result = callResult.first;

    if( !result.empty() ) {
        try {
            processUptimeTicks = std::stoi( result );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( "Error parsing value with stoi from: " + result + ", error: " + ex.what() );
        }
    }
    else {
        LOGGER_ERROR( "Unable to find PID: " + pid );
    }
    return processUptimeTicks;
}

std::chrono::seconds Reporter::getProcessUptimeSeconds( int64_t uptimeTicks ) {
    auto uptimeSeconds = std::chrono::seconds( uptimeTicks / 100 );
    auto hubUptime = std::chrono::seconds( TimeUtils::getSystemUptime() );
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeSeconds = std::chrono::duration_cast< std::chrono::seconds >( currentTime.time_since_epoch() );
    auto hubUptimeEpoch = currentTimeSeconds - hubUptime;

    return currentTimeSeconds - hubUptimeEpoch - uptimeSeconds;
}
