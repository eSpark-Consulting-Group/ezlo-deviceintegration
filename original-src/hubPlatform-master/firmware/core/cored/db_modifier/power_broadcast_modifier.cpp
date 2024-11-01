#include "cored/db_modifier/power_broadcast_modifier.h"
#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {

PowerBroadcastModifier::PowerBroadcastModifier( IDbClient& dbClient )
: m_dbClientRef( dbClient ) {}

bool PowerBroadcastModifier::shouldUserBeNotified( const std::string& itemName,
                                                   const std::string& deviceCategory,
                                                   const std::string& deviceSubcategory,
                                                   const Json& broadcastData ) {
    using itemValidator = bool ( * )(
        const std::string& deviceCategory, const std::string& deviceSubcategory, const Json& broadcastData );
    static const std::map< std::string, itemValidator > itemsValidators = {
        { "power_surge_state", &powerSurge },
        { "voltage_drop_drift_state", &voltageDropDrift },
        { "over_current_state", &overCurrent },
        { "over_voltage_state", &overVoltageState },
        { "over_load_state", &overLoad },
        { "load_error_state", &loadError },
        { "battery_maintenance_state", &batteryMaintenance },
        { "battery_charging_state", &batteryCharging },
        { "ac_state", &ac },
    };

    auto foundIt = itemsValidators.find( itemName );
    if( foundIt != itemsValidators.end() ) {
        return foundIt->second( deviceCategory, deviceSubcategory, broadcastData );
    }
    return false;
}

bool PowerBroadcastModifier::prepareUpdateBroadcast( const std::string& itemName,
                                                     const std::string& deviceCategory,
                                                     const std::string& deviceSubcategory,
                                                     Json& broadcastData ) {
    if( shouldUserBeNotified( itemName, deviceCategory, deviceSubcategory, broadcastData ) ) {
        setUserNotification( broadcastData, true );
        setParentDeviceName( broadcastData );
        return true;
    }
    return false;
}

void PowerBroadcastModifier::setUserNotification( Json& broadcastData, bool value ) {
    broadcastData[ "userNotification" ] = value;
}

void PowerBroadcastModifier::setParentDeviceName( Json& broadcastData ) {
    using namespace Model::Resources;
    const auto& parentDeviceId = json_get_str( broadcastData, DeviceFields::ParentDeviceIdFieldName );
    if( !parentDeviceId.empty() ) {
        const Json fieldsToReturn = JSON( DeviceFields::NameFieldName << true );
        const Json query = JSON( DeviceFields::IdFieldName << parentDeviceId );
        auto parent = m_dbClientRef.get().findOne( HaDevicesDBName, query, &fieldsToReturn );
        if( !parent.empty() ) {
            broadcastData[ "parentDeviceName" ] = parent[ DeviceFields::NameFieldName ];
        }
    }
}

bool PowerBroadcastModifier::powerSurge( const std::string& deviceCategory,
                                         const std::string& deviceSubcategory,
                                         const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "surge_detected";
}

bool PowerBroadcastModifier::voltageDropDrift( const std::string& deviceCategory,
                                               const std::string& deviceSubcategory,
                                               const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "drop_drift_detected";
}

bool PowerBroadcastModifier::overCurrent( const std::string& deviceCategory,
                                          const std::string& deviceSubcategory,
                                          const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "over_current_detected";
}

bool PowerBroadcastModifier::overVoltageState( const std::string& deviceCategory,
                                               const std::string& deviceSubcategory,
                                               const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "over_voltage_detected";
}

bool PowerBroadcastModifier::overLoad( const std::string& deviceCategory,
                                       const std::string& deviceSubcategory,
                                       const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "over_load_detected";
}

bool PowerBroadcastModifier::loadError( const std::string& deviceCategory,
                                        const std::string& deviceSubcategory,
                                        const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "load_error";
}

bool PowerBroadcastModifier::batteryMaintenance( const std::string& deviceCategory,
                                                 const std::string& deviceSubcategory,
                                                 const Json& broadcastData ) {
    auto value = json_get_str( broadcastData, "value" );
    return ( value == "replace_battery_soon" || value == "replace_battery_now" || value == "battery_fluid_is_low" );
}

bool PowerBroadcastModifier::batteryCharging( const std::string& deviceCategory,
                                              const std::string& deviceSubcategory,
                                              const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "in_progress";
}

bool PowerBroadcastModifier::ac( const std::string& deviceCategory,
                                 const std::string& deviceSubcategory,
                                 const Json& broadcastData ) {
    return json_get_str( broadcastData, "value" ) == "connected";
}

}  // namespace CoreD
}  // namespace HubPlatform