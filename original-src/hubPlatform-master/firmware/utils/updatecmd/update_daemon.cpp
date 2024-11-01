#include "updatecmd/update_daemon.h"
#include "updatecmd/update_executor.h"
#include "updatecmd/command_executor.h"
#include "updatecmd/log.h"
#include "updatecmd/default_packages_installer.h"

#include "update_manager/resources/um_resources.h"
#include "system_utils/system.h"
#include "system_utils/environment.h"
#include "system_utils/process/forker.h"
#include "system_utils/filesystem.h"
#include "common_utils/cpp14.h"
#include "platform_configuration/platform_configuration.h"

#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>
#include <sstream>

namespace HubPlatform {
namespace UpdateCommand {

namespace {
enum class Action {
    FactoryReset,
    SoftReset,
    FirmwareRestart,
    FirmwareUpdate,
    LuupSwitch,
    BootloaderUpdate,
    RecoveryPartitionUpdate
};

std::string getUploadLogsCommand( const std::string& userString ) {
    std::stringstream ss;
    std::string logcmd = Environment::getBinariesFolder() + "/logcmd";

    ss << logcmd << " --idle";
    if( !userString.empty() )
        ss << " --add-to-name=" + userString;
    return ss.str();
}

std::string getStoreLogsCommand( const std::string& userString ) {
    std::stringstream ss;
    std::string logcmd = Environment::getBinariesFolder() + "/logcmd";

    ss << logcmd << " --store-to-postponed";
    if( !userString.empty() )
        ss << " --add-to-name=" + userString;
    return ss.str();
}

bool isActionSupported( Action action ) {
#if defined( __HUB_TYPE_VHUB__ )
    return action == Action::SoftReset || action == Action::FirmwareRestart;
#endif
    return true;
}

}  // namespace

int UpdateDaemon::run( int argc, char* argv[] ) {
    int exitResult = EXIT_SUCCESS;
    try {
        if( argc == 1 && !installDefaultPackages() )
            return EXIT_FAILURE;

        Action action = Action::FirmwareUpdate;
        for( auto i = 0; i < argc; ++i ) {
            if( strcmp( argv[ i ], "--daemon" ) == 0 ) {
                m_isDaemon = true;
            }
            else if( strcmp( argv[ i ], "--factory_reset" ) == 0 ) {
                action = Action::FactoryReset;
            }
            else if( strcmp( argv[ i ], "--soft_reset" ) == 0 ) {
                action = Action::SoftReset;
            }
            else if( strcmp( argv[ i ], "--restart_firmware" ) == 0 ) {
                action = Action::FirmwareRestart;
            }
            else if( strcmp( argv[ i ], "--switch_to_luup" ) == 0 ) {
                action = Action::LuupSwitch;
            }
            else if( strcmp( argv[ i ], "--update_bootloader" ) == 0 ) {
                action = Action::BootloaderUpdate;
            }
            else if( strcmp( argv[ i ], "--update_recovery_partition" ) == 0 ) {
                action = Action::RecoveryPartitionUpdate;
            }
        }

        if( !isActionSupported( action ) ) {
            throw std::runtime_error( "Action is not supported/implemented for this platform" );
        }

        if( m_isDaemon )
            Process::daemonize();

        switch( action ) {
        case Action::FactoryReset:
            if( !factoryReset() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        case Action::SoftReset:
            if( !softReset() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        case Action::FirmwareRestart:
            if( !restartFirmware() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        case Action::FirmwareUpdate:
            if( !updateFirmware() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        case Action::LuupSwitch:
            if( !switchToLuupFirmware() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        case Action::BootloaderUpdate:
            if( !updateBootloader() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        case Action::RecoveryPartitionUpdate:
            if( !updateRecoveryPartition() ) {
                exitResult = EXIT_FAILURE;
            }
            break;
        }
    }
    catch( const std::exception& ex ) {
        std::ofstream log( Environment::getLogsFolder() + "/update.log" );
        LOG_ERROR( ex.what() )
        exitResult = EXIT_FAILURE;
    }
    catch( ... ) {
        std::ofstream log( Environment::getLogsFolder() + "/update.log" );
        LOG_ERROR( "Something really bad happened." )
        exitResult = EXIT_FAILURE;
    }
    return exitResult;
}

bool UpdateDaemon::factoryReset() {
    using Status = CommandExecutor::Status;
    std::ofstream log( Environment::getLogsFolder() + "/factory-reset.log" );

    const auto userPartition = PlatformConfiguration::getPlatformConfiguration().getUserPartitionDeviceName();

    // TODO: it looks like a system part of a factory reset logic should be moved to system (some system script)
    std::vector< std::string > commands {
        /* prevent a kernel crash in a case of a corrupted fs on a user partition;
         * (during releasing resources owned by hubPlatform processes an exception within
         * an ext4 driver happens which causes crashing of these processes and, for some weird reason, the WHOLE kernel,
         * so to prevent this kernel panic we perform this fs check; this check won't return the user partition
         * to a "factory" state (to do this we format it) but will prevent this kernel panic) */
        std::string( "e2fsck -y -f " ) + userPartition + "; echo", /* ignore result of e2fsck cmd*/
            "/etc/init.d/firmware stop",
#if defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ )
            std::string( "umount -f " ) + userPartition + "; echo",
            std::string( "yes | mkfs.ext4 -F -F " ) + userPartition, "block mount",
#else
            "rm -rf " + Environment::getPersistentFolder() + "/*",
#endif
            "touch " + Environment::getPersistentFolder() + "/" + UpdateManager::Resources::ResetZwaveRequiredFileName,
#if defined( __HUB_TYPE_H2_SECURE__ )
            "touch " + Environment::getPersistentFolder() + "/" +
                UpdateManager::Resources::ResetBatteryRequiredFileName,
#endif
    };

    LOG_INFO( "Factory reset started" )

    CommandExecutor executor( &log );
    bool status = executor.run( commands, std::chrono::minutes( 15 ) ) == Status::Ok;

    LOG_INFO( "Storing logs ..." )
    executor.run( { getStoreLogsCommand( "factory-reset" ) }, std::chrono::minutes( 2 ) );

    if( status ) {
        status |= ( Status::Ok == executor.run( { "jffs2reset -y", "reboot -f" }, std::chrono::minutes( 2 ) ) );
    }

    if( !status ) {
        LOG_INFO( "Factory reset failed, starting firmware..." )
        System::getCommandOutput( "/etc/init.d/firmware start", true /*catchStderr*/ );
    }

    return status;
}

bool UpdateDaemon::softReset() {
    using Status = CommandExecutor::Status;
    std::ofstream log( Environment::getLogsFolder() + "/soft-reset.log" );

    // TODO: reset software
    std::vector< std::string > commands{ Environment::getBinariesFolder() + "/stop.sh",
                                         "touch " + Environment::getPersistentFolder() + "/" +
                                             UpdateManager::Resources::ResetZwaveRequiredFileName,
                                         Environment::getBinariesFolder() + "/migration_tool --reset_data" };

    LOG_INFO( "Soft reset started" )
    CommandExecutor executor( &log );
    auto status = executor.run( commands, std::chrono::minutes( 5 ) );
    LOG_INFO( "Soft reset finished with: " + std::string( ( status == Status::Ok ) ? "Success" : "Failure" ) );

    LOG_INFO( "Storing logs ..." )
    executor.run( { getStoreLogsCommand( "soft-reset" ) }, std::chrono::minutes( 2 ) );

    LOG_INFO( "Starting firmware ..." )
    System::getCommandOutput(
        ( Environment::getBinariesFolder() + "/start.sh -l " + Environment::getLogsFolder() ).c_str(),
        true /*catchStderr*/ );

    return status == Status::Ok;
}

bool UpdateDaemon::switchToLuupFirmware() {
    std::vector< std::string > commands{
        "/etc/init.d/firmware stop", "erfs set boot_mode em_downgrade_interruptable", "reboot -f"
    };

    for( auto& command : commands ) {
        auto result = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );

        if( result.second != 0 ) {
            System::getCommandOutput( "/etc/init.d/firmware start", true /*catchStderr*/ );
            return false;
        }
    }

    return true;
}

bool UpdateDaemon::updateBootloader() {
    using Status = CommandExecutor::Status;
    std::ofstream log( Environment::getLogsFolder() + "/bootloader_update.log" );

    std::vector< std::string > commands{ PlatformConfiguration::getPlatformConfiguration().getBootloaderUpdaterPath(),
                                         "reboot -f" };

    LOG_INFO( "Update bootloader started" )
    CommandExecutor executor( &log );
    bool status = ( Status::Ok == executor.run( commands, std::chrono::minutes( 1 ) ) );

    LOG_INFO( "Update bootloader finished with: " + std::string( ( status ) ? "Success" : "Failure" ) );
    return status;
}

bool UpdateDaemon::updateRecoveryPartition() {
    using Status = CommandExecutor::Status;
    std::ofstream log( Environment::getLogsFolder() + "/recovery_partition_update.log" );

    // TODO: EFWR-4028: Should I use full update here?
    auto recoveryImage = m_opkg.createFeedsUpdater( Opkg::IFeedsUpdater::Feeds::Common )->generateRecoveryImageUrl();
    std::vector< std::string > commands{ "sysupgrade -R " + recoveryImage, "reboot -f" };

    LOG_INFO( "Update recovery partition started" );
    CommandExecutor executor( &log );
    bool status = ( Status::Ok == executor.run( commands, std::chrono::minutes( 10 ) ) );

    LOG_INFO( "Update recovery partition finished with: " + std::string( ( status ) ? "Success" : "Failure" ) );
    return status;
}

bool UpdateDaemon::restartFirmware() {
    std::vector< std::string > commands{ Environment::getBinariesFolder() + "/stop.sh",
                                         Environment::getBinariesFolder() + "/start.sh -l " +
                                             Environment::getLogsFolder() };

    for( auto& command : commands ) {
        System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    }
    return true;
}

bool UpdateDaemon::updateFirmware() {
    auto pids = std::get< 0 >( System::getCommandOutput( "pidof updatecmd" ) );
    auto processesCount = std::count( pids.begin(), pids.end(), ' ' ) + 1;
    if( processesCount > 1 ) {
        // NOTE: updatecmd already running. Nothing to do.
        return true;
    }

    filesystem::createFolder( Environment::getLogsFolder() );

    UpdateExecutor executor( m_opkg.createPackagesManager(),
                             // TODO: EFWR-4028: Which feeds type should be selected here?
                             m_opkg.createFeedsUpdater( Opkg::IFeedsUpdater::Feeds::Common ),
                             Environment::getPersistentFolder() + "/" + UpdateManager::Resources::UpdateJournalFileName,
                             Environment::getLogsFolder() + "/update.log",
                             m_isDaemon );

    return executor.run();
}

bool UpdateDaemon::installDefaultPackages() {
    DefaultPackagesInstaller installer( m_opkg.createPackagesManager(),
                                        Environment::getLogsFolder() + "/install_default_packages.log" );
    return installer.run();
}

}  // namespace UpdateCommand
}  // namespace HubPlatform
