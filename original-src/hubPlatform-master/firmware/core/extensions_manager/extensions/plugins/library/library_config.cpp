#include "extensions_manager/extensions/plugins/library/library_config.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_custom_getters.h"
#include "common_utils/cpp14.h"

#include <stdexcept>

namespace HubPlatform {
namespace ExtensionsManager {

namespace {
constexpr const char DataSectionName[] = "library";
}  // namespace

std::unique_ptr< LibraryConfig > LibraryConfig::getLibraryConfig( const Json& joConfig ) {
    using namespace JsonCustomGetters;
    using StringChecking = CheckJsonStringStrictPolicy;

    const auto configGetter = makeFieldGetter(
        DataSectionName,
        makeStructGetter( makeMemberMapping( &LibraryConfig::m_entryPoint, "entryPoint", StringChecking() ) ) );

    auto result = std::make_unique< LibraryConfig >();
    if( !configGetter( joConfig, *result ) ) {
        Log::error( cc::error( "Error during loading gateway config from json" ) );
        return nullptr;
    }

    return result;
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
