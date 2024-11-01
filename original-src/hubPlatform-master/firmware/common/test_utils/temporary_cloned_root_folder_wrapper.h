
#pragma once

#include "test_utils/temporary_root_folder_wrapper.h"

namespace HubPlatform {
namespace Tests {

class TemporaryClonedRootFolderWrapper final : public TemporaryRootFolderWrapper {
public:
    TemporaryClonedRootFolderWrapper( const std::string& sourcePath );
};

}  // namespace Tests
}  // namespace HubPlatform