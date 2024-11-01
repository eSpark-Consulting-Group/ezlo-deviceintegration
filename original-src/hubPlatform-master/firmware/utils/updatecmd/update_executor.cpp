#include "updatecmd/update_executor.h"
#include "updatecmd/command_executor.h"
#include "updatecmd/log.h"

#include "update_manager/resources/um_resources.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "system_utils/environment.h"

#include "tiny/tiny_consolecolors.h"

#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>
#include <stdlib.h>

namespace HubPlatform {
namespace UpdateCommand {

namespace {
constexpr const int RetriesCount = 3;
constexpr const std::chrono::seconds RetriesDelay( 10 );

void sendLogs( std::ostream* logStream ) {
    std::string logcmd = "/opt/firmware/bin/logcmd";
    auto uploadCommand = logcmd + " --idle --add-to-name=update";

    auto storeCommand = logcmd + " --store --idle --add-to-name=update";
    storeCommand +=
        " --output-dir=" + HubPlatform::Environment::getPostponedLogsFolder() + " --create-dir --rotate-archives=30";

    CommandExecutor executor( logStream );

    auto status = executor.run( { uploadCommand.c_str() }, std::chrono::minutes( 1 ) );
    if( status != CommandExecutor::Status::Ok ) {
        executor.run( { storeCommand.c_str() }, std::chrono::minutes( 1 ) );
    }
}

}  // namespace

UpdateExecutor::UpdateExecutor( std::unique_ptr< Opkg::IPackagesManager > packagesManager,
                                std::unique_ptr< Opkg::IFeedsUpdater > feedsUpdater,
                                std::string journalFileName,
                                const std::string& logFileName,
                                bool isDaemon )
: m_packagesManager( std::move( packagesManager ) )
, m_feedsUpdater( std::move( feedsUpdater ) )
, m_journalFileName( std::move( journalFileName ) )
, m_log( logFileName, std::ios_base::app )
, m_availablePackagesOutdated( false )
, m_isDaemon( isDaemon ) {}

bool UpdateExecutor::run( bool singleStep ) {
    using namespace UpdateManager::Resources;

    std::ostream& log = m_log;

    if( !m_journalFileName.empty() && ( m_journalFileName != "/" ) &&
        filesystem::isDirectoryExist( m_journalFileName ) ) {
        LOG_WARN( m_journalFileName + " is folder but it should be file." )
        filesystem::deleteFolder( m_journalFileName );
    }

    std::fstream journal( m_journalFileName, std::fstream::in | std::fstream::out );

    if( !journal.is_open() ) {
        LOG_WARN( "Can not open update journal" )
        return false;
    }

    bool effectRequired = !m_isDaemon;

    LOG_INFO( "Starting update process" )
    int steps = 0;
    std::string line;
    while( getline( journal, line ) ) {
        LOG_INFO( "journal line: " + line );
        if( effectRequired ) {
            system( "/opt/firmware/bin/ha-ledcmd --start firmwareUpdate" );
            effectRequired = false;
        }
        steps++;
        std::istringstream iss( line );
        std::string command, packageName, packageVersion, status;
        if( !( iss >> command >> packageName >> packageVersion >> status ) ) {
            LOG_ERROR( "Ill-formed update journal entry: " << line )
            return false;
        }

        if( !iss.eof() ) {
            LOG_ERROR( "Ill-formed update journal entry: " << line )
            return false;
        }

        if( command == Commands::Sysupgrade && status == Statuses::Executing ) {
            LOG_INFO( "Finishing sysupgrade" + std::string( " packageName: " ) + packageName +
                      std::string( " packageVersion: " ) + packageVersion )
            if( finishSysupgrade( packageName, packageVersion ) ) {
                LOG_INFO( "Sysupgrade completed successfully" )
                changeStatus( journal, Statuses::Completed );
                continue;
            }
            else {
                LOG_ERROR( "Sysupgrade failed" )
                changeStatus( journal, Statuses::Failed );
                return false;
            }
        }

        if( status == Statuses::Completed ) {
            LOG_INFO( "Skipping completed step: " << command << " " << packageName << " " << packageVersion )
            continue;
        }

        if( status != Statuses::Scheduled ) {
            LOG_WARN( "Try recovering from failed step: " << command << " " << packageName << " " << packageVersion )
            changeStatus( journal, Statuses::Scheduled );
            status = Statuses::Scheduled;
        }

        if( status == Statuses::Scheduled ) {
            LOG_INFO( "Executing: " << command << " " << packageName << " " << packageVersion )
            changeStatus( journal, Statuses::Executing );

            if( auto result = executeStep( command, packageName, packageVersion ) ) {
                LOG_ERROR( "Step failed with code " << result )
                changeStatus( journal, Statuses::Failed );
                return false;
            }

            LOG_INFO( "Step completed successfully" )
            changeStatus( journal, Statuses::Completed );
        }

        auto completedPos = journal.tellg();
        journal.seekg( 0, std::fstream::end );
        auto eofPos = journal.tellg();
        journal.seekg( completedPos );
        if( completedPos == eofPos ) {
            LOG_INFO( "All steps completed" )
            return true;
        }

        if( singleStep )
            break;
    }

    if( steps == 0 )
        LOG_WARN( "Update journal is empty" )

    return true;
}

int UpdateExecutor::executeStep( const std::string& command,
                                 const std::string& packageName,
                                 const std::string& packageVersion ) {
    using namespace UpdateManager::Resources;

    std::ostream& log = m_log;
    std::string output;
    int result = 0;

    for( int i = 0; i < RetriesCount; ++i ) {
        if( i != 0 ) {
            LOG_WARN( "Retry #" << i << ": " << command << " " << packageName << " "
                                << packageVersion + " in " + std::to_string( RetriesDelay.count() ) + " seconds" )
            std::this_thread::sleep_for( RetriesDelay );
            // NOTE: problem may be due to outdated packages list
            m_availablePackagesOutdated = true;
        }

        if( command == Commands::Sysupgrade ) {
            sendLogs( &m_log );
            auto& imageURL = packageName;
            std::tie( output, result ) = m_packagesManager->sysupgrade( imageURL, CacheDirectories::SystemImage );

            m_log << "output: " << output;
            m_log << "result: " << result;
        }
        else if( command == Commands::Upgrade ) {
            result = ensureAvailablePackagesUpToDate();
            if( result != 0 )
                return result;

            std::tie( output, result ) = m_packagesManager->upgradePackage( packageName, CacheDirectories::Packages );
        }
        else if( command == Commands::Install ) {
            result = ensureAvailablePackagesUpToDate();
            if( result != 0 )
                return result;

            std::tie( output, result ) = m_packagesManager->installPackage( packageName, CacheDirectories::Packages );
        }
        else if( command == Commands::Uninstall ) {
            std::tie( output, result ) = m_packagesManager->removePackage( packageName );
        }
        else {
            LOG_ERROR( "Unknown command in update journal: " << command )
            return 1;
        }

        m_log << output;

        if( result == 0 )
            break;
    }

    return result;
}

void UpdateExecutor::changeStatus( std::fstream& journal, const char* newStatus ) {
    journal.seekp( -( static_cast< std::fstream::off_type >( strlen( newStatus ) ) + 1 /*'\n'*/ ), std::fstream::cur );
    journal << newStatus << std::endl;
    journal.flush();
}

bool UpdateExecutor::finishSysupgrade( const std::string& imageUrl, const std::string& packageVersion ) {
    std::ostream& log = m_log;

    LOG_INFO( "Checking if sysupgrade completed successfully..." )
    // TODO: check system version, not firmware version (assuming we do sysupgrade only if system changed)
    auto installed = m_packagesManager->getInstalledPackages();

    auto isProperFirmwarePackage = [&]( const Opkg::PackageSummary& summary ) {
        LOG_INFO( std::string( "installed name: " ) + summary.name + std::string( " installed version: " ) +
                  summary.version );
        LOG_INFO( std::string( "package name: " ) + UpdateManager::Resources::PackageNames::Firmware +
                  std::string( " journal version: " ) + packageVersion );
        return ( summary.name == UpdateManager::Resources::PackageNames::Firmware ) &&
               ( summary.version == packageVersion );
    };

    if( std::none_of( installed.begin(), installed.end(), isProperFirmwarePackage ) ) {
        LOG_ERROR( "Firmware package has wrong version" )
        return false;
    }

    // TODO: Should do this only after sysupgrade?
    using namespace UpdateManager::Resources;
    if( filesystem::isDirectoryExist( CacheDirectories::Update ) ) {
        LOG_INFO( "Firmware package has proper version, retrieving caches..." )
        if( !filesystem::copyFolder( CacheDirectories::OpkgLists, "/tmp/opkg-lists" ) ) {
            return false;
        }
        // NOTE: we may need this during restore process
        if( !filesystem::copyFile( CacheDirectories::Distfeeds + std::string( "/distfeeds.conf" ),
                                   "/etc/opkg/distfeeds.conf" ) ) {
            return false;
        }
    }
    else {
        // Leave opkg updating logic if we are updgrading from firmware that does not prepare cache yet
        LOG_INFO( "Firmware package has proper version, updating feeds..." )
        filesystem::createFolder( CacheDirectories::Update );
        filesystem::createFolder( CacheDirectories::SystemImage );
        filesystem::createFolder( CacheDirectories::Packages );
        m_feedsUpdater->update( imageUrl );
        m_availablePackagesOutdated = true;
        return true;
    }

    return true;
}
int UpdateExecutor::ensureAvailablePackagesUpToDate() {
    std::ostream& log = m_log;

    if( !m_availablePackagesOutdated )
        return 0;

    LOG_INFO( "Updating available packages list" )

    std::string output;
    int result = 0;
    for( int i = 0; i < RetriesCount; ++i ) {
        if( i != 0 ) {
            LOG_WARN( "Retry #" + std::to_string( i ) + " in " + std::to_string( RetriesDelay.count() ) + " seconds" );
            std::this_thread::sleep_for( RetriesDelay );
        }
        std::tie( output, result ) = m_packagesManager->updateAvailable();
        m_log << output;

        if( result == 0 )
            break;
    }

    if( result != 0 ) {
        LOG_ERROR( "Failed to update available packages list" )
    }

    m_availablePackagesOutdated = false;
    return result;
}

}  // namespace UpdateCommand
}  // namespace HubPlatform
