#include "backup_script_manager.h"

#include "system_utils/environment.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"

#include <thread>

namespace HubPlatform {
namespace BackupUpdateManager {

ScriptManagerBase::ScriptManagerBase( std::chrono::seconds timeoutSec )
: m_timeoutSec( timeoutSec ) {}

void ScriptManagerBase::setScriptRunTimeout( std::chrono::seconds timeoutSec ) {
    m_timeoutSec = timeoutSec;
}

ScriptManager::ScriptManager()
: ScriptManagerBase() {}

ScriptManager::ScriptManager( std::chrono::seconds timeoutSec )
: ScriptManagerBase( timeoutSec ) {}

void ScriptManager::startDaemon( const std::string daemonName, const std::vector< std::string > args ) {
    const std::string daemonServicePath = Environment::getBinariesFolder() + "/" + daemonName;
    const std::string daemonAddonPath = Environment::getBinariesFolder() + "/addons/" + daemonName;

    std::string daemonBinaryPath;

    if( filesystem::isExist( daemonServicePath ) ) {
        daemonBinaryPath = daemonServicePath;
    }
    else if( filesystem::isExist( daemonAddonPath ) ) {
        daemonBinaryPath = daemonAddonPath;
    }
    else {
        throw ScriptManagerError( "Daemon binary " + daemonName + " does not exist" );
    }

    auto process = System::getCommandOutput( ( "pgrep -f " + daemonBinaryPath ).c_str(), true /*catchStderr*/ );
    std::string pid = process.first;
    if( !pid.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "Daemon " ) + cc::note( daemonBinaryPath ) +
                          cc::debug( " already running with pid: " ) + cc::note( pid ) );
        return;
    }

    std::stringstream command;
    command << "( " << daemonBinaryPath;
    for( auto& arg : args )
        command << " " << arg;

    command << " --logs=" << Environment::getLogsFolder() << " >> "
            << Environment::getLogsFolder() + "/" + daemonName + ".log 2>&1";
    command << " )&";

    LOGGER_DEBUG_RAW( "Starting daemon: " + daemonName );
    std::system( command.str().c_str() );
}

void ScriptManager::startDaemons() {
    const std::string command = Environment::getBinariesFolder() + "/start.sh -l " + Environment::getLogsFolder() +
                                " >> " + Environment::getLogsFolder() + "/ha-restart.log 2>&1";
    std::system( command.c_str() );
    // NOTE: wait some time until all services will be started and connected to spread
    std::this_thread::sleep_for( std::chrono::seconds( 20 ) );
}

void ScriptManager::stopDaemons() {
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    const std::string stopScript = Environment::getBinariesFolder() + "/stop.sh --backup >> " +
                                   Environment::getLogsFolder() + "/ha-restart.log 2>&1";
    std::system( stopScript.c_str() );
}

int ScriptManager::restoreBackupCommand( const std::string& binaryName, const std::string& targetDirectory ) {
    auto filePos = binaryName.find_last_of( '/' );
    auto logName = filePos == std::string::npos ? binaryName : binaryName.substr( filePos + 1 );

    const std::string restoreCommand = getTimeoutCommand() + Environment::getBinariesFolder() + "/" + binaryName +
                                       " --restore_backup=" + targetDirectory +
                                       " --logs=" + Environment::getLogsFolder() + " >> " +
                                       Environment::getLogsFolder() + "/" + logName + ".log 2>&1";
    LOGGER_TRACE_RAW( cc::trace( "Running: " ) + cc::note( restoreCommand ) );
    return std::system( restoreCommand.c_str() );
}

bool ScriptManager::migrateData() {
    const std::string migrateCommand =
        Environment::getBinariesFolder() + "/migration_tool >> " + Environment::getLogsFolder() + "/migration.log 2>&1";
    return std::system( migrateCommand.c_str() ) == 0;
}

void ScriptManager::resetData() {
    system( ( HubPlatform::Environment::getBinariesFolder() + "/updatecmd --daemon --soft_reset" ).c_str() );
    std::this_thread::sleep_for( std::chrono::seconds( 60 ) );
}

std::string ScriptManager::getTimeoutCommand() {
    return
#if defined( __HUB_TYPE_G150__ )
        "";
#else
        "timeout --signal=SIGKILL " + std::to_string( m_timeoutSec.count() ) + "s ";
#endif
}

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
