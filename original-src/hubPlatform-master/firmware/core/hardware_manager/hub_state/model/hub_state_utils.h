#pragma once

#include "hardware_manager/hub_state/model/hub_state.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace HardwareManager {
namespace HubStateUtils {

void setCloudConnection( IDbClient& dbClient, HubState::CloudConnection cloudConnection );
HubState::CloudConnection getCloudConnection( const IDbClient& dbClient );

void setZwaveState( IDbClient& dbClient, HubState::Zwave zwaveState );
HubState::Zwave getZwaveState( const IDbClient& dbClient );

void setBatteryState( IDbClient& dbClient, HubState::Battery batteryState );
HubState::Battery getBatteryState( const IDbClient& dbClient );

}  // namespace HubStateUtils
}  // namespace HardwareManager
}  // namespace HubPlatform
