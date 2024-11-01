#include "common_utils/addons/addon_summary.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CommonUtils {

AddonSummary AddonSummary::fromJson( const JsonObject& config ) {
    if( config.find( "id" ) == config.end() )
        return {};

    if( config.find( "version" ) == config.end() )
        return {};

    return { json_get_str( config[ "id" ] ), AddonVersion( json_get_str( config[ "version" ] ) ) };
}

std::string AddonSummary::toString() const {
    return id + "-" + version.toString();
}

JsonObject AddonSummary::toJson() const {
    return JSON( "id" << id << "version" << version.toString() );
}

bool AddonSummary::isValid() const {
    return !id.empty() && version.isValid();
}

}  // namespace CommonUtils
}  // namespace HubPlatform
