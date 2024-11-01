#include "cloud_devices/subscription_manager/subscription_manager.h"

#include "cloud_devices/subscription_manager/subscription_utils.h"
#include "cloud_devices/subscription_manager/subscription_exceptions.h"
#include "cloud_devices/resources/cloud_devices_resources.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/values/value.h"
#include "cored/model/resources/cored_resources.h"

#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "service_interfaces/groups_names.h"
#include "cored/remote_interface/cored_interface_factory.h"

#include <deque>
#include <typeinfo>

namespace HubPlatform {
namespace CloudDevices {

namespace {
std::string getStringValueFromJson( const Json& j, const std::string& key ) {
    std::string value = json_get_str( j, key );
    if( value.empty() )
        throw SubscriptionError( "Invalid field: " + key );
    return value;
}

template < typename T >
class Optional {
public:
    Optional() = default;
    Optional( T&& value ) {
        m_value = std::make_unique< T >( std::move( value ) );
    }

    Optional( const T& value ) {
        m_value = std::make_unique< T >( value );
    }

    const T& get() const {
        return *m_value;
    }

    bool hasValue() const {
        return m_value != nullptr;
    }

private:
    std::unique_ptr< T > m_value;
};

struct Mapping {
    using CoreValue = std::unique_ptr< CoreD::Model::Value >;
    using EnumValues = std::vector< std::string >;

    std::string capability;
    std::string variable;

    Optional< CoreD::Model::ValueType > valueType;
    Optional< CoreValue > value;
    Optional< JsonObject > valueObject;
    Optional< JsonObject > maxValue;
    Optional< JsonObject > minValue;
    Optional< EnumValues > enumValues;
    Optional< std::string > itemId;
};

CoreD::Model::Item createItemObject( const std::string& deviceId, const Mapping& mapping ) {
    using namespace CoreD::Model;
    auto check_json = Json( { { "capability", mapping.capability },
                              { "variable", mapping.variable },
                              { "itemName", mapping.capability + mapping.variable } } );

    auto checkSubscriptionRequiredField = []( const JsonObject& json, const std::string& key ) {
        try {
            jsonCheckString( json, key );
        }
        catch( std::exception& exception ) {
            std::string error = "value at ";
            error += key;

            if( key == "capability" ) {
                throw HubPlatform::CloudDevices::SubscriptionCapabilityError( error + exception.what() );
            }
            else if( key == "variable" ) {
                throw HubPlatform::CloudDevices::SubscriptionVariableError( error + exception.what() );
            }
            else if( key == "itemName" ) {
                throw HubPlatform::CloudDevices::SubscriptionItemNameError( error + exception.what() );
            }
            else
                throw std::runtime_error{ "value at \"" + key + "\" key: " + exception.what() };
        }
    };

    checkSubscriptionRequiredField( check_json, "capability" );
    checkSubscriptionRequiredField( check_json, "variable" );
    checkSubscriptionRequiredField( check_json, "itemName" );

    std::string itemName = mapping.capability + "." + mapping.variable;
    const auto& valueType = mapping.valueType.get();

    std::unique_ptr< CoreD::Model::Value > itemValue;
    if( mapping.valueObject.hasValue() ) {
        itemValue = ValueFactory::createItemValue( valueType, mapping.valueObject.get() );
    }
    else {
        itemValue = CoreD::Model::ValueFactory::createDefaultItemValue( valueType );
    }

    auto item = Item{ deviceId, itemName, valueType, true, false, std::move( itemValue ) };

    item.setShow( false );

    if( mapping.minValue.hasValue() ) {
        auto itemValue = ValueFactory::createItemValue( valueType, mapping.minValue.get() );
        item.setLowerLimitValue( std::move( itemValue ) );
    }

    if( mapping.maxValue.hasValue() ) {
        auto itemValue = ValueFactory::createItemValue( valueType, mapping.maxValue.get() );
        item.setUpperLimitValue( std::move( itemValue ) );
    }

    if( mapping.enumValues.hasValue() ) {
        item.setEnum( mapping.enumValues.get() );
    }

    return item;
}

Json createDbJsonMapping( const std::string& capability, const std::string& variable, const std::string& itemId ) {
    using namespace Resources;
    return JSON( DB::SubscriptionFields::Capability << capability << DB::SubscriptionFields::Variable << variable
                                                    << DB::SubscriptionFields::ItemId << itemId );
}

std::vector< Mapping > parseRequestMappings( const JsonArray& mappingsJson ) {
    std::vector< Mapping > mappings;

    for( auto& mapping : mappingsJson ) {
        mappings.emplace_back();
        auto& data = mappings.back();

        data.capability = getStringValueFromJson( mapping, Resources::RPCParams::Capability );
        data.variable = getStringValueFromJson( mapping, Resources::RPCParams::Variable );

        // NOTE: existing mapping is removed if request contains only "capability" and "variable" fields
        if( mapping.size() == 2 ) {
            continue;
        }

        if( mapping.find( Resources::RPCParams::ValueType ) != mapping.end() ) {
            std::string valueTypeStr = getStringValueFromJson( mapping, Resources::RPCParams::ValueType );
            auto valueType = CoreD::Model::ValueType::fromString( valueTypeStr );

            bool isDictionary = json_get_bool( mapping, Resources::RPCParams::DictionaryType );
            if( isDictionary ) {
                data.valueType = CoreD::Model::ValueType::makeDictionaryType( valueType );
            }
            else {
                data.valueType = valueType;
            }

            if( mapping.find( ( Resources::RPCParams::Value ) ) != mapping.end() ) {
                Json value;
                json_get( mapping, Resources::RPCParams::Value, value );
                data.valueObject = std::move( value );

                // check if constructable
                CoreD::Model::ValueFactory::createItemValue( data.valueType.get(), data.valueObject.get() );
            }
            else if( data.valueType.hasValue() ) {
                // check if constructable
                CoreD::Model::ValueFactory::createDefaultItemValue( data.valueType.get() );
            }
        }

        if( mapping.find( Resources::RPCParams::Enum ) != mapping.end() ) {
            data.enumValues = json_get_vec< std::string >( mapping, Resources::RPCParams::Enum );
        }

        if( mapping.find( Resources::RPCParams::MaxValue ) != mapping.end() ) {
            Json value;
            json_get( mapping, Resources::RPCParams::MaxValue, value );
            data.maxValue = std::move( value );
        }

        if( mapping.find( Resources::RPCParams::MinValue ) != mapping.end() ) {
            Json value;
            json_get( mapping, Resources::RPCParams::MinValue, value );
            data.minValue = std::move( value );
        }
    }
    return mappings;
}

std::vector< Mapping > parseDatabaseMappings( const JsonArray& mappingsJson ) {
    std::vector< Mapping > mappings;

    for( auto& mapping : mappingsJson ) {
        mappings.emplace_back();
        auto& data = mappings.back();
        data.capability = getStringValueFromJson( mapping, Resources::DB::SubscriptionFields::Capability );
        data.variable = getStringValueFromJson( mapping, Resources::DB::SubscriptionFields::Variable );
        data.itemId = getStringValueFromJson( mapping, Resources::DB::SubscriptionFields::ItemId );
    }
    return mappings;
}

void sortMappings( std::vector< Mapping >& requestMappings,
                   std::vector< Mapping >& dbMappings,
                   std::deque< Mapping* >& mappingsToCreate,
                   std::deque< Mapping* >& mappingsToUpdate,
                   std::deque< Mapping* >& mappingsToRemove,
                   std::deque< Mapping* >& mappingsToKeep ) {
    using namespace Resources;

    using Capability = std::string;
    using Variable = std::string;
    using MappingKey = std::pair< Capability, Variable >;

    std::map< MappingKey, Mapping* > requestMap;
    std::map< MappingKey, Mapping* > dbMap;

    for( auto& mapping : requestMappings ) {
        requestMap[ { mapping.capability, mapping.variable } ] = &mapping;
    }

    for( auto& mapping : dbMappings ) {
        dbMap[ { mapping.capability, mapping.variable } ] = &mapping;
    }

    for( auto it = requestMap.begin(); it != requestMap.end(); ++it ) {
        auto& requestKey = it->first;
        auto requestMappingPtr = it->second;

        auto dbIt = dbMap.find( requestKey );
        if( dbIt == dbMap.cend() ) {
            if( !requestMappingPtr->valueType.hasValue() )
                throw SubscriptionError( "Mapping create error: missing field: " +
                                         std::string( RPCParams::ValueType ) );
            mappingsToCreate.push_back( requestMappingPtr );
            continue;
        }

        auto dbMappingPtr = dbIt->second;

        if( requestMappingPtr->valueType.hasValue() ) {
            if( !dbMappingPtr->itemId.hasValue() )
                throw SubscriptionError( "Mapping update error: missing field: " + std::string( RPCParams::ItemId ) );
            requestMappingPtr->itemId = dbMappingPtr->itemId.get();
            mappingsToUpdate.push_back( requestMappingPtr );
        }
        else {
            if( !dbMappingPtr->itemId.hasValue() )
                throw SubscriptionError( "Mapping remove error: missing field: " + std::string( RPCParams::ItemId ) );
            mappingsToRemove.push_back( dbMappingPtr );
        }
    }

    for( auto it = dbMap.begin(); it != dbMap.cend(); ++it ) {
        auto& dbKey = it->first;
        if( requestMap.count( dbKey ) == 0 ) {
            auto dbMappingPtr = it->second;
            if( !dbMappingPtr->itemId.hasValue() )
                throw SubscriptionError( "Mapping keep error: missing field: " + std::string( RPCParams::ItemId ) );
            mappingsToKeep.push_back( dbMappingPtr );
        }
    }
}

Json addSubscriptionWithGateway( ISubscriptionHelper& helper,
                                 const std::string& gatewayName,
                                 const Json& requestParams ) {
    using namespace Resources;
    auto gateway = helper.getGatewayByName( gatewayName );
    if( gateway.empty() )
        throw GatewayNotFoundError( "Gateway does not exist: " + gatewayName );

    const auto& command = GatewayScripts::ScriptCommand::CreateAbstractSubscription;
    const std::string script = json_get_str( gateway, command );
    if( script.empty() )
        throw BadGatewayError( "Gateway \"" + gatewayName + "\" does not have registered command: " + command );

    Json params;
    params[ GatewayScripts::Fields::Request ] = requestParams;
    helper.sendExecutePluginScript( script, params );
    return JSON_EMPTY_OBJ;
}

Json addSubscriptionInternally( ISubscriptionHelper& helper,
                                IDbClient& dbClient,
                                const std::string& abstractUUID,
                                const std::vector< Mapping >& mappings ) {
    using namespace Resources;

    std::string deviceId;

    auto cleanup = [&]() {
        for( auto& mapping : mappings ) {
            if( !mapping.itemId.hasValue() )
                helper.removeItem( mapping.itemId.get() );
        }

        if( !deviceId.empty() )
            helper.removeDevice( deviceId );
    };

    CoreD::Model::Device device( helper.getGatewayId(),
                                 abstractUUID,
                                 AbstractDeviceProperty::DeviceType,
                                 AbstractDeviceProperty::DeviceTypeId,
                                 AbstractDeviceProperty::DeviceCategory,
                                 false );

    auto deviceResult = helper.addDevice( device );
    if( !deviceResult.second )
        throw SubscriptionError( "Failed to add abstract device" );
    deviceId = std::move( deviceResult.first );

    JsonArray dbMappings;

    for( auto& mapping : mappings ) {
        auto item = createItemObject( deviceId, mapping );
        auto result = helper.addItem( item );
        if( !result.second )
            throw SubscriptionBadParamError( "Invalid parameters for item creation", "check parameters" );
        auto& itemId = result.first;
        dbMappings.push_back( createDbJsonMapping( mapping.capability, mapping.variable, itemId ) );
    }

    Json dbData;
    dbData[ DB::SubscriptionFields::Id ] = abstractUUID;
    dbData[ DB::SubscriptionFields::DeviceId ] = deviceId;
    dbData[ DB::SubscriptionFields::Mappings ] = std::move( dbMappings );

    SubscriptionUtils::storeSubscriptionToDb( dbClient, dbData );

    return JSON_EMPTY_OBJ;
}

Json updateSubscriptionWithGateway( ISubscriptionHelper& helper,
                                    const std::string& gatewayName,
                                    const Json& requestParams,
                                    const Json& subscriptionRecord,
                                    std::deque< Mapping* >& mappingsToCreate,
                                    std::deque< Mapping* >& mappingsToUpdate,
                                    std::deque< Mapping* >& mappingsToRemove,
                                    std::deque< Mapping* >& mappingsToKeep ) {
    using namespace Resources;

    auto gateway = helper.getGatewayByName( gatewayName );
    if( gateway.empty() )
        throw GatewayNotFoundError( "Gateway does not exist: " + gatewayName );

    const auto& command = GatewayScripts::ScriptCommand::UpdateAbstractSubscription;
    const std::string script = json_get_str( gateway, command );
    if( script.empty() )
        throw BadGatewayError( "Gateway \"" + gatewayName + "\" does not have registered command: " + command );

    auto createMappingArray = []( std::deque< Mapping* >& mappings ) {
        JsonArray array;
        for( auto* mapping : mappings ) {
            array.push_back( JSON( DB::SubscriptionFields::Capability
                                   << mapping->capability << DB::SubscriptionFields::Variable << mapping->variable ) );
        }
        return array;
    };

    Json mappingOperations;
    mappingOperations[ GatewayScripts::Fields::MappingOperationAdd ] = createMappingArray( mappingsToCreate );
    mappingOperations[ GatewayScripts::Fields::MappingOperationUpdate ] = createMappingArray( mappingsToUpdate );
    mappingOperations[ GatewayScripts::Fields::MappingOperationRemove ] = createMappingArray( mappingsToRemove );
    mappingOperations[ GatewayScripts::Fields::MappingOperationKeep ] = createMappingArray( mappingsToKeep );

    Json params;
    params[ GatewayScripts::Fields::Request ] = requestParams;
    params[ GatewayScripts::Fields::SubscriptionRecord ] = subscriptionRecord;
    params[ GatewayScripts::Fields::MappingOperations ] = std::move( mappingOperations );

    helper.sendExecutePluginScript( script, params );
    return JSON_EMPTY_OBJ;
}

Json updateSubscriptionInternally( ISubscriptionHelper& helper,
                                   IDbClient& dbClient,
                                   const Json& subscriptionRecord,
                                   std::deque< Mapping* >& mappingsToCreate,
                                   std::deque< Mapping* >& mappingsToUpdate,
                                   std::deque< Mapping* >& mappingsToRemove,
                                   std::deque< Mapping* >& mappingsToKeep ) {
    using namespace Resources;

    auto abstractUUID = getStringValueFromJson( subscriptionRecord, DB::SubscriptionFields::Id );
    auto deviceId = getStringValueFromJson( subscriptionRecord, DB::SubscriptionFields::DeviceId );

    JsonArray updatedDbMappings;

    for( auto* mapping : mappingsToCreate ) {
        auto item = createItemObject( deviceId, *mapping );
        auto result = helper.addItem( item );
        if( !result.second )
            throw SubscriptionBadParamError( "Invalid parameters for item creation", "check parameters" );
        auto& itemId = result.first;
        updatedDbMappings.push_back( createDbJsonMapping( mapping->capability, mapping->variable, itemId ) );
    }

    for( auto* mapping : mappingsToUpdate ) {
        auto item = createItemObject( deviceId, *mapping );
        auto itemId = mapping->itemId.get();
        helper.modifyItem( itemId, item );
        updatedDbMappings.push_back( createDbJsonMapping( mapping->capability, mapping->variable, itemId ) );
    }

    for( auto* mapping : mappingsToKeep ) {
        updatedDbMappings.push_back(
            createDbJsonMapping( mapping->capability, mapping->variable, mapping->itemId.get() ) );
    }

    for( auto& mapping : mappingsToRemove ) {
        helper.removeItem( mapping->itemId.get() );
    }

    Json updatedDbData;
    updatedDbData[ DB::SubscriptionFields::Id ] = abstractUUID;
    updatedDbData[ DB::SubscriptionFields::DeviceId ] = deviceId;
    updatedDbData[ Resources::DB::SubscriptionFields::Mappings ] = updatedDbMappings;
    SubscriptionUtils::storeSubscriptionToDb( dbClient, updatedDbData );

    return JSON_EMPTY_OBJ;
}

Json deleteSubscriptionWithGateway( ISubscriptionHelper& helper,
                                    const std::string& gatewayName,
                                    const Json& requestParams,
                                    const Json& subscriptionRecord ) {
    using namespace Resources;

    auto gateway = helper.getGatewayByName( gatewayName );
    if( gateway.empty() )
        throw GatewayNotFoundError( "Gateway does not exist: " + gatewayName );

    const auto& command = GatewayScripts::ScriptCommand::DeleteAbstractSubscription;
    const std::string script = json_get_str( gateway, command );
    if( script.empty() )
        throw BadGatewayError( "Gateway \"" + gatewayName + "\" does not have registered command: " + command );

    Json params;
    params[ GatewayScripts::Fields::Request ] = requestParams;
    params[ GatewayScripts::Fields::SubscriptionRecord ] = subscriptionRecord;

    helper.sendExecutePluginScript( script, params );
    return JSON_EMPTY_OBJ;
}

Json deleteSubscriptionInternal( ISubscriptionHelper& helper, IDbClient& dbClient, const Json& subscriptionRecord ) {
    using namespace Resources;

    auto abstractUUID = getStringValueFromJson( subscriptionRecord, DB::SubscriptionFields::Id );
    auto deviceId = getStringValueFromJson( subscriptionRecord, DB::SubscriptionFields::DeviceId );

    JsonArray mappings;
    json_get_arr( subscriptionRecord, DB::SubscriptionFields::Mappings, mappings );

    for( const auto& mapping : mappings ) {
        auto itemId = json_get_str( mapping, DB::SubscriptionFields::ItemId );
        helper.removeItem( itemId );
    }

    helper.removeDevice( deviceId );
    SubscriptionUtils::removeSubscriptionFromDb( dbClient, abstractUUID );
    return JSON_EMPTY_OBJ;
}

}  // namespace

SubscriptionManager::SubscriptionManager( ISubscriptionHelper& helper, IDbClient& dbClient )
: m_helper( helper )
, m_dbClient( dbClient ) {}

JsonObject SubscriptionManager::addSubscription( const Json& params ) {
    using namespace Resources;

    JsonObject response = JSON_EMPTY_OBJ;

    try {
        auto abstractUUID = getStringValueFromJson( params, RPCParams::AbstractUUID );

        JsonArray mappingsJson;
        if( !json_get_arr( params, RPCParams::Mappings, mappingsJson ) || mappingsJson.empty() ) {
            throw SubscriptionError( "Invalid field: " + std::string( RPCParams::Mappings ) );
        }
        auto mappingData = parseRequestMappings( mappingsJson );

        for( auto& data : mappingData ) {
            if( !data.valueType.hasValue() ) {
                throw SubscriptionError( "Missing field: " + std::string( RPCParams::ValueType ) );
            }
        }

        std::string gatewayName = json_get_str( params, RPCParams::GatewayName );

        if( !SubscriptionUtils::readSubscriptionFromDb( m_dbClient, abstractUUID ).empty() ) {
            Json deleteRequest = JSON( RPCParams::AbstractUUID << abstractUUID );
            if( !gatewayName.empty() )
                deleteRequest[ RPCParams::GatewayName ] = gatewayName;
            deleteSubscription( deleteRequest );
        }

        if( !gatewayName.empty() ) {
            return addSubscriptionWithGateway( m_helper, gatewayName, params );
        }
        else {
            return addSubscriptionInternally( m_helper, m_dbClient, abstractUUID, mappingData );
        }
    }
    catch( const GatewayNotFoundError& ex ) {
        Log::error( cc::error( "Gateway not found: " ) + cc::note( ex.what() ) );
        setError( response, Errors::NO_GATEWAY_FOUND, ex.what() );
    }
    catch( const BadGatewayError& ex ) {
        Log::error( cc::error( "Bad gateway: " ) + cc::note( ex.what() ) );
        setError( response, Errors::GATEWAY_COMMAND_IS_EMPTY, ex.what() );
    }
    catch( const SubscriptionBadParamError& ex ) {
        Log::error( cc::error( "Wrong params: " ) + cc::note( ex.what() ) );
        setError( response, Errors::WRONG_PARAMS, ex.what() );
    }
    catch( const SubscriptionCapabilityError& ex ) {
        Log::error( cc::error( "Invalid variable: " ) + cc::note( ex.what() ) );
        setError( response, Errors::INVALID_SUBSCRIPTION_CAPABILITY_NAME, ex.what() );
    }
    catch( const SubscriptionVariableError& ex ) {
        Log::error( cc::error( "Invalid capability: " ) + cc::note( ex.what() ) );
        setError( response, Errors::INVALID_SUBSCRIPTION_VARIABLE_NAME, ex.what() );
    }
    catch( const SubscriptionItemNameError& ex ) {
        Log::error( cc::error( "Invalid item name: " ) + cc::note( ex.what() ) );
        setError( response, Errors::INVALID_ITEM_NAME, ex.what() );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception while creating subscription list: " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
        setError( response, Errors::INTERNAL_OPERATION_ERROR, e.what() );
    }
    catch( ... ) {
        setError( response, Errors::INTERNAL_OPERATION_ERROR, "unknown exception" );
    }

    return response;
}

JsonObject SubscriptionManager::updateSubscription( const Json& params ) {
    using namespace Resources;

    JsonObject response = JSON_EMPTY_OBJ;

    try {
        auto abstractUUID = getStringValueFromJson( params, RPCParams::AbstractUUID );
        std::string gatewayName = json_get_str( params, RPCParams::GatewayName );

        JsonArray mappingsJson;
        if( !json_get_arr( params, RPCParams::Mappings, mappingsJson ) || mappingsJson.empty() ) {
            throw SubscriptionError( "Invalid field: " + std::string( RPCParams::Mappings ) );
        }
        auto requestMappings = parseRequestMappings( mappingsJson );

        auto dbSubscriptions = SubscriptionUtils::readSubscriptionFromDb( m_dbClient, abstractUUID );
        if( dbSubscriptions.empty() )
            throw SubscriptionError( "Abstract not found in database: " + abstractUUID );

        JsonObject dbMappingsJson;
        json_get_arr( dbSubscriptions, DB::SubscriptionFields::Mappings, dbMappingsJson );
        auto dbMappings = parseDatabaseMappings( dbMappingsJson );

        std::deque< Mapping* > mappingsToCreate;
        std::deque< Mapping* > mappingsToUpdate;
        std::deque< Mapping* > mappingsToRemove;
        std::deque< Mapping* > mappingsToKeep;

        sortMappings(
            requestMappings, dbMappings, mappingsToCreate, mappingsToUpdate, mappingsToRemove, mappingsToKeep );

        if( !gatewayName.empty() ) {
            return updateSubscriptionWithGateway( m_helper,
                                                  gatewayName,
                                                  params,
                                                  dbSubscriptions,
                                                  mappingsToCreate,
                                                  mappingsToUpdate,
                                                  mappingsToRemove,
                                                  mappingsToKeep );
        }
        else {
            return updateSubscriptionInternally( m_helper,
                                                 m_dbClient,
                                                 dbSubscriptions,
                                                 mappingsToCreate,
                                                 mappingsToUpdate,
                                                 mappingsToRemove,
                                                 mappingsToKeep );
        }
    }
    catch( const GatewayNotFoundError& ex ) {
        Log::error( cc::error( "Gateway not found: " ) + cc::note( ex.what() ) );
        setError( response, Errors::NO_GATEWAY_FOUND, ex.what() );
    }
    catch( const BadGatewayError& ex ) {
        Log::error( cc::error( "Bad gateway: " ) + cc::note( ex.what() ) );
        setError( response, Errors::GATEWAY_COMMAND_IS_EMPTY, ex.what() );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception while update subscription: " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
        setError( response, Errors::INTERNAL_OPERATION_ERROR, e.what() );
    }
    catch( ... ) {
        setError( response, Errors::INTERNAL_OPERATION_ERROR, "unknown exception" );
    }

    return response;
}

JsonObject SubscriptionManager::deleteSubscription( const Json& params ) {
    using namespace Resources;

    auto abstractUUID = getStringValueFromJson( params, RPCParams::AbstractUUID );
    std::string gatewayName = json_get_str( params, RPCParams::GatewayName );

    auto dbData = SubscriptionUtils::readSubscriptionFromDb( m_dbClient, abstractUUID );
    if( dbData.empty() ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to delete subscription. UUID not found: " ) + cc::note( abstractUUID ) );
        auto response = JSON_EMPTY_OBJ;
        setError( response, Errors::DOES_NOT_EXIST, "uuid not found" );
        return response;
    }

    if( !gatewayName.empty() ) {
        return deleteSubscriptionWithGateway( m_helper, gatewayName, params, dbData );
    }
    else {
        return deleteSubscriptionInternal( m_helper, m_dbClient, dbData );
    }
}

void SubscriptionManager::sendSubscriptionList() {
    using namespace Resources::DB;

    try {
        auto subscriptions = SubscriptionUtils::readAllSubscriptionsFromDb( m_dbClient );

        std::map< std::string, Json > itemValueMap;

        for( auto& data : subscriptions ) {
            const JsonArray& mappings = data.at( SubscriptionFields::Mappings );
            for( auto& mapping : mappings ) {
                std::string itemId = json_get_str( mapping, SubscriptionFields::ItemId );
                auto itemJson = m_helper.getItem( itemId );
                if( itemJson.empty() )
                    throw SubscriptionError( "Item with id (" + itemId + ") not found" );
                itemValueMap.emplace( itemId, std::move( itemJson ) );
            }
        }

        auto message = SubscriptionUtils::createSubscriptionMessage( subscriptions, itemValueMap );
        m_helper.sendSubscriptionList( std::move( message ) );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception while sending subscription list: " ) +
                          cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Unknown exception while sending subscription list" ) );
    }
}

void SubscriptionManager::storeSubscriptionRecord( const Json& params ) {
    Json record;
    if( !json_get( params, Resources::GatewayScripts::Fields::SubscriptionRecord, record ) )
        throw SubscriptionError( "Unable to store subscription record: record is missing" );
    SubscriptionUtils::storeSubscriptionToDb( m_dbClient, record );
}

void SubscriptionManager::removeSubscriptionRecord( const Json& params ) {
    auto abstractUUID = getStringValueFromJson( params, Resources::RPCParams::AbstractUUID );
    SubscriptionUtils::removeSubscriptionFromDb( m_dbClient, abstractUUID );
}

namespace {
const char* const ItemId = "_id";
const char* const Value = "value";
}  // namespace

/** Dispatches data of a request directly to the handling dispatch script.
 * @params - object (_id: itemId, value: any) */
JsonObject SubscriptionManager::dispatchSubscription( const Json& params ) {
    LOGGER_TRACE( cc::j( params ) );

    using namespace Resources;
    std::string gatewayName = json_get_str( params, RPCParams::GatewayName );

    auto gateway = m_helper.getGatewayByName( gatewayName );
    if( gateway.empty() )
        throw GatewayNotFoundError( "Gateway does not exist: " + gatewayName );

    const auto& command = GatewayScripts::ScriptCommand::DispatchAbstractSubscription;
    const std::string script = json_get_str( gateway, command );
    if( script.empty() )
        throw BadGatewayError( "Gateway \"" + gatewayName + "\" does not have registered command: " + command );

    auto itemId = json_get_str( params, ItemId );
    auto valueIt = params.find( Value );
    if( itemId.empty() ) {
        LOGGER_ERROR( cc::error( "ItemId is empty" ) );
        throw SubscriptionBadParamError( "ItemId is empty", "_id" );
    }
    if( valueIt == params.end() ) {
        LOGGER_ERROR( cc::error( "Value is not found" ) );
        throw SubscriptionBadParamError( "Value not found", "value" );
    }

    if( !m_helper.itemBelongsToGateway( itemId, gatewayName ) ) {
        LOGGER_ERROR( cc::error( "The item doesn't belong to the gateway: " ) + gatewayName );
        throw SubscriptionBadParamError(
            "Item doesn't belong to the gateway: itemId: " + itemId + " gateway name: " + gatewayName, "_id" );
    }

    Json dispatchParams;
    dispatchParams[ ItemId ] = itemId;
    dispatchParams[ Value ] = *valueIt;

    LOGGER_TRACE( cc::trace( "dispatchParams: " ) + cc::j( dispatchParams ) );

    // TODO: make it an async request, check result and return result as a response to the
    // hub.cloud.subscription.dispatch
    m_helper.sendExecutePluginScript( script, dispatchParams );
    return JSON_EMPTY_OBJ;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
