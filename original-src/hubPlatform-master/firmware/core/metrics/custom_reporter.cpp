#include "custom_reporter.h"
#include "resources/metrics_resources.h"
#include "json_utils/json_utils.h"
#include "system_utils/system.h"
#include "system_utils/filesystem.h"
#include <cstring>

namespace HubPlatform {
namespace Metrics {

CustomReporter::CustomReporter( const std::string& processName )
: Reporter( processName )
, m_serviceName( processName ) {}

// DONE: Replace with service name instead of pid: get pid for a name every time
std::string CustomReporter::getProcStatus() {
    m_pid = fetchPidByProcessName( m_serviceName );

    return HubPlatform::System::readFile( "/proc/" + std::to_string( m_pid ) + "/status" );
}

std::string CustomReporter::prepareReportData() {
    std::time_t curTime = std::time( nullptr );
    int64_t processUptimeTicks = getProcessUptimeTicks( std::to_string( m_pid ) );
    auto processUptimeSeconds = getProcessUptimeSeconds( processUptimeTicks );
    using namespace HubPlatform::filesystem;
    std::string fdPath = "/proc/" + std::to_string( m_pid ) + "/fd/";
    int openFdCount = 0;

    iterateFileSystemEntry( fdPath,
                            [&openFdCount]( const std::string& /*basePath*/, const std::string& /*entryName*/ ) {
                                ++openFdCount;
                                return true;
                            } );
    using namespace Resources::DataSender;

    JsonObject serviceReport =
        JSON( m_serviceName << JSON( Resources::DataSender::ServiceFields::reportDate
                                     << curTime << ServiceFields::uptime << processUptimeSeconds.count()
                                     << serviceStatus << healthy << ServiceFields::cpuTime << Reporter::getCPUTime()
                                     << ServiceFields::pid << m_pid << ServiceFields::memUsage
                                     << Reporter::getProcessMemory() << ServiceFields::customReport << true
                                     << ServiceFields::openFileDescriptors << openFdCount ) );
    JsonObject statusReport = JSON( StatusReportFields::service << serviceReport );
    return statusReport.dump();
}

pid_t CustomReporter::fetchPidByProcessName( const std::string& serviceName ) {
    if( serviceName.empty() )
        return 0;

    const std::string command = "pgrep " + serviceName;
    pid_t pid = 0;

    // Extract to system_utils
    auto callResult = System::getCommandOutput( command.c_str() );
    auto& result = callResult.first;

    if( !result.empty() ) {
        size_t pos = result.find_first_of( '\n' );
        if( pos != std::string::npos ) {
            result.erase( pos );
        }
        try {
            // TODO: Move it to report creations from constructor
            pid = std::stoi( result );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( "Error parsing pid with stoi from: " + result + ", error: " + ex.what() );
        }
    }
    else {
        LOGGER_ERROR( "Unable to find PID for the service: " + serviceName );
    }
    return pid;
}

}  // namespace Metrics
}  // namespace HubPlatform
