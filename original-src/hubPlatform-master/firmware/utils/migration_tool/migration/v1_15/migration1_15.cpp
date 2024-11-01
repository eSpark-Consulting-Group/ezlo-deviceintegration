#include "migration_tool/migration/v1_15/migration1_15.h"

#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_15 {

Migration::Migration( IDbClient& dbClient, const ProvisionDataUtils::ProvisionData& provisionData )
: m_dbClient( dbClient )
, m_provisionData( provisionData ) {}

bool Migration::migrate() {
    auto CredentialsDBName = "db.firmware.uid.credentials";

    auto allPeersCredentials =
        m_dbClient.findAll( CredentialsDBName, JSON( "key" << JSON( "$exists" << true ) ), nullptr );

    for( auto& peerCredentials : allPeersCredentials ) {
        auto id = json_get_str( peerCredentials, db::ID_FIELD_NAME );
        auto key = json_get_str( peerCredentials, "key" );
        auto encryptedKey = m_provisionData.encryptSensitiveData( key );
        m_dbClient.update(
            CredentialsDBName, JSON( db::ID_FIELD_NAME << id ), JSON( "$set" << JSON( "key" << encryptedKey ) ) );
    }
    return true;
}

}  // namespace v1_15
}  // namespace MigrationTool
}  // namespace HubPlatform
