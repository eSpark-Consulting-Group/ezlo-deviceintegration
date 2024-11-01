#include "zigbee_gateway_settings.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_53_zigbee_gateway_settings {

namespace DbNames {
constexpr char GatewaysSettingsDB[] = "db.firmware.cored.gateway_settings";
}  // namespace DbNames

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    LOGGER_DEBUG_RAW( "Zigbee gateway settings ::migrate called" );

    m_dbClient.remove( DbNames::GatewaysSettingsDB,
                       JSON( "_id"
                             << "zigbee.radio_channel" ) );
    m_dbClient.remove( DbNames::GatewaysSettingsDB,
                       JSON( "_id"
                             << "zigbee.pan_id" ) );
    return true;
}

}  // namespace v1_53_zigbee_gateway_settings
}  // namespace MigrationTool
}  // namespace HubPlatform
