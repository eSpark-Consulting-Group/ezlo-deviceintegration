#include "protect_feature_on_e450_migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_47_protect_feature_on_e450 {

namespace DbNames {
const char CoreFeaturesDBName[] = "db.firmware.cored.features";
}  // namespace DbNames

namespace Fields {
const char id[] = "_id";
const char status[] = "status";
};  // namespace Fields

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool Migration::migrate() {
#if !defined( __HUB_TYPE_H2__ ) && !defined( __HUB_TYPE_LOCAL__ )
    return true;
#endif
    LOGGER_DEBUG_RAW( "E450 ProtectFeatureMigration::migrate called" );

    auto feature = m_dbClient.findOne( DbNames::CoreFeaturesDBName, JSON( Fields::id << "protect" ), nullptr );
    if( feature.empty() )
        return true;

    feature[ Fields::status ] = true;
    m_dbClient.update( DbNames::CoreFeaturesDBName, JSON( Fields::id << "protect" ), feature );

    return true;
}

}  // namespace v1_47_protect_feature_on_e450
}  // namespace MigrationTool
}  // namespace HubPlatform
