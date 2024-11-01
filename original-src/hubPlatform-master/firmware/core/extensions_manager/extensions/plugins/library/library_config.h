#pragma once

#include "json_utils/json_fwd.h"

#include <memory>
#include <string>

namespace HubPlatform {
namespace ExtensionsManager {

struct LibraryConfig final {
    static std::unique_ptr< LibraryConfig > getLibraryConfig( const Json& joConfig );

    std::string m_entryPoint;
};

}  // namespace ExtensionsManager
}  // namespace HubPlatform
