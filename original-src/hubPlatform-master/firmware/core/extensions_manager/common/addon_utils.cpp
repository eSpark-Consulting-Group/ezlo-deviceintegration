#include "extensions_manager/common/addon_utils.h"
#include "common_utils/addons/addon_summary.h"
#include "extensions_manager/resources/em_resources.h"
#include "system_utils/environment.h"

#include "json_utils/json_utils.h"
#include "common_utils/curl_wrapper.h"
#include "common_utils/cpp14.h"

#include <sys/stat.h>

#include <fstream>
#include <cstring>
#include <dirent.h>
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace ExtensionsManager {

AddonUtils::AddonIds AddonUtils::getInstalledAddons() const {
    AddonIds result;

    const auto addonsFolderPath = Environment::getAddonsFolder();
    DIR* dir = ::opendir( addonsFolderPath.c_str() );
    if( dir == nullptr ) {
        Log::warn( cc::warn( "Addons scanner: can not open addons directory: " ) + cc::p( addonsFolderPath ) );
        return result;
    }

    struct dirent* entry = nullptr;
    while( ( entry = ::readdir( dir ) ) != nullptr ) {
        if( entry->d_type != DT_REG && entry->d_type != DT_LNK )
            continue;

        auto addonId = extractAddonId( entry->d_name );
        if( addonId.empty() ) {
            Log::warn( cc::warn( "Addons scanner: ignoring file with strange name " ) + cc::note( entry->d_name ) );
            continue;
        }

        result.push_back( addonId );
    }
    ::closedir( dir );
    return result;
}

std::string AddonUtils::extractAddonId( const std::string& addonFileName ) const {
    if( addonFileName.find( "ha-" ) != 0U )
        return {};

    if( addonFileName.rfind( 'd' ) != addonFileName.size() - 1 )
        return {};

    std::string addonId = addonFileName.substr( 3, addonFileName.size() - 4 );
    return addonId;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
