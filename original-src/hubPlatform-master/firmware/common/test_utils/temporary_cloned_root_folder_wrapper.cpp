
#include "test_utils/temporary_cloned_root_folder_wrapper.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"

#include <unistd.h>

namespace HubPlatform {
namespace Tests {

TemporaryClonedRootFolderWrapper::TemporaryClonedRootFolderWrapper( const std::string& sourcePath ) {
    const auto isCopied = filesystem::copyFolder( sourcePath, getPath() );
    if( !isCopied ) {
        throw TemporaryFolderError( std::string( "TemporaryFolderWrapper: error: Could not copy etalon folder: " ) +
                                    sourcePath );
    }
}

}  // namespace Tests
}  // namespace HubPlatform