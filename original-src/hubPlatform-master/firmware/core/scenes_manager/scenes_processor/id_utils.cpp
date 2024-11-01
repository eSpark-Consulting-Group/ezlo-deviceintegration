#include "scenes_manager/scenes_processor/id_utils.h"
#include "base_utils/ezlodb.h"

#include <cstring>

namespace HubPlatform {
namespace ScenesManager {
namespace IdUtils {

static const char* placeholderPrefix = "${";
static const char placeholderSuffix = '}';

bool isPlaceholder( const std::string& str ) {
    if( str.empty() )
        return false;

    const size_t prefixLen = strlen( placeholderPrefix );
    const size_t suffixLen = 1;
    const size_t placeholderMinLen = 1;
    if( str.length() < prefixLen + suffixLen + placeholderMinLen )
        return false;

    return str.rfind( placeholderPrefix, 0 ) == 0 && str.back() == placeholderSuffix;
}

std::string replacePlaceholder( const std::string& placeholder, PlaceholdersMapping& mapping ) {
    if( !isPlaceholder( placeholder ) )
        throw std::logic_error( "Provided string is not an id placeholder" );

    auto it = mapping.find( placeholder );
    if( it != mapping.end() )
        return it->second;

    std::string newId = Db::genOid();
    mapping.emplace( placeholder, newId );
    return newId;
}

}  // namespace IdUtils
}  // namespace ScenesManager
}  // namespace HubPlatform
