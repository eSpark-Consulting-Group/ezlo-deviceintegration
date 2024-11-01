#include "zigbee/zigbee_client.h"

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "addon_interfaces/addon.h"
#include "zigbee/zigbee_addon.h"

#include "jsonrpc/jsonrpc.h"

#include <cassert>

#define ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "zigbee." } + __func__ ), number )

#define ZIGBEE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "zigbee." } + __func__ ), number )

#define ZIGBEE_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "zigbee." } + __func__ ) )

namespace HubPlatform {
namespace {

Zigbee::ZigbeeClient createZigbeeClient( lua_State* luaState ) {
    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );
    return Zigbee::ZigbeeClient( addon );
}

int startInclude( lua_State* luaState ) {
    const auto interviewConfigurationsFile = LuaToNative< NativeString >{}( luaState, 1 );

    createZigbeeClient( luaState ).startInclude( interviewConfigurationsFile.valueOr( std::string{} ) );
    return 0;
}

int stopInclude( lua_State* luaState ) {
    createZigbeeClient( luaState ).stopInclude();
    return 0;
}

int removeNode( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );

    try {
        createZigbeeClient( luaState ).removeNode( nodeId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove node: %d. %s", nodeId, e.what() );
    }

    return 0;
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( Zigbee::ZigbeeAddon::Name ) );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to subscribe" );
    }

    try {
        const auto subscriberId = eventsSubscriber->subscribe( getPluginEventsSubscriber( luaState ) );
        lua_pushstring( luaState, subscriberId.c_str() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to subscribe: %s", ex.what() );
    }

    return 1;
}

int setSubscriptionFilters( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = ZIGBEE_LUA_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules(
            luaState, Addons::Addon::createQualifiedName( Zigbee::ZigbeeAddon::Name ), subscriberId, rules );
    }
    catch( const std::exception& ex ) {
        return luaL_error(
            luaState, "fail to set event filter rule for a (%s) subscription: %s", subscriberId.c_str(), ex.what() );
    }

    return 0;
}

int unsubscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( Zigbee::ZigbeeAddon::Name ) );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to unsubscribe" );
    }

    try {
        eventsSubscriber->unsubscribe( getPluginEventsSubscriber( luaState ) );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to unsubscribe: %s", ex.what() );
    }

    return 0;
}

int getNode( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    try {
        const auto json = createZigbeeClient( luaState ).getNodeAsJson( nodeId );
        return pushJsonAsLuaObj(
            luaState, json, createMappingKeyFormatter( DefaultDbToLuaKeyMap ), std::numeric_limits< size_t >::max() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get node: %d. %s", nodeId, e.what() );
    }
}

int getAllNodesIds( lua_State* luaState ) {
    const auto json = createZigbeeClient( luaState ).getAllNodesIds();
    return pushJsonAsLuaObj( luaState, json );
}

int getInfo( lua_State* luaState ) {
    const auto json = createZigbeeClient( luaState ).getInfo();
    return pushJsonAsLuaObj( luaState, json );
}

int setAttributeValue( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( 5 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto clusterName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto attributeName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );

    try {
        createZigbeeClient( luaState )
            .setAttributeValue( nodeId, endpointId, clusterName, attributeName, pullJson( luaState, 5 ) );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to set attribute value. node_id: %d, endpoint id: %d, cluster_name: %s, , attribute_name: %s. %s",
            nodeId,
            endpointId,
            clusterName.c_str(),
            attributeName.c_str(),
            e.what() );
    }

    return 0;
}

int setNetworkRadioChannel( lua_State* luaState ) {
    ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );

    try {
        createZigbeeClient( luaState ).setNetworkRadioChannel( ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to set network radio channel", e.what() );
    }

    return 0;
}

int collectChildAndRouteInfo( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 0 );

    try {
        createZigbeeClient( luaState ).collectChildAndRouteInfo();
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to collect child and route info. %s", e.what() );
    }

    return 0;
}

int sendCommand( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto clusterName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto commandName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );
    const auto params = pullJsonOpt( luaState, 5 );

    try {
        createZigbeeClient( luaState ).sendCommand( nodeId, endpointId, clusterName, commandName, params );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to send command. node id: %d, endpoint id: %d, cluster name: %s, command name: %s. %s",
                           nodeId,
                           endpointId,
                           clusterName.c_str(),
                           commandName.c_str(),
                           e.what() );
    }

    return 0;
}

int requestAttributeValue( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( 4 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto clusterName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto attributeName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );
    try {
        createZigbeeClient( luaState ).requestAttributeValue( nodeId, endpointId, clusterName, attributeName );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to request attribute value. node id: %d, endpoint id: %d, cluster name: %s, attribute name: %s. %s",
            nodeId,
            endpointId,
            clusterName.c_str(),
            attributeName.c_str(),
            e.what() );
    }

    return 0;
}

int requestUserCodes( lua_State* luaState ) {
    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto startUserId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 );
    const auto endUserId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 );

    try {
        createZigbeeClient( luaState ).requestUserCodes( nodeId, endpointId, startUserId, endUserId );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to request user codes. node id: %d, endpoint id: %d, start user ID: %d, end user ID: %d. %s",
            nodeId,
            endpointId,
            startUserId,
            endUserId,
            e.what() );
    }

    return 0;
}

int requestMfgSpecificAttributeValue( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_PARAMETERS( 4 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto clusterId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 );
    const auto attributeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 );
    try {
        createZigbeeClient( luaState ).requestMfgSpecificAttributeValue( nodeId, endpointId, clusterId, attributeId );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to request attribute value. node id: %d, endpoint id: %d, cluster id: %d, attribute id: %d. %s",
            nodeId,
            endpointId,
            clusterId,
            attributeId,
            e.what() );
    }

    return 0;
}

int configureAttributeReport( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto clusterName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto attributeName = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );

    const auto minInterval = LuaToNative< NativeInteger >{}( luaState, 5 );
    const auto maxInterval = LuaToNative< NativeInteger >{}( luaState, 6 );
    const auto reportableChange = pullJsonOpt( luaState, 7 );

    try {
        if( !minInterval.isValid() ) {
            createZigbeeClient( luaState ).configureAttributeReport( nodeId, endpointId, clusterName, attributeName );
        }
        else {
            if( !maxInterval.isValid() ) {
                Log::error( "maxInterval parameter is not specified" );
            }
            else {
                createZigbeeClient( luaState )
                    .configureAttributeReport( nodeId,
                                               endpointId,
                                               clusterName,
                                               attributeName,
                                               minInterval.value(),
                                               maxInterval.value(),
                                               reportableChange );
            }
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to configure attribute report. node id: %d, endpoint id: %d, cluster name: %s, "
                           "attribute name: %s. %s",
                           nodeId,
                           endpointId,
                           clusterName.c_str(),
                           attributeName.c_str(),
                           e.what() );
    }

    return 0;
}

int configureMfgSpecificAttributeReport( lua_State* luaState ) {
    ZIGBEE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto endpointId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto clusterId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 );
    const auto attributeId = ZIGBEE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 );

    const auto minInterval = LuaToNative< NativeInteger >{}( luaState, 5 );
    const auto maxInterval = LuaToNative< NativeInteger >{}( luaState, 6 );
    const auto reportableChange = pullJsonOpt( luaState, 7 );

    try {
        if( !minInterval.isValid() ) {
            createZigbeeClient( luaState )
                .configureMfgSpecificAttributeReport( nodeId, endpointId, clusterId, attributeId );
        }
        else {
            if( !maxInterval.isValid() ) {
                Log::error( "maxInterval parameter is not specified" );
            }
            else {
                createZigbeeClient( luaState )
                    .configureMfgSpecificAttributeReport( nodeId,
                                                          endpointId,
                                                          clusterId,
                                                          attributeId,
                                                          minInterval.value(),
                                                          maxInterval.value(),
                                                          reportableChange );
            }
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to configure attribute report. node id: %d, endpoint id: %d, cluster id: %d, "
                           "attribute id: %d. %s",
                           nodeId,
                           endpointId,
                           clusterId,
                           attributeId,
                           e.what() );
    }

    return 0;
}

int reset( lua_State* luaState ) {
    try {
        createZigbeeClient( luaState ).reset();
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to reset. %s", e.what() );
    }
    return 0;
}

const char* const moduleName = "zigbee";
const luaL_Reg module[] = {
    { "start_include", errorCatchWrapper< startInclude > },
    { "stop_include", errorCatchWrapper< stopInclude > },
    { "remove_node", errorCatchWrapper< removeNode > },
    { "get_all_nodes_ids", errorCatchWrapper< getAllNodesIds > },
    { "subscribe", errorCatchWrapper< subscribe > },
    { "set_subscription_filters", errorCatchWrapper< setSubscriptionFilters > },
    { "unsubscribe", errorCatchWrapper< unsubscribe > },
    { "get_node", errorCatchWrapper< getNode > },
    { "get_info", errorCatchWrapper< getInfo > },
    { "set_attribute_value", errorCatchWrapper< setAttributeValue > },
    { "send_command", errorCatchWrapper< sendCommand > },
    { "request_attribute_value", errorCatchWrapper< requestAttributeValue > },
    { "request_user_codes", errorCatchWrapper< requestUserCodes > },
    { "configure_attribute_report", errorCatchWrapper< configureAttributeReport > },
    { "request_mfg_specific_attribute_value", errorCatchWrapper< requestMfgSpecificAttributeValue > },
    { "configure_mfg_specific_attribute_report", errorCatchWrapper< configureMfgSpecificAttributeReport > },
    { "reset", errorCatchWrapper< reset > },
    { "set_network_radio_channel", errorCatchWrapper< setNetworkRadioChannel > },
    { "collect_child_and_route_info", errorCatchWrapper< collectChildAndRouteInfo > },
    { NULL, NULL }
};

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
