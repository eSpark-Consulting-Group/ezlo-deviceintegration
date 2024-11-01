#include "update_manager/common/um_hub_reset_type.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace UpdateManager {

HubResetType hubResetTypeFromJson( const JsonObject& json ) {
    bool requestedSoftReset = json_get_bool( json, "softReset" );
    bool requestedFactoryReset = json_get_bool( json, "resetToFactoryDefaults" );

    if( requestedFactoryReset || !requestedSoftReset )
        return HubResetType::Factory;

    return HubResetType::Soft;
}

JsonObject hubResetTypeToJson( HubResetType resetType ) {
    JsonObject json;
    switch( resetType ) {
    case HubResetType::Factory:
        json[ "resetToFactoryDefaults" ] = true;
        break;
    case HubResetType::Soft:
        json[ "softReset" ] = true;
        break;
    default:
        Log::error( "Detected usage of unsupported reset type" );
        break;
    }
    return json;
}

}  // namespace UpdateManager
}  // namespace HubPlatform