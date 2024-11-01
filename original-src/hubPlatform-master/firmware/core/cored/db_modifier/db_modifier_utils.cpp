#include "db_modifier_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace DBModifierUtils {
Json addProtectionConfig( const Json& data, const Json& deviceInfo ) {
    Json updatedData = data;
    auto protectionConfig = json_get_str( deviceInfo, Model::Resources::DeviceFields::ProtectConfig );
    updatedData[ Model::Resources::DeviceFields::ProtectConfig ] = protectionConfig;
    return updatedData;
}

std::string getProtectConfig( IDbClient& dbClient, const std::string& itemId ) {
    std::string protectConfig;
    auto items = dbClient.findAll( HaItemsDBName, JSON_EMPTY_OBJ, nullptr );
    auto itemInfo =
        dbClient.findOne( HaItemsDBName, JSON( Model::Resources::ItemFields::IdFieldName << itemId ), nullptr );
    std::string deviceId;
    if( json_get( itemInfo, Model::Resources::ItemFields::DeviceIdFieldName, deviceId ) ) {
        const auto& deviceInfo = dbClient.findOne( Model::Resources::DBNames::DevicesDB,
                                                   JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ),
                                                   nullptr );
        protectConfig = json_get_str( deviceInfo, Model::Resources::DeviceFields::ProtectConfig );
    }

    return protectConfig;
}

}  // namespace DBModifierUtils
}  // namespace CoreD
}  // namespace HubPlatform
