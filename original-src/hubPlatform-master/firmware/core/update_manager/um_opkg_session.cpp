#include "update_manager/um_opkg_session.h"

#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"

#include <chrono>
#include <thread>
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace UpdateManager {

namespace {
constexpr const int RetriesCount = 2;
constexpr const std::chrono::seconds RetriesDelay( 10 );
constexpr const char OpkgListsPath[] = "/tmp/opkg-lists";
}  // namespace

OpkgSession::OpkgSession( Opkg::OpkgManager& opkgManager )
: m_opkgManager( opkgManager ) {}

/** Create a session with specified updater class */
OpkgSession::OpkgSession( Opkg::OpkgManager& opkgManager, std::unique_ptr< Opkg::IFeedsUpdater > feedsUpdater )
: m_opkgManager( opkgManager )
, m_feedsUpdater( std::move( feedsUpdater ) ) {}

OpkgSession::~OpkgSession() {
    if( m_feedsUpdated ) {
        takeFeedsUpdater().resetToCurrent();
    }
}

bool OpkgSession::updateFeeds( const std::string& source, int snapshot ) {
    m_feedsUpdated = true;
    return takeFeedsUpdater().update( source, snapshot );
}

void OpkgSession::commit() {
    m_feedsUpdated = false;
    takeFeedsUpdater().resetCurrent();
}

bool OpkgSession::updateAvailablePackages() {
    try {
        return tryUpdateAvailablePackages();
    }
    catch( const OpkgSessionAvailabilityFailed& ex ) {
        LOGGER_ERROR_RAW( "Failed to update packages: update failed" );
    }
    catch( const OpkgSessionException& ex ) {
        LOGGER_ERROR_RAW( "Failed to update packages" );
    }
    return false;
}

bool OpkgSession::tryUpdateAvailablePackages() {
    if( m_opkgListsUpdated ) {
        // Assume lists were not changed during this opkg session
        return true;
    }

    std::string output;
    int result = 0;
    Log::info( "Updating available packages list" );
    for( int i = 0; i < RetriesCount + 1; ++i ) {
        if( i != 0 ) {
            Log::warn( "Retry #" + std::to_string( i ) + " in " + std::to_string( RetriesDelay.count() ) + " seconds" );
            std::this_thread::sleep_for( RetriesDelay );
        }
        std::tie( output, result ) = takePackagesManager().updateAvailable();
        if( result != 0 ) {
            LOGGER_ERROR( cc::error( "opkg update output, result: " ) + cc::num10( result ) );
        }
        LOGGER_DEBUG_RAW( output );

        if( result == 0 ) {
            LOGGER_WARN( cc::warn( "opkg update finishes successfully, stop retrying." ) );
            break;
        }
    }

    if( result != 0 ) {
        Log::error( cc::error( "Failed to update available packages list" ) );
        throw OpkgSessionAvailabilityFailed( result, output );
    }

    m_opkgListsUpdated = true;
    return true;
}

bool OpkgSession::checkAvailablePackages() {
    std::string output;
    int result = 0;
    Log::info( "Checking available packages list" );
    for( int i = 0; i < RetriesCount + 1; ++i ) {
        if( i != 0 ) {
            Log::warn( "Retry #" + std::to_string( i ) + " in " + std::to_string( RetriesDelay.count() ) + " seconds" );
            std::this_thread::sleep_for( RetriesDelay );
        }
        std::tie( output, result ) = takePackagesManager().checkAvailable();

        if( result != 0 ) {
            LOGGER_ERROR( cc::error( "opkg update only available output, result: " ) + cc::num10( result ) );
        }
        LOGGER_DEBUG_RAW( output );

        if( result == 0 ) {
            LOGGER_DEBUG( cc::debug( "opkg check update finished successfully, stop retrying." ) );
            break;
        }
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    }

    if( result != 0 ) {
        Log::error( cc::error( "Failed to check available packages list" ) );
        return false;
    }
    return true;
}

bool OpkgSession::verifyAvailablePackageVersion( Opkg::PackageSummary& packageSummary ) {
    if( !updateAvailablePackages() ) {
        Log::warn( "Update available packages failed" );
        return false;
    }

    if( packageSummary.version.empty() ) {
        auto isProperPackage = [&]( const Opkg::PackageSummary& package ) {
            return package.name == packageSummary.name;
        };

        auto packages = takePackagesManager().getAvailablePackages();
        auto it = std::find_if( packages.begin(), packages.end(), isProperPackage );
        if( it == packages.end() ) {
            Log::info( "Package " + packageSummary.name + " is not available" );
            return false;
        }

        packageSummary.version = it->version;
    }
    else {
        auto isProperPackage = [&]( const Opkg::PackageSummary& package ) {
            return ( package.name == packageSummary.name ) && ( package.version == packageSummary.version );
        };

        auto packages = takePackagesManager().getAvailablePackages();
        auto it = std::find_if( packages.begin(), packages.end(), isProperPackage );
        if( it == packages.end() ) {
            Log::info( "Requested " + packageSummary.name + " version is not available" );
            return false;
        }
    }

    return true;
}

const Opkg::IFeedsUpdater& OpkgSession::getFeedsUpdater() const {
    if( !m_feedsUpdater ) {
        m_feedsUpdater = m_opkgManager.createFeedsUpdater();
    }
    return *m_feedsUpdater;
}

Opkg::IFeedsUpdater& OpkgSession::takeFeedsUpdater() {
    if( !m_feedsUpdater ) {
        m_feedsUpdater = m_opkgManager.createFeedsUpdater();
    }
    return *m_feedsUpdater;
}

Opkg::IPackagesManager& OpkgSession::takePackagesManager() {
    if( !m_packagesManager ) {
        m_packagesManager = m_opkgManager.createPackagesManager();
    }
    return *m_packagesManager;
}

OpkgSessionAvailabilityFailed::OpkgSessionAvailabilityFailed( int code, std::string output )
: OpkgSessionException( "Failed to update packages" )
, m_code( code )
, m_output( std::move( output ) ) {}

}  // namespace UpdateManager
}  // namespace HubPlatform
