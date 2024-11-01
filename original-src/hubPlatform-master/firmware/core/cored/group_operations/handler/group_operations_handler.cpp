#include "group_operations_handler.h"
#include "cored/group_operations/device_group_validator.h"
#include "cored/group_operations/item_group_validator.h"
#include "cored/group_operations/device_group_updater.h"
#include "cored/group_operations/item_group_updater.h"
#include "cored/group_operations/resources.h"
#include "cored/group_operations/device_group.h"
#include "cored/group_operations/item_group.h"
#include "cored/group_operations/device_group_find_handler.h"
#include "cored/model/resources/cored_resources.h"
#include "crypto_utils/digests.h"
#include "base_utils/ezlodb.h"
#include "cored/group_operations/group_db_reader.h"
#include "common_utils/cpp14.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "jsonrpc/jsonrpc.h"
#include <unistd.h>

namespace HubPlatform {
namespace CoreD {
using namespace Resources;

const auto ID = "_id";

IDbClient& GroupOperationsHandler::getDBClient() {
    return m_dbClient;
}

GroupOperationsHandler::GroupOperationsHandler( IDbClient& dbClient,
                                                BroadcastCallback& sendBroadcast,
                                                CoreDRemoteInterface::CoreDInterface& core )
: m_dbClient( dbClient )
, m_sendBroadcast( sendBroadcast )
, m_core( core ) {}

JsonObject GroupOperationsHandler::validateCreateGroupRequest( const JsonObject& message, bool checkRequiredParams ) {
    DeviceGroupValidator validator( m_dbClient );
    return validator.validateCreateGroupRequest( message, checkRequiredParams );
}

JsonObject GroupOperationsHandler::getDeviceGroupUpdateBroadcastData( const JsonObject& updatedData,
                                                                      const JsonObject& savedData ) {
    return DeviceGroupUpdater::getDeviceGroupUpdateBroadcastData( updatedData, savedData );
}

JsonObject GroupOperationsHandler::getItemGroupUpdateBroadcastData( const JsonObject& updatedData,
                                                                    const JsonObject& savedData ) {
    return ItemGroupUpdater::getItemGroupUpdateBroadcastData( updatedData, savedData );
}

bool GroupOperationsHandler::isExistDeviceGroupWithName( const std::string& name ) {
    Json query = JSON( "name" << name );
    auto deviceGroups = getDBClient().findAll( Model::Resources::DBNames::DeviceGroupsDB, query, nullptr );
    return !deviceGroups.empty();
}

JsonObject GroupOperationsHandler::validateItemGroupRequest( const JsonObject& message, bool checkRequiredParams ) {
    return ItemGroupValidator::validateCreateItemGroupRequest( message, checkRequiredParams );
}

JsonObject GroupOperationsHandler::processCreateDeviceGroupRequest( const JsonObject& params ) {
    using namespace Model::Resources;

    auto answer = validateCreateGroupRequest( params, true );
    if( !answer.empty() ) {
        LOGGER_ERROR( cc::error( "Device group validation failed: " ) + cc::j( answer ) );
        return answer;
    }
    DeviceGroupInfo info;
    info.fromJson( params );
    info.setId( Db::genOid() );
    auto requestParams = info.toJson();
    if( !isExistDeviceGroupWithName( info.getName() ) ) {
        getDBClient().insert( DBNames::DeviceGroupsDB, requestParams );
        if( !info.getDevices().empty() ) {
            requestParams[ DeviceGroup::AddedDevices ] = info.getDevices();
        }
        if( !info.getExceptions().empty() ) {
            requestParams[ DeviceGroup::AddedExceptions ] = info.getExceptions();
        }
        if( m_sendBroadcast != nullptr ) {
            m_sendBroadcast( DEVICE_GROUP_CREATED, requestParams );
        }
    }
    else {
        setErrorWithField(
            answer, Errors::SUCH_NAME_ALREADY_EXISTS, "The device group with the same name is exist", "" );
    }

    return answer;
}

JsonObject GroupOperationsHandler::processDeleteDeviceGroupRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;

    Json answer;

    JsonObject idObject;
    if( json_get( params, DeviceGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Id );
            return answer;
        }
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }

        bool force = json_get_bool( params, "force" );
        auto id = idObject.get< std::string >();
        auto groupId = JSON( ID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::DeviceGroupsDB, groupId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", ID );
        }
        else {
            DeviceGroupInfo info;
            info.fromJson( dbItem );

            if( force || !info.getPersistent() ) {
                if( !info.getDevices().empty() ) {
                    groupId[ DeviceGroup::RemovedDevices ] = info.getDevices();
                }
                if( !info.getExceptions().empty() ) {
                    groupId[ DeviceGroup::RemovedExceptions ] = info.getExceptions();
                }
                groupId[ DeviceGroup::Role ] = info.getRole();
                getDBClient().remove( DBNames::DeviceGroupsDB, groupId );
                if( m_sendBroadcast != nullptr ) {
                    m_sendBroadcast( DEVICE_GROUP_DELETED, groupId );
                }
            }
            else {
                setErrorWithField(
                    answer, Errors::DELETE_GROUP_IMPOSSIBLE, "Can't delete persistent device group", "" );
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Id );
    }

    return answer;
}

JsonObject GroupOperationsHandler::processUpdateDeviceGroupRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    Json answer;
    JsonObject idObject;
    if( json_get( params, DeviceGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Id );
            return answer;
        }
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }
        auto id = idObject.get< std::string >();

        if( json_contains( params, DeviceGroup::Role ) ) {
            setErrorWithField(
                answer, Errors::CONSTANT_PARAMS, "Read only field, can't be changed", DeviceGroup::Role );
            return answer;
        }
        if( json_contains( params, DeviceGroup::PackageId ) ) {
            setErrorWithField(
                answer, Errors::CONSTANT_PARAMS, "Read only field, can't be changed", DeviceGroup::PackageId );
            return answer;
        }
        if( json_contains( params, DeviceGroup::EntryDelay ) ) {
            auto enumCheck = EntryDelay::fromStringEntryDelay( json_get_str( params, DeviceGroup::EntryDelay ) );
            if( enumCheck == EntryDelay::EntryDelayEnum::invalid ) {
                setErrorWithField(
                    answer, Errors::WRONG_PARAMS, "Invalid value for entryDelay enum", DeviceGroup::EntryDelay );
                return answer;
            }
        }

        auto groupId = JSON( ID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::DeviceGroupsDB, groupId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", ID );
        }
        else {
            answer = validateCreateGroupRequest( params, false );
            if( answer.empty() ) {
                DeviceGroupInfo info;
                info.fromJson( dbItem );
                auto savedGroup = info.toJson();
                Json updatedJson = DeviceGroupUpdater::getDeviceGroupUpdateData( savedGroup, params );
                getDBClient().update( DBNames::DeviceGroupsDB, JSON( ID << id ), JSON( "$set" << updatedJson ) );
                auto updatedObject = getDeviceGroupUpdateBroadcastData( params, savedGroup );
                if( !updatedObject.empty() && m_sendBroadcast != nullptr ) {
                    updatedObject[ ID ] = id;
                    if( json_get_bool( updatedObject, DeviceGroup::Sync ) == false ) {
                        updatedObject[ DeviceGroup::Name ] = info.getName();
                    }

                    m_sendBroadcast( DEVICE_GROUP_UPDATED, updatedObject );
                }
            }
            else {
                return answer;
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "", DeviceGroup::Id );
    }

    return answer;
}

JsonObject GroupOperationsHandler::processAddOrRemoveDevicesDeviceGroupRequest( const JsonObject& params,
                                                                                bool addNotRemove ) {
    using namespace Resources;
    using namespace Model::Resources;
    Json answer;
    JsonObject idObject;
    if( json_get( params, DeviceGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Id );
            return answer;
        }
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }
        auto id = idObject.get< std::string >();
        auto groupId = JSON( ID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::DeviceGroupsDB, groupId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", ID );
        }
        else {
            Json request;
            if( json_get_arr( params, DeviceGroup::Devices, request ) ) {
                Json devices;
                json_get_arr( dbItem, DeviceGroup::Devices, devices );
                for( const auto& device : request ) {
                    /* if adding device and not already present... */
                    if( !json_array_is_string_present( devices, device ) && addNotRemove ) {
                        devices.push_back( device );
                    }
                    /* if removing a device and is present )... */
                    if( json_array_is_string_present( devices, device ) && !addNotRemove ) {
                        for( auto ptr = devices.begin(); ptr != devices.end(); ) {
                            if( ptr.value() == device ) {
                                ptr = devices.erase( ptr );
                            }
                            else {
                                ptr++;
                            }
                        }
                    }
                }
                Json command;
                json_add_str( command, DeviceGroup::Id, id );
                json_add_str( command, DeviceGroup::Name, json_get_str( dbItem, DeviceGroup::Name ) );
                json_add_str_array( command, DeviceGroup::Devices, devices );
                answer = processUpdateDeviceGroupRequest( command );
            }
            else {
                setErrorWithField( answer, Errors::WRONG_PARAMS, "", DeviceGroup::Devices );
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "", DeviceGroup::Id );
    }
    return answer;
}

JsonObject GroupOperationsHandler::processDeviceGroupsListRequest( const JsonObject& params ) {
    using namespace Model::Resources;
    DeviceGroupInfo deviceGroup;
    return processGroupOperationsListRequest( params, DBNames::DeviceGroupsDB, "deviceGroups", deviceGroup );
}

JsonObject GroupOperationsHandler::processDeviceGroupGetRequest( const JsonObject& params ) {
    using namespace Model::Resources;
    DeviceGroupInfo deviceGroup;
    return processGroupOperationsGetRequest( params, DBNames::DeviceGroupsDB, deviceGroup );
}

JsonObject GroupOperationsHandler::processCreateItemGroupRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;
    auto error = validateItemGroupRequest( params, true );
    if( error.empty() ) {
        ItemGroupInfo info;
        info.fromJson( params );
        info.setId( Db::genOid() );
        auto requestParams = info.toJson();
        getDBClient().insert( DBNames::ItemGroupsDB, requestParams );
        if( m_sendBroadcast != nullptr ) {
            m_sendBroadcast( ITEM_GROUP_CREATED, requestParams );
        }
    }

    return error;
}

JsonObject GroupOperationsHandler::processDeleteItemGroupRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;

    Json answer;

    JsonObject idObject;
    if( json_get( params, ItemGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "", ItemGroup::Id );
            return answer;
        }
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }

        bool force = json_get_bool( params, "force" );
        auto id = idObject.get< std::string >();
        auto groupId = JSON( ID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::ItemGroupsDB, groupId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", ID );
        }
        else {
            ItemGroupInfo info;
            info.fromJson( dbItem );

            if( force || !info.getPersistent() ) {
                getDBClient().remove( DBNames::ItemGroupsDB, groupId );
                if( m_sendBroadcast != nullptr ) {
                    m_sendBroadcast( ITEM_GROUP_DELETED, groupId );
                }
            }
            else {
                setErrorWithField( answer, Errors::DELETE_GROUP_IMPOSSIBLE, "Can't delete persistent item group", "" );
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "", ItemGroup::Id );
    }

    return answer;
}

JsonObject GroupOperationsHandler::processItemGroupsListRequest( const JsonObject& params ) {
    using namespace Model::Resources;
    ItemGroupInfo itemGroup;
    return processGroupOperationsListRequest( params, DBNames::ItemGroupsDB, "itemGroups", itemGroup );
}

JsonObject GroupOperationsHandler::processItemGroupGetRequest( const JsonObject& params ) {
    using namespace Model::Resources;
    ItemGroupInfo itemGroup;
    return processGroupOperationsGetRequest( params, DBNames::ItemGroupsDB, itemGroup );
}

JsonObject GroupOperationsHandler::processDeviceItemGroupExpandRequest( const JsonObject& params ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    Json answer;
    JsonArray devices;
    auto deviceGroupId = json_get_str( params, "deviceGroupId" );
    if( deviceGroupId.empty() ) {
        LOGGER_ERROR( cc::error( "Device group id not found" ) );
        setErrorWithField( answer, Errors::BAD_PARAMS, "Bad request, field does not exist", "deviceGroupId" );
        return answer;
    }
    auto itemGroupId = json_get_str( params, "itemGroupId" );
    auto showItems = json_get_bool( params, "showItems" );

    auto checkAndGetVersion = []( const Json& params ) -> std::pair< bool, std::string > {
        Json version = JSON_EMPTY_OBJ;
        if( json_get( params, "version", version ) && !version.empty() ) {
            if( !version.is_string() )
                return { false, "" };
            return { true, json_get_str( version ) };
        }
        return { true, "" };
    };

    bool status = false;
    std::string versionParams;
    std::tie( status, versionParams ) = checkAndGetVersion( params );
    if( !status ) {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "version" );
        return answer;
    }

    using namespace Resources;
    JsonObject dbDeviceGroup = GroupOperations::DbReader::fetchSingleDeviceGroup(
        m_dbClient, JSON( DeviceFields::IdFieldName << deviceGroupId ) );

    if( dbDeviceGroup.empty() ) {
        LOGGER_ERROR( cc::error( "Device group does not exist" ) );
        setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Device group doesn't exist", "deviceGroup" );
        return answer;
    }

    auto saveItemRecord = []( const Json& singleItem, JsonArray& items ) -> void {
        items.push_back( JSON( "itemId" << json_get_str( singleItem, ItemFields::IdFieldName ) << "name"
                                        << json_get_str( singleItem, ItemFields::NameFieldName ) ) );
    };

    auto devicesArray = expandDeviceGroup( dbDeviceGroup );
    if( itemGroupId.empty() ) {
        for( auto& singleDevice : devicesArray ) {
            JsonArray items;
            auto deviceId = json_get_str( singleDevice, DeviceFields::IdFieldName );
            auto deviceName = json_get_str( singleDevice, DeviceFields::NameFieldName );
            auto itemsArray = m_core.getItemsBy( JSON( ItemFields::DeviceIdFieldName << deviceId ) );
            if( showItems ) {
                for( auto& singleItem : itemsArray ) {
                    saveItemRecord( singleItem, items );
                }
                devices.push_back( JSON( "deviceId" << deviceId << "name" << deviceName << "items" << items ) );
            }
            else {
                devices.push_back( JSON( "deviceId" << deviceId << "name" << deviceName ) );
            }
        }
        answer[ "devices" ] = devices;
    }
    else {
        auto dbItemGroup = GroupOperations::DbReader::fetchSingleItemGroup(
            m_dbClient, JSON( ItemFields::IdFieldName << itemGroupId ) );
        if( dbItemGroup.empty() ) {
            LOGGER_ERROR( cc::error( "Item group does not exist" ) );
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Item group doesn't exist", "itemGroup" );
            return answer;
        }
        auto itemsArray = expandItemGroup( dbItemGroup, devicesArray );

        auto compareLessById = []( const Json& x, const Json& y ) -> bool {
            return json_get_str( x, ID ) < json_get_str( y, ID );
        };
        std::stable_sort( begin( itemsArray ), end( itemsArray ), compareLessById );

        for( auto& singleDevice : devicesArray ) {
            JsonArray items;
            auto deviceId = json_get_str( singleDevice, DeviceFields::IdFieldName );
            auto deviceName = json_get_str( singleDevice, DeviceFields::NameFieldName );
            auto itemsRawArray = m_core.getItemsBy( JSON( ItemFields::DeviceIdFieldName << deviceId ) );
            for( auto& singleItem : itemsRawArray ) {
                const auto idFieldIt = singleItem.find( ID );
                if( idFieldIt != singleItem.end() ) {
                    // search: replace n^2 with lon(n) complexity
                    if( std::binary_search( begin( itemsArray ), end( itemsArray ), singleItem, compareLessById ) ) {
                        saveItemRecord( singleItem, items );
                    }
                }
            }
            devices.push_back( JSON( "deviceId" << deviceId << "name" << deviceName << "items" << items ) );
        }
        answer[ "devices" ] = devices;
    }

    if( !devices.empty() ) {
        std::stringstream ss;
        ss << answer << getpid();

        auto hash = CryptoUtils::Digests::sha1( ss );

        if( !versionParams.empty() && hash == versionParams ) {
            return JSON( "version" << hash );
        }
        answer[ "version" ] = hash;
    }
    return answer;
}

std::vector< JsonObject > GroupOperationsHandler::expandDeviceGroup( const JsonObject& deviceGroup ) {
    using namespace Resources;

    JsonArray devices;
    json_get_arr( deviceGroup, DeviceGroup::Devices, devices );
    JsonArray exceptions;
    json_get_arr( deviceGroup, DeviceGroup::Exceptions, exceptions );

    JsonArray categoryArray;
    json_get_arr( deviceGroup, DeviceGroup::Categories, categoryArray );

    JsonArray deviceCategoryArray;
    JsonArray queryArray;
    using namespace CoreD::Model::Resources;
    for( int i = 0; i < categoryArray.size(); i++ ) {
        JsonArray deviceSubcategoryArray;
        deviceCategoryArray[ i ] = json_get_str( categoryArray[ i ], DeviceGroup::Category );
        json_get_arr( categoryArray[ i ], DeviceGroup::Subcategories, deviceSubcategoryArray );
        if( !exceptions.empty() ) {
            for( int x = 0; x < exceptions.size(); x++ ) {
                queryArray[ i ] =
                    JSON( DeviceGroup::Category << deviceCategoryArray[ i ] << DeviceGroup::Subcategory
                                                << JSON( "$in" << deviceSubcategoryArray ) << DeviceFields::IdFieldName
                                                << JSON( "$ne" << exceptions[ x ] ) );
            }
        }
        else {
            queryArray[ i ] = JSON( DeviceGroup::Category << deviceCategoryArray[ i ] << DeviceGroup::Subcategory
                                                          << JSON( "$in" << deviceSubcategoryArray ) );
        }
    }

    if( !devices.empty() ) {
        for( int i = 0; i < devices.size(); i++ ) {
            queryArray.push_back( JSON( DeviceFields::IdFieldName << devices[ i ] ) );
        }
    }

    auto devicesArray = m_core.getDevicesBy( JSON( "$or" << queryArray ) );

    return devicesArray;
}

std::vector< JsonObject > GroupOperationsHandler::expandItemGroup( const JsonObject& itemGroup,
                                                                   const std::vector< JsonObject >& devicesArray ) {
    using namespace CoreD::Model::Resources;

    using namespace Resources;

    JsonArray deviceIdsArray;
    for( int i = 0; i < devicesArray.size(); i++ ) {
        deviceIdsArray[ i ] = json_get_str( devicesArray.at( i ), DeviceFields::IdFieldName );
    }

    JsonArray itemNamesArray;
    json_get_arr( itemGroup, ItemGroup::ItemNames, itemNamesArray );

    auto query = JSON( ItemFields::DeviceIdFieldName << JSON( "$in" << deviceIdsArray ) << ItemFields::NameFieldName
                                                     << JSON( "$in" << itemNamesArray ) );

    auto itemsArray = m_core.getItemsBy( query );
    return itemsArray;
}

JsonObject GroupOperationsHandler::processDeviceGroupFindRequest( const JsonObject& params ) {
    return processGroupOperationsFindRequest( params, Model::Resources::DBNames::DeviceGroupsDB );
}

JsonObject GroupOperationsHandler::processUpdateItemGroupRequest( const JsonObject& params ) {
    using namespace Resources;
    using namespace Model::Resources;

    Json answer;
    JsonObject idObject;
    if( json_get( params, ItemGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", ItemGroup::Id );
            return answer;
        }
        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }
        auto id = idObject.get< std::string >();

        auto groupId = JSON( ID << id );
        JsonObject dbItem = getDBClient().findOne( DBNames::ItemGroupsDB, groupId, nullptr );
        if( dbItem.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "Record with the key does not exist", ID );
        }
        else {
            answer = validateItemGroupRequest( params, false );
            if( answer.empty() ) {
                ItemGroupInfo info;
                info.fromJson( dbItem );
                auto savedGroup = info.toJson();
                Json updatedJson = ItemGroupUpdater::getItemGroupUpdateData( savedGroup, params );
                getDBClient().update( DBNames::ItemGroupsDB, JSON( ID << id ), JSON( "$set" << updatedJson ) );
                auto updatedObject = getItemGroupUpdateBroadcastData( params, savedGroup );
                if( !updatedObject.empty() && m_sendBroadcast != nullptr ) {
                    updatedObject[ ID ] = id;

                    if( json_get_bool( updatedObject, ItemGroup::Sync ) == false ) {
                        updatedObject[ ItemGroup::Name ] = info.getName();
                    }

                    m_sendBroadcast( ITEM_GROUP_UPDATED, updatedObject );
                }
            }
            else {
                return answer;
            }
        }
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", ItemGroup::Id );
    }

    return answer;
}

JsonObject GroupOperationsHandler::processGroupOperationsListRequest( const JsonObject& params,
                                                                      const char* dbName,
                                                                      const char* groupName,
                                                                      GroupInfo& groupInfo ) {
    using namespace Resources;
    Json answer;
    std::set< std::string > ids;
    JsonArray strings;
    JsonObject stringsObj;
    if( json_get( params, DeviceGroup::Ids, strings ) ) {
        if( !strings.is_array() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Ids );
        }
        try {
            fromJsonStringArray( strings, [&ids]( const std::string& value ) { ids.emplace( value ); } );
        }
        catch( std::exception& ex ) {
            std::string reason = "Wrong value type: ";
            reason += ex.what();
            setErrorWithField( answer, Errors::WRONG_PARAMS, reason, DeviceGroup::Ids );
            return answer;
        }
    }
    std::string versionParams;
    Json version = params[ "version" ];
    if( !version.empty() ) {
        if( !version.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", "version" );
            return answer;
        }
        versionParams = json_get_str( params, "version" );
    }

    auto groups = getDBClient().findAll( dbName, JSON_EMPTY_OBJ, nullptr );
    JsonArrayBuilder filteredGroups;
    std::stringstream ss;

    auto processParams = [&filteredGroups, &ss]( const JsonObject& group ) {
        filteredGroups << group;
        ss << group;
    };

    for( const auto& group : groups ) {
        groupInfo.fromJson( group );
        auto id = json_get_str( group, ID );
        if( !ids.empty() && ids.count( id ) ) {
            processParams( groupInfo.toJson() );
        }
        else if( ids.empty() ) {
            processParams( groupInfo.toJson() );
        }
    }

    ss << getpid();

    auto hash = CryptoUtils::Digests::sha1( ss );

    if( !version.empty() && hash == versionParams ) {
        return JSON( "version" << hash );
    }

    return JSON( "version" << hash << groupName << filteredGroups );
}

JsonObject GroupOperationsHandler::processGroupOperationsGetRequest( const JsonObject& params,
                                                                     const char* dbName,
                                                                     GroupInfo& groupInfo ) {
    using namespace Resources;
    Json answer;

    JsonObject idObject;
    if( json_get( params, DeviceGroup::Id, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::Id );
            return answer;
        }

        if( idObject.empty() ) {
            setError( answer, Errors::ID_IS_EMPTY );
            return answer;
        }
        auto id = idObject.get< std::string >();
        answer = getDBClient().findOne( dbName, JSON( ID << id ), nullptr );

        if( answer.empty() ) {
            setErrorWithField( answer, Errors::DOES_NOT_EXIST, "", ID );
            return answer;
        }
        groupInfo.fromJson( answer );
        answer = groupInfo.toJson();
    }
    else {
        setErrorWithField( answer, Errors::WRONG_PARAMS, "Not found", DeviceGroup::Id );
        return answer;
    }

    return answer;
}

JsonObject GroupOperationsHandler::processGroupOperationsFindRequest( const JsonObject& params, const char* dbName ) {
    DeviceGroupFindHandler findHandler( getDBClient(), dbName );
    return findHandler.processFindRequest( params );
}
}  // namespace CoreD
}  // namespace HubPlatform
