#include "security_threat_migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_39_security_threat {

namespace DbNames {
const char* DevicesDb = "db.firmware.cored.devices";  // HaDevicesDBName
const char* ItemsDb = "db.firmware.uid.items";        // HaItemsDBName
}  // namespace DbNames

namespace Fields {
constexpr char id[] = "_id";
constexpr char armed[] = "armed";
constexpr char deviceId[] = "deviceId";
};  // namespace Fields

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
    Json fields = JSON( Fields::id << 1 );
    auto armedDevices = m_dbClient.findAll( DbNames::DevicesDb, JSON( Fields::armed << true ), &fields );
    for( const auto& device : armedDevices ) {
        const std::string& deviceId = json_get_str( device, Fields::id );
        Json deviceQuery = JSON( Fields::deviceId << deviceId );
        auto securityThreat = m_dbClient.findOne( DbNames::ItemsDb, deviceQuery, nullptr );

        if( securityThreat.empty() ) {
            LOGGER_INFO( "Set device.armed=false for device " + cc::note( deviceId ) );

            auto updateArmed = JSON( "$set" << JSON( Fields::armed << false ) );
            m_dbClient.update( DbNames::DevicesDb, JSON( Fields::id << deviceId ), updateArmed );
            m_modifiedDevices++;
        }
    }
    return true;
}

size_t Migration::modifiedDevices() const {
    return m_modifiedDevices;
}

}  // namespace v1_39_security_threat
}  // namespace MigrationTool
}  // namespace HubPlatform