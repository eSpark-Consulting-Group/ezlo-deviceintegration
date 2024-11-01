#include "cloud_devices/subscription_manager/subscription_utils.h"

#include "cloud_devices/subscription_manager/subscription_exceptions.h"
#include "cloud_devices/resources/cloud_devices_resources.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CloudDevices {
namespace SubscriptionUtils {

Json readSubscriptionFromDb( IDbClient& dbClient, const std::string& abstractUUID ) {
    using namespace Resources::DB;
    auto query = JSON( SubscriptionFields::Id << abstractUUID );
    auto result = dbClient.findOne( TableNames::Subscriptions, query, nullptr );
    if( result.empty() )
        return {};
    return result;
}

Json readAllSubscriptionsFromDb( IDbClient& dbClient ) {
    auto result = dbClient.findAll( Resources::DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, nullptr );
    if( result.empty() )
        return {};
    return result;
}

void removeSubscriptionFromDb( IDbClient& dbClient, const std::string& abstractUUID ) {
    using namespace Resources::DB;
    dbClient.remove( TableNames::Subscriptions, JSON( SubscriptionFields::Id << abstractUUID ) );
}

void removeSubscriptionItemFromDb( IDbClient& dbClient, const std::string& abstractUUID, const std::string& id ) {
    using namespace Resources::DB;

    Json subscription = readSubscriptionFromDb( dbClient, abstractUUID );

    JsonObject mappings;
    json_get_arr( subscription, SubscriptionFields::Mappings, mappings );
    JsonArrayBuilder editMappings;
    for( auto mapping : mappings ) {
        if( !( mapping.find( SubscriptionFields::ItemId ) != mapping.end() &&
               json_get_str( mapping, SubscriptionFields::ItemId ) == id ) ) {
            editMappings << mapping;
        }
    }

    subscription[ SubscriptionFields::Mappings ] = editMappings;

    dbClient.remove( TableNames::Subscriptions, JSON( SubscriptionFields::Id << abstractUUID ) );
    dbClient.update( Resources::DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, subscription, true, false );
}

void storeSubscriptionToDb( IDbClient& dbClient, const Json& subscription ) {
    using namespace Resources::DB;
    auto validateField = []( const Json& json, const std::string& key ) {
        if( json_get_str( json, key ).empty() ) {
            throw SubscriptionError( "Unable to store invalid record to database: invalid field: " + key );
        }
    };

    validateField( subscription, SubscriptionFields::Id );
    validateField( subscription, SubscriptionFields::DeviceId );

    auto& mappings = subscription.at( SubscriptionFields::Mappings );
    for( auto& mapping : mappings ) {
        validateField( mapping, SubscriptionFields::Capability );
        validateField( mapping, SubscriptionFields::Variable );
        validateField( mapping, SubscriptionFields::ItemId );
    }

    dbClient.update( Resources::DB::TableNames::Subscriptions, JSON_EMPTY_OBJ, subscription, true, false );
}

Json createSubscriptionMessage( const JsonObject& subscriptionsTable, const ItemValueMap& itemValueMap ) {
    using namespace Resources;
    using namespace Resources::DB;

    SubscriptionBuilder::ResponseTypes responseTypes;
    auto abstracts = JSON_EMPTY_ARR;
    for( auto& data : subscriptionsTable ) {
        JsonObject abstract = SubscriptionBuilder::formatAbstract( data, itemValueMap, responseTypes );
        abstracts.push_back( std::move( abstract ) );
    }

    Json params;
    params[ CloudRPCParams::Abstracts ] = std::move( abstracts );

    JsonObject responseTypesResult = SubscriptionBuilder::formatResponseType( responseTypes );
    if( !responseTypesResult.empty() )
        params[ CloudRPCParams::ResponseType ] = responseTypesResult;

    JsonObject message = SubscriptionBuilder::formatControllerSubscribeAbstractsCommand( std::move( params ) );

    return message;
}

Json SubscriptionBuilder::formatControllerSubscribeAbstractsCommand( Json params ) {
    using namespace Resources;
    const int cloudApiVersion = 1;

    JsonObject message;
    message[ API::General::Api ] = cloudApiVersion;
    message[ API::General::Method ] = CloudMethods::CloudControllerSubscribeAbstracts;
    Json& apiParams = message[ API::General::Params ];
    apiParams = std::move( params );

    return message;
}

JsonObject SubscriptionBuilder::formatMapping( const Json& dbMapping,
                                               const ItemValueMap& itemValueMap,
                                               std::set< std::string >& responseTypes ) {
    using namespace Resources;
    using namespace Resources::DB;

    Json mapping;
    mapping[ CloudRPCParams::Capability ] = dbMapping.at( SubscriptionFields::Capability );
    mapping[ CloudRPCParams::Variable ] = dbMapping.at( SubscriptionFields::Variable );

    std::string itemId = json_get_str( dbMapping, SubscriptionFields::ItemId );

    auto& itemJson = itemValueMap.at( itemId );

    auto valueJson = itemJson.at( HubPlatform::CoreD::Model::Resources::ItemFields::ValueFieldName );
    std::string valueType = json_get_str( itemJson, HubPlatform::CoreD::Model::Resources::ItemFields::TypeFieldName );

    if( valueType == "int" || valueType == "float" || valueType == "string" ) {
        mapping[ CloudRPCParams::Value ] = valueJson;
    }
    else if( valueType == "bool" ) {
        mapping[ CloudRPCParams::Value ] = int( valueJson.get< bool >() );
    }
    else {
        mapping[ CloudRPCParams::Value ] = valueJson.dump();
    }

    bool isGeofencing =
        json_get_str( dbMapping, SubscriptionFields::Capability ) == Abstract::Geofencing::CapabilityGeofencesAll;

    if( isGeofencing ) {
        // Cloud docs: ASF v.6
        mapping[ CloudRPCParams::ResponseType ] = CloudRPCParams::GeofencingDispatch;
        mapping[ CloudRPCParams::ResponseData ] =
            JSON( "receiving_item_id" << itemId << "gateway_name" << CloudRPCParams::GeofencingGateway );

        responseTypes.insert( CloudRPCParams::GeofencingDispatch );
    }
    else {
        mapping[ CloudRPCParams::ItemId ] = itemId;
    }

    return mapping;
}

JsonObject SubscriptionBuilder::formatAbstract( const JsonObject& data,
                                                const ItemValueMap& itemValueMap,
                                                ResponseTypes& responseTypes ) {
    using namespace Resources;
    using namespace Resources::DB;

    Json abstract;

    auto gatewayName = json_get_str( data, SubscriptionFields::GatewayName );
    if( !gatewayName.empty() )
        abstract[ CloudRPCParams::GatewayName ] = gatewayName;

    abstract[ CloudRPCParams::AbstractUUID ] = data.at( SubscriptionFields::Id );
    abstract[ CloudRPCParams::DeviceId ] = data.at( SubscriptionFields::DeviceId );

    JsonArray mappings;

    const JsonArray& dbMappings = data.at( SubscriptionFields::Mappings );
    for( auto& dbMapping : dbMappings ) {
        Json mapping = SubscriptionBuilder::formatMapping( dbMapping, itemValueMap, responseTypes );

        mappings.push_back( std::move( mapping ) );
    }

    abstract[ CloudRPCParams::Mappings ] = std::move( mappings );

    return abstract;
}

// Cloud Docs: ASF v.6
Json SubscriptionBuilder::formatResponseType( const ResponseTypes& responseTypes ) {
    if( responseTypes.empty() )
        return {};

    using namespace Resources::CloudRPCParams;
    JsonObject result;
    for( auto& responseType : responseTypes ) {
        if( responseTypes.find( GeofencingDispatch ) != responseTypes.end() ) {
            const char* const GeofencingDispatchTemplate =
                R"({"method": "hub.cloud.subscription.dispatch","params": { "gatewayName": "", "_id" : "", "value" : []}})";
            Json dataPath = { { "receiving_item_id", ".params._id" }, { "gateway_name", ".params.gatewayName" } };
            Json geofencingResponseSchema = {
                { "template", GeofencingDispatchTemplate }, { "value_path", ".params.value" }, { "data_path", dataPath }
            };
            result[ GeofencingDispatch ] = geofencingResponseSchema;
        }
        else {
            LOGGER_DEBUG( cc::debug( "Cloud Subscription response type definition not found: " ) + responseType );
        }
    }
    return result;
}

}  // namespace SubscriptionUtils
}  // namespace CloudDevices
}  // namespace HubPlatform
