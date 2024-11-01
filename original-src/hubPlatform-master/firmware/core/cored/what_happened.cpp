#include "base_utils/ezlodb.h"
#include "log_utils/ezlo_log.h"
#include "base_utils/timeutils.h"
#include "base_utils/acl.h"
#include "base_utils/fo_utils.h"
#include "database_client/db_tables_names.h"
#include "json_utils/json_wrapper.h"
#include "database_client/dbqueryparser.h"
#include "cored/what_happened.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace CoreD {

namespace WhatHappendedImpl {

static const std::set< std::string > allowedDataTypes = { "battery",    "sensor", "dwsensor", "alarm",  "water",
                                                          "alarm_door", "motion", "switch",   "remote", "door_lock" };

void addTimeOffset( JsonObject& jsonData ) {
    jsonData[ "offset" ] = std::to_string( TimeUtils::getCurrentOffset() / 3600 );
}

// TODO: reimplement it with the usage of CoreService
bool getRoomName( IDbClient& client, const std::string& pageId, std::string& result ) {
    const JsonObject fieldsToReturn = JSON( "name" << 1 );
    Json page = client.findOne( HaRoomsDBName, JSON( db::ID_FIELD_NAME << pageId ), &fieldsToReturn );
    return json_get( page, "name", result );
}

JsonBuilder filterGetQuery( const JsonObject& params ) {
    JsonBuilder queryBuilder;
    JsonBuilder typeFilterBuilder;
    std::vector< std::string > labelFilter;
    JsonArray roomsFilter, typeFilter, deviceFilter;
    if( json_get( params, "roomsFilter", roomsFilter ) && !roomsFilter.empty() )
        queryBuilder << "parent_id" << JSON( "$in" << roomsFilter );
    if( json_get( params, "deviceFilter", deviceFilter ) && !deviceFilter.empty() )
        queryBuilder << "deviceId" << JSON( "$in" << deviceFilter );
    if( json_get( params, "typeFilter", typeFilter ) && !typeFilter.empty() ) {
        for( const std::string& type : typeFilter ) {
            if( type == "all" ) {
                typeFilterBuilder.clear();
                labelFilter.clear();
                break;
            }
            else if( type == "devices" ) {
                typeFilterBuilder << "data_type" << JSON( "$in" << allowedDataTypes );
            }
            else if( type == "rules" ) {
                labelFilter.push_back( "Rule" );
            }
            else if( type == "users" ) {
                labelFilter.push_back( "user" );
            }
        }
    }
    if( !typeFilterBuilder.empty() || !labelFilter.empty() ) {
        JsonArray filterArray;
        if( !typeFilterBuilder.empty() )
            filterArray.push_back( typeFilterBuilder.obj() );
        if( !labelFilter.empty() )
            filterArray.push_back( JSON( "label" << JSON( "$in" << labelFilter ) ) );
        queryBuilder << "$or" << filterArray;
    }
    return queryBuilder;
}

JsonObject saveFilteredLog( IDbClient& client, const JsonObject& itemUpdated, const std::string& source = "" ) {
    std::string type, label, roomName, pageId, dataType, devName, deviceId;
    JsonObject itemValuesEnum, device, whatHappenedData;
    Json jsValue;

    // TODO: refactor it with CoreDService
    JsonObject item = FoUtils::getItemById( json_get_str( itemUpdated, db::ID_FIELD_NAME ) );
    deviceId = json_get_str( item, "deviceId" );
    device = FoUtils::getDeviceById( deviceId );
    deviceId = json_get_str( item, "deviceId" );
    if( !json_get_bool( item, "ready" ) || !json_get( item, "pageId", pageId ) ||
        !json_get( device, "name", devName ) || !json_get( item, "name", dataType ) ||
        !json_get( item, "type", type ) || !json_get( item, "label", label ) || !json_get( item, "value", jsValue ) ) {
        throw std::runtime_error( "value should not be updated" );
    }
    static const std::map< bool, std::string > getSwitchMap = { { true, "ezlo.what_happened.value.on" },
                                                                { false, "ezlo.what_happened.value.off" } };
    static const std::map< bool, std::string > getMotionMap = { { true, "ezlo.what_happened.value.move" },
                                                                { false, "ezlo.what_happened.value.still" } };
    static const std::map< bool, std::string > getDwsensorMap = { { true, "ezlo.what_happened.value.open" },
                                                                  { false, "ezlo.what_happened.value.close" } };
    static const std::map< std::string, std::map< bool, std::string > > dataTypesMap = {
        { "switch", getSwitchMap },
        { "dwsensor", getDwsensorMap },
        { "motion", getMotionMap },
        { "battery", {} },
    };

    if( allowedDataTypes.count( dataType ) < 1 || !getRoomName( client, pageId, roomName ) ) {
        throw std::runtime_error( "value should not be updated" );
    }

    // TODO: really strange logic. Should be removed !!!
    // Make at least function composition !!!

    int value{ 0 };
    json_get( jsValue, value );
    if( dataType == "battery" ) {
        if( value > 15 )
            throw std::runtime_error( "value should not be updated" );
        if( value < 0 )
            value = 0;
        json_add_str( whatHappenedData, "valueLabel", "ezlo.what_happened.value.battery.level" );
        json_add_str( whatHappenedData, "valueData", std::to_string( value ) + "%" );
    }
    else if( dataTypesMap.count( dataType ) != 0 ) {
        json_add_str( whatHappenedData, "valueLabel", dataTypesMap.at( dataType ).at( value ) );
    }
    else {
        json_add_str( whatHappenedData, "valueLabel", json_write( jsValue ) );
    }
    whatHappenedData[ "data_type" ] = dataType;
    whatHappenedData[ "parent_id" ] = pageId;
    whatHappenedData[ "room" ] = roomName;
    whatHappenedData[ "label" ] = label;
    whatHappenedData[ "itemId" ] = json_get_str( item, db::ID_FIELD_NAME );
    whatHappenedData[ "dev_name" ] = devName;
    whatHappenedData[ "deviceId" ] = deviceId;
    whatHappenedData[ "value" ] = itemUpdated[ "value" ];
    whatHappenedData[ "type" ] = type;
    whatHappenedData[ "source" ] = source;
    addTimeOffset( whatHappenedData );
    return whatHappenedData;
    //        Db::insertWithAge( "db.firmware.cored.devicelog", whatHappenedData );
}

bool isMissedItemInList( IDbClient& client, const std::vector< std::string >& itemIdsList ) {
    JsonObject joDevice, fieldsToReturn = JSON( db::ID_FIELD_NAME << 1 );
    for( const auto& itemId : itemIdsList ) {
        joDevice = client.findOne( HaItemsDBName, JSON( db::ID_FIELD_NAME << itemId ), &fieldsToReturn );
        if( joDevice.empty() )
            return true;
    }
    return false;
}

bool isFailedItemInList( IDbClient& client, const std::vector< std::string >& itemIdsList ) {
    JsonObject joItem, fieldsToReturn = JSON( "reachable" << 1 << db::ID_FIELD_NAME << 0 );
    bool isReachable = true;
    for( const auto& itemId : itemIdsList ) {
        joItem = client.findOne( HaItemsDBName, JSON( db::ID_FIELD_NAME << itemId ), &fieldsToReturn );
        isReachable = json_get_bool( joItem, "reachable", true );
        if( !isReachable )
            return true;
    }
    return false;
}

std::string guessColor( const JsonObject& element ) {
    std::string device_state = json_get_str( element, "valueLabel" );
    int battery_level = json_get_int( element, "valueData" );

    if( device_state == "ezlo.what_happened.value.battery.level" && battery_level < 15 )
        return "red";
    if( device_state.find( "on" ) != std::string::npos || device_state.find( "open" ) != std::string::npos ||
        device_state.find( "move" ) != std::string::npos || device_state.find( "leak" ) != std::string::npos ||
        device_state.find( "smoke" ) != std::string::npos )
        return "red";
    return "green";
}

}  // namespace WhatHappendedImpl

using namespace WhatHappendedImpl;

WhatHappened::WhatHappened( IDbClient& client )
: dbClient( client ) {}

void WhatHappened::insertWithAge( const JsonObject& obj ) {
    dbClient.insert( LogWhatHappened, JSON( obj << "age" << TimeUtils::getCurrentIsoDate() ) );
}

void WhatHappened::saveDeviceLog( const JsonObject& deviceData, const MessageSubClasses msgSubclass ) {
    switch( msgSubclass ) {
    case HUB_DEVICE_ADDED:
        saveDeviceAdded( deviceData );
        break;
    case HUB_DEVICE_REMOVED:
        saveDeviceRemoved( deviceData );
        break;
    default:
        break;
    }
    return;
}

void WhatHappened::saveItemLog( const JsonObject& itemUpdated ) {
    JsonObject message = itemUpdated;
    json_remove( message, "valueFormatted" );
    try {
        insertWithAge( saveFilteredLog( dbClient, message ) );
    }
    catch( const std::exception& ) {
    }
}

void WhatHappened::saveRuleLog( const JsonObject& obj, const MessageSubClasses event ) {
    std::string id;
    std::string sender;
    if( json_get( obj, db::ID_FIELD_NAME, id ) && json_get( obj, "sender", sender ) ) {
        saveRuleLog( id, event, sender );
    }
    else {
        Log::error( "EventHandler::saveRuleLog, can't extract id/sender" );
    }
}

void WhatHappened::saveRuleLog( const std::string& ruleId,
                                const MessageSubClasses event,
                                const std::vector< std::string >& itemIdsList ) {
    if( isMissedItemInList( dbClient, itemIdsList ) )
        saveRuleLog( ruleId, RULE_EXECUTED_MISSED, "" );
    else if( isFailedItemInList( dbClient, itemIdsList ) )
        saveRuleLog( ruleId, RULE_EXECUTED_FAILED, "" );
    else
        saveRuleLog( ruleId, event, "" );
}

void WhatHappened::saveRuleLog( const std::string& ruleId, const MessageSubClasses event, const std::string& source ) {
    std::string roomName, roomId;
    // TODO: make the reference to the constexpr char for _id parent_id fields
    Json fields = JSON( "name" << 1 << "parent_id" << 1 << db::ID_FIELD_NAME << 0 );
    Json rule = dbClient.findOne( HaScenesDBName, JSON( db::ID_FIELD_NAME << ruleId ), &fields );
    if( rule.empty() ) {
        Log::error( cc::error( "saveRuleLog: Rule '" + ruleId + "' not found!" ) );
        return;
    }
    if( !json_get( rule, "parent_id", roomId ) || !getRoomName( dbClient, roomId, roomName ) )
        roomName = "[[ezlo.what_happened.room.global]]";
    rule[ "roomName" ] = roomName;
    rule[ "parent_id" ] = roomId;
    rule[ "source" ] = source;
    switch( event ) {
    case RULE_EXECUTED:
    case RULE_EXECUTED_FAILED:
    case RULE_EXECUTED_MISSED:
        saveRuleExecuted( rule, event );
        break;
    case SCENE_ADDED:
    case RULE_ADD:
        saveRuleChangeState( rule, CREATED );
        break;
    case SCENE_CHANGED:
    case RULE_CHANGE:
        saveRuleChangeState( rule, CHANGED );
        break;
    case SCENE_DELETED:
        saveRuleChangeState( rule, DELETED );
        break;
    default:
        return;
    }
}

void WhatHappened::saveRuleDeleted( const JsonObject& args ) {
    std::string roomName, source, ruleName, roomId;
    if( !json_get( args, "sender", source ) || !json_get( args, "name", ruleName ) )
        return;
    if( !json_get( args, "parent_id", roomId ) || !getRoomName( dbClient, roomId, roomName ) )
        roomName = "[[ezlo.what_happened.room.global]]";
    JsonObject rule;
    rule[ "roomName" ] = roomName;
    rule[ "name" ] = ruleName;
    rule[ "parent_id" ] = roomId;
    rule[ "source" ] = source;
    addTimeOffset( rule );
    saveRuleChangeState( rule, DELETED );
}

void WhatHappened::saveDeviceAdded( const JsonObject& deviceData ) {
    std::string deviceName, deviceId, dataType;
    if( !json_get( deviceData, "name", deviceName ) || !json_get( deviceData, db::ID_FIELD_NAME, deviceId ) ||
        !json_get( deviceData, "type", dataType ) )
        return;
    if( allowedDataTypes.count( dataType ) > 0 ) {
        JsonObject device;
        device[ "label" ] = "device";
        device[ "room" ] = "[[ezlo.what_happened.room.unassigned]]";
        device[ "source" ] = "";
        device[ "dev_name" ] = deviceName;
        device[ "data_type" ] = dataType;
        device[ "deviceId" ] = deviceId;
        device[ "valueLabel" ] = "ezlo.what_happened.value.added";
        addTimeOffset( device );
        insertWithAge( device );
    }
}

void WhatHappened::saveDeviceRemoved( const JsonObject& device ) {
    std::string deviceId, roomName, deviceName, dataType;
    if( !json_get( device, db::ID_FIELD_NAME, deviceId ) || !json_get( device, "type", dataType ) )
        return;
    if( allowedDataTypes.count( dataType ) > 0 ) {
        roomName = "[[ezlo.what_happened.room.unassigned]]";
        deviceName = json_get_str( device, "name" );
        if( deviceName == "" )
            deviceName = "[[ezlo.what_happened.device.unknown]]";
        JsonObject deviceData;
        deviceData[ "label" ] = "device";
        deviceData[ "room" ] = roomName;
        deviceData[ "parent_id" ] = "";
        deviceData[ "source" ] = "";
        deviceData[ "dev_name" ] = deviceName;
        deviceData[ "valueLabel" ] = "ezlo.what_happened.value.removed";
        addTimeOffset( deviceData );
        insertWithAge( deviceData );
    }
}

void WhatHappened::saveUserAssign( const JsonObject& args, bool state ) {
    std::string roomName, source, user, roomId;
    if( !json_get( args, db::ID_FIELD_NAME, roomId ) || !json_get( args, "sender", source ) ||
        !json_get( args, "user", user ) )
        return;
    if( !getRoomName( dbClient, roomId, roomName ) )
        roomName = "[[ezlo.what_happened.room.unassigned]]";
    JsonObject userObject;
    userObject[ "label" ] = "user";
    userObject[ "room" ] = roomName;
    userObject[ "parent_id" ] = roomId;
    userObject[ "source" ] = source;
    userObject[ "dev_name" ] = user;
    userObject[ "valueLabel" ] = state ? "ezlo.what_happened.value.assigned" : "ezlo.what_happened.value.unassigned";
    addTimeOffset( userObject );
    insertWithAge( userObject );
}

void WhatHappened::saveUserBind( const JsonObject& args, bool state ) {
    std::string source, user;
    if( !json_get( args, "sender", source ) || !json_get( args, "email", user ) )
        return;
    JsonObject userObject;
    userObject[ "label" ] = "user";
    userObject[ "room" ] = "ezlo";
    userObject[ "source" ] = source;
    userObject[ "dev_name" ] = user;
    userObject[ "valueLabel" ] = state ? "ezlo.what_happened.value.binded" : "ezlo.what_happened.value.unbinded";
    addTimeOffset( userObject );
    insertWithAge( userObject );
    //    Db::insertWithAge( "db.firmware.cored.devicelog", userObject );
}

void WhatHappened::getWhatHappened( const JsonObject& params, JsonObject& answer ) {
    long long jsTime, length = 0;
    int startRecords = 0, limitRecords = 0, second = 1000;
    if( !getField( params, "length", answer, length ) || !getField( params, "startRecords", answer, startRecords ) ||
        !getField( params, "limitRecords", answer, limitRecords ) ) {
        return;
    }

    JsonBuilder queryBuilder = filterGetQuery( params );
    if( !json_get( params, "startTime", jsTime ) ) {
        jsTime = ( static_cast< long long >( Db::getTime() ) * second ) - length;
    }
    jsTime += 60 * second;
    const int LIMIT_RECORDS = 50;
    if( limitRecords > LIMIT_RECORDS )
        limitRecords = LIMIT_RECORDS;
    const std::string startTime = Db::date( jsTime / second );
    const std::string endTime = Db::date( ( jsTime + length ) / second );
    queryBuilder << "age" << JSON( "$gte" << startTime << "$lte" << endTime );
    Db::Query query = Db::Query( queryBuilder.obj() ).sort( "age", -1 );
    Json fieldsToReturn = JSON( "age" << 1 << "dev_name" << 1 << "label" << 1 << "room" << 1 << "offset" << 1
                                      << "source" << 1 << "valueString" << 1 << "valueData" << 1 << "valueLabel" << 1
                                      << "parent_id" << 1 << "data_type" << 1 );
    JsonObject result;
    result[ "logs" ] = JSON_EMPTY_ARR;
    Json& logs = result[ "logs" ];
    std::cerr << queryBuilder.obj().dump( 2 );
    for( JsonObject& element : dbClient.findAll( LogWhatHappened, query, &fieldsToReturn, limitRecords, startRecords ) )
        if( Acl::checkPermission( json_get_str( element, "parent_id" ), params, Resource::UI, Permission::VIEW ) ) {
            json_add_str( element, "color", guessColor( element ) );
            element.erase( "parent_id" );
            logs.push_back( element );
        }
    result[ "total" ] = dbClient.count( LogWhatHappened, query.getFilter() );
    result[ "returned" ] = logs.size();
    answer[ "result" ] = result;
}

void WhatHappened::saveRuleChangeState( const JsonObject& rule, const RuleState state ) {
    if( rule.count( "source" ) == 0 )
        return;
    JsonObject ruleLog;
    ruleLog[ "label" ] = "Rule";
    ruleLog[ "room" ] = rule.at( "roomName" );
    ruleLog[ "source" ] = rule.at( "source" );
    ruleLog[ "dev_name" ] = rule.at( "name" );
    ruleLog[ "parent_id" ] = rule.at( "parent_id" );
    static const std::map< int, std::string > valueStrings = {
        { ENABLED, "ezlo.what_happened.value.enabled" }, { DISABLED, "ezlo.what_happened.value.disabled" },
        { CREATED, "ezlo.what_happened.value.created" }, { CHANGED, "ezlo.what_happened.value.changed" },
        { DELETED, "ezlo.what_happened.value.deleted" },
    };
    try {
        ruleLog[ "valueLabel" ] = valueStrings.at( state );
    }
    catch( ... ) {
    }
    addTimeOffset( ruleLog );
    insertWithAge( ruleLog );
    //    Db::insertWithAge( "db.firmware.cored.devicelog", ruleLog );
}

void WhatHappened::saveRuleExecuted( const JsonObject& rule, const MessageSubClasses event ) {
    JsonObject ruleLog;
    ruleLog[ "label" ] = "Rule";
    ruleLog[ "room" ] = rule.at( "roomName" );
    ruleLog[ "source" ] = "";
    ruleLog[ "dev_name" ] = rule.at( "name" );
    switch( event ) {
    case RULE_EXECUTED_MISSED:
        ruleLog[ "valueLabel" ] = "ezlo.what_happened.value.executed.missed";
        break;
    case RULE_EXECUTED_FAILED:
        ruleLog[ "valueLabel" ] = "ezlo.what_happened.value.executed.partly";
        break;
    default:
        ruleLog[ "valueLabel" ] = "ezlo.what_happened.value.executed";
    }
    addTimeOffset( ruleLog );
    insertWithAge( ruleLog );
    //    Db::insertWithAge( "db.firmware.cored.devicelog", ruleLog );
}

}  // namespace CoreD
}  // namespace HubPlatform
