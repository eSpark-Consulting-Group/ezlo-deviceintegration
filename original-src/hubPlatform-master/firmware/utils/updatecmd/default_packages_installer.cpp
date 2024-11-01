#include "default_packages_installer.h"

#include "update_manager/resources/um_resources.h"

#include "system_utils/filesystem.h"
#include "system_utils/environment.h"

#include <list>
#include <cstring>
#include <unistd.h>

namespace HubPlatform {
namespace UpdateCommand {

DefaultPackagesInstaller::DefaultPackagesInstaller(
    std::unique_ptr< HubPlatform::Opkg::IPackagesManager > packagesManager,
    const std::string& logFileName )
: m_packagesManager( std::move( packagesManager ) )
, m_log( logFileName, std::ios_base::app ) {}

bool DefaultPackagesInstaller::run() {
    using namespace filesystem;
    const std::string defaultPackagesDir = Environment::getFirmwareRootFolder() + "/default";
    const std::string installFlagFile = defaultPackagesDir + "/.default_packages_installed";

    std::ostream& log = m_log;

    // If there's no flag file
    if( !std::ifstream( installFlagFile ).good() ) {
        LOG_DEBUG( "File '" + installFlagFile + "' does not exist." );

        std::list< std::string > defaultPackages;
        bool noResolvedDependencies;

        auto collectPackagesNames = [&defaultPackages]( const std::string& dirPath, const std::string& fileName ) {
            defaultPackages.push_back( dirPath + '/' + fileName );
            return true;
        };

        iterateFileSystemEntry( defaultPackagesDir + "/addons", collectPackagesNames );
        iterateFileSystemEntry( defaultPackagesDir + "/plugins", collectPackagesNames );
        iterateFileSystemEntry( defaultPackagesDir + "/custom_packages", collectPackagesNames );

        // try to install default packages resolving dependencies
        while( true ) {
            noResolvedDependencies = true;

            for( auto it = defaultPackages.begin(); it != defaultPackages.end(); ) {
                int returnCode = 0;
                std::string output;
                const auto packageName = *it;

                std::tie( output, returnCode ) = m_packagesManager->installPackage( packageName );
                LOG_DEBUG( output );
                if( returnCode != 0 ) {
                    LOG_ERROR( "Error occured during installation of a '" + packageName +
                               "' package. Error code: " + std::to_string( returnCode ) );
                    ++it;
                }
                else {
                    it = defaultPackages.erase( it );
                    noResolvedDependencies = false;
                }
            }

            if( defaultPackages.empty() ) {
                break;
            }

            if( noResolvedDependencies ) {
                LOG_ERROR( "Default packages are not installed properly!" );
                return false;
            }
        }

        // Create flag file
        if( !std::ofstream( installFlagFile ).good() ) {
            LOG_DEBUG( "Could not create file '" << installFlagFile << "'!" );
        }
        else {
            LOG_DEBUG( "Created file '" + installFlagFile + "'" );
        }
        sync();
    }
    else {
        LOG_DEBUG( "File '" + installFlagFile + "' exists." );
    }

    return true;
}

}  // namespace UpdateCommand
}  // namespace HubPlatform
