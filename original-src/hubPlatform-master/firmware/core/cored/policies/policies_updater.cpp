#include "policies.h"
#include "policies_updater.h"
#include "resources.h"
#include "json_utils/json_path.h"
#include "jsonrpc/jsonrpc.h"
#include <algorithm>
#include "config_reader/config_keys.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/group_operations/device_group.h"

namespace HubPlatform {
namespace CoreD {
using namespace Resources;

JsonObject PolicyUpdater::getPolicyUpdateData( const JsonObject& dbItem, const Json& params ) {
    using namespace Resources;
    auto savedName = json_get_str( dbItem, Policy::Name );
    auto updateName = json_get_str( params, Policy::Name );

    auto savedPersistent = json_get_bool( dbItem, Policy::Persistent );
    auto updatePersistent = json_get_bool( params, Policy::Persistent );

    bool hasUpdatedPersistent = json_get( params, Policy::Persistent, updatePersistent );
    Json updatedJson = params;

    if( !hasUpdatedPersistent ) {
        updatedJson[ Policy::Persistent ] = savedPersistent;
    }

    return updatedJson;
}

JsonObject PolicyUpdater::getPolicyUpdateBroadcastData( const JsonObject& updatedData, const JsonObject& savedData ) {
    using namespace Resources;
    using namespace Policies;

    Json updatedJson = JSON_EMPTY_OBJ;

    PolicyInfo fromDb, fromUser;
    fromDb.fromJson( savedData );
    fromUser.fromJson( updatedData );

    if( updatedData.count( Policy::ProtectConfig ) && fromDb.getSecurityConfig() != fromUser.getSecurityConfig() ) {
        updatedJson[ Policy::ProtectConfig ] = fromUser.getSecurityConfig();
    }

    if( updatedData.count( Policy::DeviceGroup ) && fromDb.getDeviceGroup() != fromUser.getDeviceGroup() ) {
        updatedJson[ Policy::DeviceGroup ] = fromUser.getDeviceGroup();
    }

    if( updatedData.count( Policy::MeshBots ) && ( fromDb.getMeshbots() != fromUser.getMeshbots() ) ) {
        updatedJson[ Policy::MeshBots ] = fromUser.getMeshbots();
    }

    if( fromDb.getName() != fromUser.getName() ) {
        updatedJson[ Policy::Name ] = fromUser.getName();
        updatedJson[ Policy::Sync ] = true;
    }
    else {
        if( !updatedJson.empty() ) {
            updatedJson[ Policy::Sync ] = false;
        }
    }

    return updatedJson;
}

void PolicyUpdater::ensurePredefinedPoliciesInDb( IDbClient& dbClient, const ConfigReader& configReader ) {
    using namespace Policies;
    ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::PreDefinePolicies );
    std::size_t size = configReader.getArraySize( path );
    for( std::size_t i = 0; i < size; ++i ) {
        try {
            ConfigReader::Path idPath = path / i;
            const JsonObject policy = configReader.getJsonObject( idPath );

            auto id = json_get_str( policy, db::ID_FIELD_NAME );
            using namespace Model::Resources;
            if( !dbClient.has( DBNames::PoliciesDB, JSON( db::ID_FIELD_NAME << id ) ) ) {
                PolicyInfo policyInfo;
                if( policyInfo.fromJson( policy ) ) {
                    auto data = policyInfo.toJson();
                    dbClient.insert( DBNames::PoliciesDB, data );
                }
            }
        }
        catch( const std::exception& ex ) {
            Log::error( "Failed to add predefined policy configs, exception at(" + std::to_string( i ) +
                        "), error :" + std::string( ex.what() ) );
        }
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
