
#include "uid/ruleblocks.h"

#include "base_utils/ezlodb.h"
#include "base_utils/fo_utils.h"
#include "base_utils/acl.h"
#include "database_client/db_tables_names.h"
#include "base_utils/utils.h"

using namespace utils;

namespace RuleBlocks {

const static int g_additionalThenBlocksCount = 2;

JsonArray getRoomDevicesArray( const JsonObject& params, const std::string& pageId ) {
    const Json fieldsToReturn = JSON( "_id" << 1 << "name" << 1 );
    Json query;
    if( pageId.empty() )
        query = JSON( "$or" << JSON_ARRAY( JSON( "pageId" << JSON( "$exists" << false ) )
                                           << JSON( "pageId" << JSON( "$eq" << JSON_ARRAY( "" ) ) ) ) );
    else
        query = JSON( "pageId" );

    return Db::getList(
        HaDevicesDBName,
        std::bind( &Acl::checkPermission, std::placeholders::_1, params, Resource::DEVICES, Permission::VIEW ),
        query,
        &fieldsToReturn );
}

JsonArray getRoomItemsMethodsArray( const JsonObject& params, const std::string& pageId ) {
    const Json fieldsToReturn = JSON( "_id" << 1 << "name" << 1 << "label" << 1 << "hasGetter" << 1 << "hasSetter" << 1
                                            << "private.showAsRule" << 1 << "private.instanceId" << 1 );
    Json query;
    if( pageId.empty() )
        query = JSON( "$or" << JSON_ARRAY( JSON( "pageId" << JSON( "$exists" << false ) )
                                           << JSON( "pageId" << JSON( "$eq" << JSON_ARRAY( "" ) ) )
                                           << "private.showAsRule" << true ) );
    else
        query = JSON( "pageId" << pageId << "private.showAsRule" << true );

    return Db::getList(
        HaItemsDBName,
        std::bind( &Acl::checkPermission, std::placeholders::_1, params, Resource::DEVICES, Permission::VIEW ),
        query,
        &fieldsToReturn );
}

JsonArray getDeviceItemsMethodsArray( const JsonObject& params, const std::string& deviceId ) {
    const Json fieldsToReturn = JSON( "_id" << 1 << "name" << 1 << "label" << 1 << "hasGetter" << 1 << "hasSetter" << 1
                                            << "private.showAsRule" << 1 << "private.instanceId" << 1 );
    Json query = JSON( "deviceId" << deviceId << "private.showAsRule" << true );
    return Db::getList(
        HaItemsDBName,
        std::bind( &Acl::checkPermission, std::placeholders::_1, params, Resource::DEVICES, Permission::VIEW ),
        query,
        &fieldsToReturn );
}

///@param[in] parentId Non-empty to query only dataitems with such parent id or empty to query *only* parentless
/// dataitems
JsonArray getRoomDevicesRuleBlocks( const JsonObject& params,
                                    const std::string& pageId,
                                    const std::string& blockType,
                                    const bool toItems ) {
    JsonArray jaResult = JSON_EMPTY_ARR;

    const JsonArray roomDevices = std::move( getRoomDevicesArray( params, pageId ) );

    for( const auto& device : roomDevices ) {
        JsonObject joResult;

        joResult[ "_id" ] = json_get_str( device, "_id" );
        joResult[ "name" ] = json_get_str( device, "name" );
        joResult[ "parent_id" ] = pageId;
        JsonArray methods = JSON_EMPTY_ARR;

        const JsonArray roomMethods = std::move( getDeviceItemsMethodsArray( params, json_get_str( device, "_id" ) ) );
        for( const auto& roomMethod : roomMethods ) {
            const std::string itemId = json_get_str( roomMethod, "_id" );
            const std::string itemName = json_get_str( roomMethod, "name" );

            if( !itemId.empty() && !itemName.empty() &&
                json_get_bool( roomMethod[ "private" ], "showAsRule", false ) ) {
                const std::string itemLabel = json_get_str( roomMethod, "label" );
                if( json_get_bool( roomMethod, "hasSetter", false ) && blockType == "then" ) {
                    JsonObject jo = JSON( "name" << std::string( "set_" ).append( itemName ) << "data_type" << itemName
                                                 << "hr_name" << itemLabel );
                    JsonArray resultInstances = JSON_EMPTY_ARR;
                    resultInstances.push_back( JSON( "id" << json_get_int( roomMethod[ "private" ], "instanceId", -1 )
                                                          << "itemId" << itemId << "title" << itemLabel ) );
                    jo[ "instances" ] = resultInstances;
                    methods.push_back( jo );
                }

                if( json_get_bool( roomMethod, "hasGetter", false ) && blockType == "when" ) {
                    JsonObject jo = JSON( "name" << std::string( "get_" ).append( itemName ) << "data_type" << itemName
                                                 << "hr_name" << itemLabel );
                    JsonArray resultInstances = JSON_EMPTY_ARR;
                    resultInstances.push_back( JSON( "id" << json_get_int( roomMethod[ "private" ], "instanceId", -1 )
                                                          << "itemId" << itemId << "title" << itemLabel ) );
                    jo[ "instances" ] = resultInstances;
                    methods.push_back( jo );
                }
            }
        }
        joResult[ "methods" ] = methods;
        jaResult.push_back( joResult );
    }

    std::cout << ( stringify( "\njaResult: ", json_write( jaResult ), "\n" ) );
    return jaResult;
}

}  // namespace RuleBlocks

JsonArray RuleBlocks::getList( const JsonObject& params,
                               const std::string& blockType,
                               const std::string& conditionFilter,
                               const std::string& pageId,
                               const bool toItems ) {
    if( conditionFilter == "Device" )
        return getRoomDevicesRuleBlocks( params, pageId, blockType, toItems );
    return JSON_EMPTY_ARR;
}

size_t RuleBlocks::getBlocksCount( const std::string& blockType, const std::string& parentId ) {
    size_t result = 0;
    if( blockType == "when" ) {
        result = Db::count( HaItemsDBName,
                            JSON( "hasGetter" << true << "private.showAsRule" << true << "pageId" << parentId ) );
    }
    else {
        const auto objects = Db::findAll(
            HaItemsDBName, JSON( "hasSetter" << true << "private.showAsRule" << true << "pageId" << parentId ) );
        result = objects.size();
        for( const auto& obj : objects ) {
            std::string type, name;
            name = json_get_str( obj, "name" );
            if( name == "dimmer" )
                result += g_additionalThenBlocksCount;
        }
        result += g_additionalThenBlocksCount;
    }
    return result;
}
