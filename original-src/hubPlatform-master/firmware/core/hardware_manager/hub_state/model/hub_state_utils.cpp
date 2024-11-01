#include "hardware_manager/hub_state/model/hub_state_utils.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareManager {
namespace HubStateUtils {

namespace {
void setState( IDbClient& dbClient, HubState::Type type, const std::string& value ) {
    Json state;
    state[ db::ID_FIELD_NAME ] = HubState::toString( type );
    state[ "value" ] = value;
    dbClient.update( TempHubStateDBName, JSON_EMPTY_OBJ, state, true, false );
}

std::string getState( const IDbClient& dbClient, HubState::Type type ) {
    auto state = dbClient.findOne( TempHubStateDBName, JSON( db::ID_FIELD_NAME << toString( type ) ), nullptr );
    return json_get_str( state, "value" );
}

}  // namespace

void setCloudConnection( IDbClient& dbClient, HubState::CloudConnection cloudConnection ) {
    setState( dbClient, HubState::Type::CloudConnection, HubState::toString( cloudConnection ) );
}

HubState::CloudConnection getCloudConnection( const IDbClient& dbClient ) {
    return HubState::cloudConnectionFromString( getState( dbClient, HubState::Type::CloudConnection ) );
}

void setZwaveState( IDbClient& dbClient, HubState::Zwave zwaveState ) {
    setState( dbClient, HubState::Type::ZwaveState, HubState::toString( zwaveState ) );
}

HubState::Zwave getZwaveState( const IDbClient& dbClient ) {
    return HubState::zwaveStateFromString( getState( dbClient, HubState::Type::ZwaveState ) );
}

void setBatteryState( IDbClient& dbClient, HubState::Battery batteryState ) {
    setState( dbClient, HubState::Type::BatteryState, HubState::toString( batteryState ) );
}

HubState::Battery getBatteryState( const IDbClient& dbClient ) {
    return HubState::batteryStateFromString( getState( dbClient, HubState::Type::BatteryState ) );
}

}  // namespace HubStateUtils
}  // namespace HardwareManager
}  // namespace HubPlatform
