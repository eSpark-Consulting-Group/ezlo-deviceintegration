#include "group_operations_utils.h"
#include "scenes_manager/modify_scene/scene_scale_modifier.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "cored/model/values/value_factory.h"
#include "config_reader/config_keys.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "cored/model/resources/cored_resources.h"
#include "item_group.h"
#include "device_group.h"
#include "device_group_validator.h"

namespace HubPlatform {
namespace CoreD {
namespace GroupOperationsUtils {

bool areStringArraysEqual( const JsonArray& leftArray, const JsonArray& rightArray ) {
    if( !leftArray.is_array() || !rightArray.is_array() )
        return false;

    if( leftArray.size() != rightArray.size() )
        return false;

    std::vector< std::string > left = json_get_vec< std::string >( leftArray );
    std::vector< std::string > right = json_get_vec< std::string >( rightArray );

    std::sort( begin( left ), end( left ) );
    std::sort( begin( right ), end( right ) );

    std::vector< std::string > difference;
    std::set_difference( begin( right ), end( right ), begin( left ), end( left ), std::back_inserter( difference ) );

    return difference.empty();
}

bool areObjectArraysEqual( const JsonArray& leftArray, const JsonArray& rightArray ) {
    if( leftArray.size() != rightArray.size() ) {
        return false;
    }

    for( const auto& elementUpdated : leftArray ) {
        bool found = false;
        for( const auto& elementSaved : rightArray ) {
            if( elementUpdated == elementSaved ) {
                found = true;
                break;
            }
        }
        if( !found ) {
            return false;
        }
    }

    return true;
}

void ensurePredefinedItemGroupsInDb( IDbClient& dbClient, const ConfigReader& configReader ) {
    ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::PreDefineItemGroups );
    std::size_t size = configReader.getArraySize( path );
    for( std::size_t i = 0; i < size; ++i ) {
        try {
            ConfigReader::Path idPath = path / i;
            const JsonObject itemGroup = configReader.getJsonObject( idPath );
            auto id = json_get_str( itemGroup, db::ID_FIELD_NAME );
            using namespace Model::Resources;
            if( !dbClient.has( DBNames::ItemGroupsDB, JSON( db::ID_FIELD_NAME << id ) ) ) {
                ItemGroupInfo info;
                info.fromJson( itemGroup );
                auto requestParams = info.toJson();
                dbClient.insert( DBNames::ItemGroupsDB, requestParams );
            }
        }
        catch( const std::exception& ex ) {
            Log::error( "Failed to add predefined item groups, exception at(" + std::to_string( i ) +
                        "),msg :" + std::string( ex.what() ) );
        }
    }
}

void ensurePredefinedDeviceGroupsInDb( IDbClient& dbClient, const ConfigReader& configReader ) {
    ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::PreDefineDeviceGroups );
    std::size_t size = configReader.getArraySize( path );
    for( std::size_t i = 0; i < size; ++i ) {
        try {
            ConfigReader::Path idPath = path / i;
            const JsonObject deviceGroup = configReader.getJsonObject( idPath );
            DeviceGroupValidator validator( dbClient );
            auto answer = validator.validateCreateGroupRequest( deviceGroup, true );
            if( !answer.empty() ) {
                LOGGER_ERROR( cc::error( "Device group validation failed: " ) + cc::j( answer ) );
                return;
            }
            auto id = json_get_str( deviceGroup, db::ID_FIELD_NAME );
            using namespace Model::Resources;
            if( !dbClient.has( DBNames::DeviceGroupsDB, JSON( db::ID_FIELD_NAME << id ) ) ) {
                DeviceGroupInfo info;
                info.fromJson( deviceGroup );
                auto requestParams = info.toJson();
                dbClient.insert( DBNames::DeviceGroupsDB, requestParams );
            }
        }
        catch( const std::exception& ex ) {
            Log::error( "Failed to add predefined device groups, exception at(" + std::to_string( i ) +
                        "), error :" + std::string( ex.what() ) );
        }
    }
}

}  // namespace GroupOperationsUtils
}  // namespace CoreD
}  // namespace HubPlatform
