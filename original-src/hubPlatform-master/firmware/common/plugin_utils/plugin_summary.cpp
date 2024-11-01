#include "plugin_utils/plugin_summary.h"

#include "json_utils/json_utils.h"

#include <stdexcept>

namespace HubPlatform {
namespace CommonUtils {

PluginSummary PluginSummary::fromJson( const JsonObject& config ) {
    try {
        jsonCheckString( config, "id" );
        jsonCheckString( config, "version" );
    }
    catch( const std::exception& exception ) {
        return {};
    }

    return { json_get_str( config[ "id" ] ), PluginVersion( json_get_str( config[ "version" ] ) ) };
}

JsonObject PluginSummary::toJson() const {
    return JSON( "id" << id << "version" << version.toString() );
}

std::string PluginSummary::toString() const {
    return id + "-" + version.toString();
}

bool PluginSummary::isValid() const {
    return !id.empty() && version.isValid();
}

bool PluginSummary::operator==( const PluginSummary& rhs ) const {
    return ( id == rhs.id ) && ( version == rhs.version );
}

bool PluginSummary::operator!=( const PluginSummary& rhs ) const {
    return !operator==( rhs );
}

}  // namespace CommonUtils
}  // namespace HubPlatform
