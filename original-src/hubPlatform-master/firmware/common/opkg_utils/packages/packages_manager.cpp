#include "opkg_utils/packages/packages_manager.h"
#include "system_utils/system.h"

#include <tuple>
#include <sstream>

namespace HubPlatform {
namespace Opkg {

PackagesManager::PackagesManager( CommandRunner& commandRunner )
: m_commandRunner( commandRunner ) {}

PackagesManager::PackageSummaries PackagesManager::getInstalledPackages() const {
    PackageSummaries summaries;

    auto output = std::get< 0 >( m_commandRunner.listInstalled() );

    std::istringstream outputStream( output );
    std::string line;

    while( std::getline( outputStream, line ) ) {
        std::istringstream lineStream( line );
        std::string name, delimiter, version;
        if( !( lineStream >> name >> delimiter >> version ) )
            continue;

        if( !lineStream.eof() )
            continue;

        if( delimiter != "-" )
            continue;

        PackageSummary summary = { std::move( name ), std::move( version ) };
        summaries.emplace_back( std::move( summary ) );
    }

    return summaries;
}

PackagesManager::UpgradablePackageSummaries PackagesManager::getUpgradablePackages() const {
    UpgradablePackageSummaries summaries;

    auto output = std::get< 0 >( m_commandRunner.listUpgradable() );

    std::istringstream outputStream( output );
    std::string line;

    while( std::getline( outputStream, line ) ) {
        std::istringstream lineStream( line );
        std::string name, delimiter1, version, delimiter2, availableVersion;
        if( !( lineStream >> name >> delimiter1 >> version >> delimiter2 >> availableVersion ) )
            continue;

        if( !lineStream.eof() )
            continue;

        if( ( delimiter1 != "-" ) || ( delimiter2 != "-" ) )
            continue;

        PackageSummary summary = { std::move( name ), std::move( version ) };
        UpgradablePackageSummary upgradableSummary = { std::move( summary ), std::move( availableVersion ) };
        summaries.emplace_back( std::move( upgradableSummary ) );
    }

    return summaries;
}

PackagesManager::PackageSummaries PackagesManager::getAvailablePackages() const {
    PackageSummaries summaries;

    auto output = std::get< 0 >( m_commandRunner.listAvailable() );

    std::istringstream outputStream( output );
    std::string line;

    while( std::getline( outputStream, line ) ) {
        if( !line.empty() && line[ 0 ] == ' ' )
            continue;

        std::istringstream lineStream( line );
        std::string name, delimiter1, version;
        bool isAvailable = false;

        if( !( lineStream >> name >> delimiter1 >> version ) )
            continue;

        if( delimiter1 != "-" )
            continue;

        if( !lineStream.eof() ) {
            isAvailable = true;
        }

        if( isAvailable ) {
            PackageSummary summary = { std::move( name ), std::move( version ) };
            summaries.emplace_back( std::move( summary ) );
        }
    }

    return summaries;
}

PackagesManager::PackageSummaries PackagesManager::getDependantPackages( const std::string& packageName ) const {
    PackageSummaries summaries;

    auto output = std::get< 0 >( m_commandRunner.listDependant( packageName ) );

    std::istringstream outputStream( output );
    std::string line;

    while( std::getline( outputStream, line ) ) {
        if( line.empty() || line[ 0 ] != '\t' )
            continue;

        std::istringstream lineStream( line );
        std::string name, version;
        if( !( lineStream >> name >> version ) )
            continue;

        if( lineStream.eof() )
            continue;

        PackageSummary summary = { std::move( name ), std::move( version ) };
        summaries.emplace_back( std::move( summary ) );
    }

    return summaries;
}

PackagesManager::StdOutWithReturnCode PackagesManager::updateAvailable() {
    return m_commandRunner.updateAvailable();
}

/** Perform update with download-only flag, no installation */
PackagesManager::StdOutWithReturnCode PackagesManager::checkAvailable() {
    return m_commandRunner.updateDownloadOnlyAvailable();
}

PackagesManager::StdOutWithReturnCode PackagesManager::downloadImage( const std::string& imageUrl,
                                                                      const std::string& cacheDirectory ) {
    return m_commandRunner.downloadImage( imageUrl, cacheDirectory );
}

PackagesManager::StdOutWithReturnCode PackagesManager::downloadPackage( const std::string& packageName,
                                                                        const std::string& cacheDirectory,
                                                                        bool withDependencies ) {
    return m_commandRunner.downloadPackage( packageName, cacheDirectory, withDependencies );
}

PackagesManager::StdOutWithReturnCode PackagesManager::sysupgrade( const std::string& imageUrl,
                                                                   const std::string& cacheDirectory ) {
    return m_commandRunner.sysupgrade( imageUrl, cacheDirectory );
}

PackagesManager::StdOutWithReturnCode PackagesManager::upgradePackage( const std::string& packageName,
                                                                       const std::string& cacheDirectory ) {
    return m_commandRunner.upgradePackage( packageName, cacheDirectory );
}

PackagesManager::StdOutWithReturnCode PackagesManager::installPackage( const std::string& packageName,
                                                                       const std::string& cacheDirectory ) {
    return m_commandRunner.installPackage( packageName, cacheDirectory );
}

PackagesManager::StdOutWithReturnCode PackagesManager::removePackage( const std::string& packageName ) {
    return m_commandRunner.removePackage( packageName );
}

}  // namespace Opkg
}  // namespace HubPlatform