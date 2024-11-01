
#include "test_utils/temporary_root_folder_wrapper.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"

#include <unistd.h>

namespace HubPlatform {
namespace Tests {

TemporaryRootFolderWrapper::TemporaryRootFolderWrapper() {
    Environment::setFirmwareRootFolder( getPath() );
    Environment::setTempDataFolder( getPath() + "/tmp" );

    auto createFolderAndThrowErrorIfCannot = []( const std::string& path ) {
        if( !filesystem::createFolder( path ) ) {
            throw TemporaryFolderError( std::string( "TemporaryRootFolderWrapper: error: Could not create folder: " ) +
                                        path );
        }
    };

    createFolderAndThrowErrorIfCannot( Environment::getDataFolder() );
    createFolderAndThrowErrorIfCannot( Environment::getTempDataFolder() );
    createFolderAndThrowErrorIfCannot( Environment::getConfigsFolder() );
    createFolderAndThrowErrorIfCannot( Environment::getLibrariesFolder() );
    createFolderAndThrowErrorIfCannot( Environment::getCertificateFolder() + "/local" );
    createFolderAndThrowErrorIfCannot( Environment::getCustomPluginsFolder() );
}

}  // namespace Tests
}  // namespace HubPlatform