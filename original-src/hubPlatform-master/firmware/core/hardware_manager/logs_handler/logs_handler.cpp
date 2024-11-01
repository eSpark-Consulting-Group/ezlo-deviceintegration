#include "hardware_manager/logs_handler/logs_handler.h"

#include <cstdlib>

#include "system_utils/mount_point.h"
#include "system_utils/filesystem.h"
#include "system_utils/environment.h"
#include "system_utils/system.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "service_interfaces/service.h"
#include "networkd/remote_interface/network_remote_interface.h"

#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "hardware_manager/hub_state/hub_state_indication_manager.h"
#include "hardware_manager/led/led.h"
#include "hardware_manager/diagnostics_dumper.h"

namespace HubPlatform {
namespace HardwareManager {

namespace {

static const std::string LogCmdPath = Environment::getBinariesFolder().append( "/logcmd" );
static const std::string StorageLogsPath = Environment::getPersistentFolder().append( "/logs" );
static const std::string MountPointPath{ "/mnt/logs" };

class DiagnosticsHandler {
public:
    DiagnosticsHandler( Led& powerLed )
    : m_powerLed( powerLed ) {}
    ~DiagnosticsHandler();
    void dump();

private:
    Led& m_powerLed;
    bool m_ok = false;
};

DiagnosticsHandler::~DiagnosticsHandler() {
#if( defined( __HUB_TYPE_G150__ ) )
    if( !m_ok ) {
        m_powerLed.turnBlinking( 100, 100 );
    }
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    m_powerLed.turnOn();
#endif
}

void DiagnosticsHandler::dump() {
#if( defined( __HUB_TYPE_G150__ ) )
    m_powerLed.turnBlinking( 250, 250 );
#endif
    DiagnosticsDumper diagnostics;
    m_ok = diagnostics.dump();
}

}  // namespace

LogsHandler::LogsHandler( Services::Service& service, HubStateIndicationManager& indicationManager, Led& powerLed )
: m_hostService( service )
, m_indicationManager( indicationManager )
, m_powerLed( powerLed ) {}

LogsHandler::~LogsHandler() = default;

bool LogsHandler::saveLogs() {
    DiagnosticsHandler diagnostics( m_powerLed );
    diagnostics.dump();

    if( saveToCloud() ) {
        m_indicationManager.setEventEffect( EffectType::FilesUploaded );
        return true;
    }

    if( saveToUsbStick() ) {
        m_indicationManager.setEventEffect( EffectType::Ok );
        return true;
    }

    if( saveToStorage() ) {
        m_indicationManager.setEventEffect( EffectType::Ok );
        return true;
    }

    m_indicationManager.setEventEffect( EffectType::Fail );
    return false;
}

bool LogsHandler::saveToCloud() {
    Network::RemoteInterface networkRemoteInterface( m_hostService );
    bool isInternetAvailable = networkRemoteInterface.isInternetAvailable();
    if( !isInternetAvailable ) {
        Log::warn( cc::warn( "Failed to save logs to cloud: internet is not available" ) );
        return false;
    }

    Log::info( cc::info( "Saving logs to cloud ..." ) );
    int result = std::system( LogCmdPath.c_str() );
    if( result != 0 ) {
        Log::warn( cc::warn( "Failed to save logs to cloud: logcmd return: " +
                             std::to_string( System::unpackExitCode( result ).exitCode ) ) );
        return false;
    }

    Log::info( cc::info( "Logs saved to cloud" ) );
    return true;
}

bool LogsHandler::saveToUsbStick() {
    try {
        auto mountPoint = filesystem::MountPoint::mount( "/dev/sda1", MountPointPath );

        Log::info( cc::info( "Saving logs to usb stick ..." ) );
        int result = std::system( ( LogCmdPath + " --store --output-dir=" + MountPointPath ).c_str() );
        if( result != 0 ) {
            Log::warn( cc::warn( "Failed to save logs to USB: logcmd return: " +
                                 std::to_string( System::unpackExitCode( result ).exitCode ) ) );
            return false;
        }

        Log::info( cc::info( "Logs saved to USB stick" ) );
        return true;
    }
    catch( filesystem::MountPointError& e ) {
        Log::warn( cc::warn( "Failed to mount USB: " + std::string( e.what() ) ) );
    }
    return false;
}

bool LogsHandler::saveToStorage() {
    Log::info( cc::info( "Storing logs to postponed dir" ) );
    int result = std::system( ( LogCmdPath + " --store-to-postponed" ).c_str() );
    if( result != 0 ) {
        Log::warn( cc::warn( "Failed to store logs : logcmd return: " +
                             std::to_string( System::unpackExitCode( result ).exitCode ) ) );
        return false;
    }

    Log::info( cc::info( "Logs stored to postponed dir" ) );
    return true;
}

}  // namespace HardwareManager
}  // namespace HubPlatform
