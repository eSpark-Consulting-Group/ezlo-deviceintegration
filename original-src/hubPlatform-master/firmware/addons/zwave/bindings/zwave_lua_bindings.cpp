#include "zwave/zwave_client.h"
#include "zwave/resources/zwave_resources.h"
#include "zwave/zwave_addon.h"

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "addon_interfaces/addon.h"

#include "jsonrpc/jsonrpc.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_custom_getters.h"

#define ZWAVE_LUA_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "zwave." } + __func__ ), number )

#define ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "zwave." } + __func__ ), number )

#define ZWAVE_LUA_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "zwave." } + __func__ ) )

#define ZWAVE_LUA_HAS_INPUT_PARAMETERS( number ) lua_type( luaState, number ) != LUA_TNIL;

#define UNDEFINED_TARGET_CHANNEL_ID 255

namespace HubPlatform {
namespace {

Zwave::ZwaveClient createZwaveClient( lua_State* luaState ) {
    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );
    return Zwave::ZwaveClient( addon );
}

int startInclude( lua_State* luaState ) {
    try {
        const auto deviceSettingFilePath = LuaToNative< NativeString >{}( luaState, 1 );
        createZwaveClient( luaState ).startInclude( deviceSettingFilePath.valueOr( std::string{} ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to start include : %s", e.what() );
    }

    return 0;
}

int stopInclude( lua_State* luaState ) {
    createZwaveClient( luaState ).stopInclude();
    return 0;
}

int stopExclude( lua_State* luaState ) {
    createZwaveClient( luaState ).stopExclude();
    return 0;
}

int startLearn( lua_State* luaState ) {
    createZwaveClient( luaState ).startLearn();
    return 0;
}

int stopLearn( lua_State* luaState ) {
    createZwaveClient( luaState ).stopLearn();
    return 0;
}

int stopSecurityInclude( lua_State* luaState ) {
    createZwaveClient( luaState ).stopSecurityInclude();
    return 0;
}

int startExclude( lua_State* luaState ) {
    if( lua_gettop( luaState ) > 0 ) {
        const auto nodeId = (uint8_t) lua_tonumber( luaState, 1 );
        createZwaveClient( luaState ).startExclude( nodeId );
    }
    else {
        createZwaveClient( luaState ).startExclude();
    }
    return 0;
}

int setAuthenticationMode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto modes = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeStringVector, 1 );

    try {
        createZwaveClient( luaState ).setAuthenticationMode( modes );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to set an authentication modes (%s) : %s", json_write( modes ).c_str(), e.what() );
    }

    return 0;
}

int setDeviceSpecificKey( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto key = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );

    try {
        createZwaveClient( luaState ).setDeviceSpecificKey( static_cast< uint16_t >( key ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to set an DSK (%d) : %s", key, e.what() );
    }

    return 0;
}

int setDevicesPollIntervals( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 );

    const auto useFixedWakeUpInterval = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeBool, 1 );
    const auto wakeUpInterval = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeUint32, 2 );
    const auto alwaysOnDevicesInetrval = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeUint32, 3 );
    const auto attemptsCount = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );

    try {
        createZwaveClient( luaState )
            .setDevicesPollIntervals( useFixedWakeUpInterval, wakeUpInterval, alwaysOnDevicesInetrval, attemptsCount );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to set device poll intervals "
            "(useFixedWakeUpInterval: %s, wakeUpInterval: %d, alwaysOnDevicesInetrval: %d, attemptsCount: %d) : %s",
            useFixedWakeUpInterval ? "true" : "false",
            wakeUpInterval,
            alwaysOnDevicesInetrval,
            attemptsCount,
            e.what() );
    }

    return 0;
}

int setUnresponsiveDeviceIgnoreInterval( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto interval = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeUint32, 1 );

    try {
        createZwaveClient( luaState ).setUnresponsiveDeviceIgnoreInterval( interval );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to set unresponsive device ignore interval %d", interval, e.what() );
    }

    return 0;
}

int setDefaultDeviceConfigurationsFile( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto deviceSettingFilePath = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    createZwaveClient( luaState ).setDefaultDeviceConfigurationsFile( deviceSettingFilePath );

    return 0;
}

int getAllNodesIds( lua_State* luaState ) {
    const auto json = createZwaveClient( luaState ).getAllNodesIds();
    return pushJsonAsLuaObj( luaState, json );
}

int getNode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto nodeId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto json = createZwaveClient( luaState ).getNodeAsJson( nodeId );
    return pushJsonAsLuaObj( luaState, json );
}

int deserializeNode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );
    const auto nodeBase64 = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto json = createZwaveClient( luaState ).nodeBase64ToJson( nodeBase64 );
    return pushJsonAsLuaObj( luaState, json );
}

int hasNode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto nodeId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    return pushJsonAsLuaObj( luaState, createZwaveClient( luaState ).hasNode( nodeId ) );
}

int getNodeInfo( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto nodeId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto json = createZwaveClient( luaState ).getNodeInfoAsJson( nodeId );
    return pushJsonAsLuaObj( luaState, json );
}

int getManufacturerName( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto manufacturerId = static_cast< uint16_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );

    const auto name = createZwaveClient( luaState ).getManufacturerName( manufacturerId );
    return pushJsonAsLuaObj( luaState, name );
}

int getCommandClass( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 );

    const auto nodeId = static_cast< uint8_t >( lua_tointeger( luaState, 1 ) );
    const auto channelId = static_cast< uint8_t >( lua_tointeger( luaState, 2 ) );
    const auto ccId = static_cast< uint8_t >( lua_tointeger( luaState, 3 ) );

    const auto json = createZwaveClient( luaState ).getCommandClassAsJson( nodeId, channelId, ccId );
    return pushJsonAsLuaObj( luaState, json );
}

int setValue( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    auto value = pullJson( luaState, 4 );
    const auto durationOpt = LuaToNative< NativeInteger >{}( luaState, 5 );
    createZwaveClient( luaState )
        .setValue( nodeId,
                   classId,
                   channelId,
                   value,
                   durationOpt.isValid() ? durationOpt.value() : common::optional< uint32_t >() );
    return 0;
}

int setValueMulticast( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = pullUint8Vector( luaState, 1 );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    auto value = pullJson( luaState, 4 );
    const auto mode = LuaToNative< NativeString >{}( luaState, 5 );

    try {
        createZwaveClient( luaState )
            .setValueMulticast( nodeId, classId, channelId, std::move( value ), mode.valueOr( std::string() ) );
    }
    catch( const std::exception& e ) {
        std::ostringstream buf;
        std::copy( nodeId.begin(), nodeId.end() - 1, std::ostream_iterator< uint8_t >( buf, ", " ) );
        buf << nodeId.back();

        return luaL_error( luaState,
                           "fail to set multicast value. node ids: [%s], channel id: %d, class id: %d. %s",
                           buf.str().c_str(),
                           channelId,
                           classId,
                           e.what() );
    }
    return 0;
}

int setTokenValue( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto value = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );

    try {
        createZwaveClient( luaState ).setTokenValue( nodeId, classId, channelId, value );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set token value. node id: %d, channel id: %d, class id: %d, "
                           "value : %s. %s",
                           nodeId,
                           channelId,
                           classId,
                           value.c_str(),
                           e.what() );
    }
    return 0;
}

int setTokenValueMulticast( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = pullUint8Vector( luaState, 1 );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    auto value = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );
    const auto mode = LuaToNative< NativeString >{}( luaState, 5 );

    try {
        createZwaveClient( luaState )
            .setTokenValueMulticast( nodeId, classId, channelId, std::move( value ), mode.valueOr( std::string() ) );
    }
    catch( const std::exception& e ) {
        std::ostringstream buf;
        std::copy( nodeId.begin(), nodeId.end() - 1, std::ostream_iterator< uint8_t >( buf, ", " ) );
        buf << nodeId.back();

        return luaL_error( luaState,
                           "fail to set multicast token value. node ids: [%s], channel id: %d, class id: %d. %s",
                           buf.str().c_str(),
                           channelId,
                           classId,
                           e.what() );
    }
    return 0;
}

int setRgbValue( lua_State* luaState ) {
    const auto nodeId = (uint8_t) lua_tonumber( luaState, 1 );
    const auto classId = (uint8_t) lua_tonumber( luaState, 2 );
    const auto channelId = (uint8_t) lua_tonumber( luaState, 3 );
    createZwaveClient( luaState ).setRgbValue( nodeId, classId, channelId, pullJson( luaState, 4 ) );
    return 0;
}

int setRgbValueMulticast( lua_State* luaState ) {
    const auto nodeId = pullUint8Vector( luaState, 1 );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    auto value = pullJson( luaState, 4 );
    const auto mode = LuaToNative< NativeString >{}( luaState, 5 );

    try {
        createZwaveClient( luaState )
            .setRgbValueMulticast( nodeId, classId, channelId, std::move( value ), mode.valueOr( std::string() ) );
    }
    catch( const std::exception& e ) {
        std::ostringstream buf;
        std::copy( nodeId.begin(), nodeId.end() - 1, std::ostream_iterator< uint8_t >( buf, ", " ) );
        buf << nodeId.back();

        return luaL_error( luaState,
                           "fail to set multicast rgb value. node ids: [%s], channel id: %d, class id: %d. %s",
                           buf.str().c_str(),
                           channelId,
                           classId,
                           e.what() );
    }
    return 0;
}

int setSoundSwitchConfiguration( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto volume = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto defaultToneId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );

    try {
        createZwaveClient( luaState ).setSoundSwitchConfiguration( nodeId, channelId, volume, defaultToneId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set sound switch configuration value. node id: %d, channel id: %d, volume: %d, "
                           "default tone id: %d. %s",
                           nodeId,
                           channelId,
                           volume,
                           defaultToneId,
                           e.what() );
    }

    return 0;
}

int requestSoundSwitchConfiguration( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestSoundSwitchConfiguration( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request sound switch configuration value. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }

    return 0;
}

int setSoundSwitchTonePlay( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto toneId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto volume = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );

    try {
        createZwaveClient( luaState ).setSoundSwitchTonePlay( nodeId, channelId, toneId, volume );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to set sound switch tone play value. node id: %d, channel id: %d, tone id: %d, volume: %d. %s",
            nodeId,
            channelId,
            toneId,
            volume,
            e.what() );
    }

    return 0;
}

int requestSoundSwitchTonePlay( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestSoundSwitchTonePlay( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request sound switch tone play value. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }

    return 0;
}

int setSetpointValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 5 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto setpointType = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );
    const auto scale = lookupLuaTable< NativeString >( luaState, "scale", 5, true );
    const auto value = lookupLuaTable< NativeNumber >( luaState, "value", 5, true );

    try {
        createZwaveClient( luaState )
            .setSetpointValue( nodeId, classId, channelId, setpointType, scale.value(), value.value() );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to set setpoint value. node id: %d, channel id: %d, class id: %d, setpoint type: %d. %s",
            nodeId,
            channelId,
            classId,
            setpointType,
            e.what() );
    }

    return 0;
}

int setConfigurationValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto paramNumber = static_cast< uint16_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto paramSize = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto value = static_cast< int64_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInt64, 4 ) );

    try {
        createZwaveClient( luaState ).setConfigurationValue( nodeId, paramNumber, paramSize, value );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to set configuration value. node id: %d, param number: %d, param length: %d, value : %d. %s",
            nodeId,
            paramNumber,
            paramSize,
            value,
            e.what() );
    }

    return 0;
}

int addConfiguration( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto paramNumber = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto paramSize = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );

    try {
        createZwaveClient( luaState ).addConfiguration( nodeId, paramNumber, paramSize );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to add a configuration. node id: %d, param number: %d, param length: %d",
                           nodeId,
                           paramNumber,
                           paramSize,
                           e.what() );
    }

    return 0;
}

int removeConfiguration( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto paramNumber = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).removeConfiguration( nodeId, paramNumber );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to remove a configuration. node id: %d, param number: %d", nodeId, paramNumber, e.what() );
    }

    return 0;
}

int addNodeToConfigurationGroup( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto groupId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto destinationNode = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );
    const auto destinationEndpoint = LuaToNative< NativeInteger >{}( luaState, 5 );

    try {
        createZwaveClient( luaState )
            .addNodeToConfigurationGroup( nodeId,
                                          channelId,
                                          groupId,
                                          destinationNode,
                                          destinationEndpoint.valueOr( UNDEFINED_TARGET_CHANNEL_ID ) );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to add node to association group. node id: %d, channel id: %d, group id: %d, destination node: %d, "
            "destination endpoint : %d. %s",
            nodeId,
            channelId,
            groupId,
            destinationNode,
            destinationEndpoint.valueOr( 0 ),
            e.what() );
    }

    return 0;
}

int removeNodeFromConfigurationGroup( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto groupId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto destinationNode = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );
    const auto destinationEndpoint = LuaToNative< NativeInteger >{}( luaState, 5 );

    try {
        createZwaveClient( luaState )
            .removeNodeFromConfigurationGroup( nodeId,
                                               channelId,
                                               groupId,
                                               destinationNode,
                                               destinationEndpoint.valueOr( UNDEFINED_TARGET_CHANNEL_ID ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to remove node from association group. node id: %d, channel id: %d, group id: %d, "
                           "destination node: %d, "
                           "destination endpoint : %d. %s",
                           nodeId,
                           channelId,
                           groupId,
                           destinationNode,
                           destinationEndpoint.valueOr( 0 ),
                           e.what() );
    }

    return 0;
}

int resetConfigurationValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto paramNumber = static_cast< uint16_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).resetConfigurationValue( nodeId, paramNumber );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to reset configuration value. node id: %d, param number: %d. %s",
                           nodeId,
                           paramNumber,
                           e.what() );
    }

    return 0;
}

int resetAllConfigurationValues( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );

    try {
        createZwaveClient( luaState ).resetAllConfigurationValues( nodeId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to reset all configuration values. node id: %d. %s", nodeId, e.what() );
    }

    return 0;
}

int requestConfigurationValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto paramNumber = static_cast< uint16_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestConfigurationValue( nodeId, paramNumber );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request configuration value. node id: %d, param number: %d. %s",
                           nodeId,
                           paramNumber,
                           e.what() );
    }

    return 0;
}

int switchMultilevelLevelChange( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 3 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto command = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto durationOpt = LuaToNative< NativeInteger >{}( luaState, 4 );

    createZwaveClient( luaState )
        .switchMultilevelLevelChange(
            nodeId, channelId, command, durationOpt.isValid() ? durationOpt.value() : common::optional< uint32_t >() );

    return 0;
}

int colorSwitchLevelChange( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto componentId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto command = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );
    const auto durationOpt = LuaToNative< NativeInteger >{}( luaState, 5 );

    createZwaveClient( luaState )
        .colorSwitchLevelChange( nodeId,
                                 channelId,
                                 componentId,
                                 command,
                                 durationOpt.isValid() ? durationOpt.value() : common::optional< uint32_t >() );

    return 0;
}

int requestValue( lua_State* luaState ) {
    const auto nodeId = (uint8_t) lua_tonumber( luaState, 1 );
    const auto classId = (uint8_t) lua_tonumber( luaState, 2 );
    const auto channelId = (uint8_t) lua_tonumber( luaState, 3 );
    createZwaveClient( luaState ).requestValue( nodeId, classId, channelId );
    return 0;
}

int requestSetpointValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto setpointType = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );

    try {
        createZwaveClient( luaState ).requestSetpointValue( nodeId, channelId, setpointType );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request setpoint value. node id: %d, channel id: %d, setpoint type: %d. %s",
                           nodeId,
                           channelId,
                           setpointType,
                           e.what() );
    }

    return 0;
}

int reset( lua_State* luaState ) {
    createZwaveClient( luaState ).reset();
    return 0;
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( Zwave::ZwaveAddon::Name ) );
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
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = ZWAVE_LUA_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules(
            luaState, Addons::Addon::createQualifiedName( Zwave::ZwaveAddon::Name ), subscriberId, rules );
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
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( Zwave::ZwaveAddon::Name ) );
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

int setDoorLockMode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto doorLockMode = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).setDoorLockMode( nodeId, channelId, doorLockMode );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set door lock mode. node id: %d, channel id: %d, door lock mode: %s. %s",
                           nodeId,
                           channelId,
                           doorLockMode.c_str(),
                           e.what() );
    }

    return 0;
}

int requestDoorLockState( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestDoorLockState( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to request door lock state. node id: %d, channel id: %d. %s", nodeId, channelId, e.what() );
    }

    return 0;
}

int setDoorLockConfiguration( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto jsonConfiguration = pullJson( luaState, 3 );

    using DoorLockConfiguration = Zwave::ZwaveClient::DoorLockConfiguration;
    using DoorLockExtendedConfiguration = Zwave::ZwaveClient::DoorLockExtendedConfiguration;
    using DoorLockExtendedConfigurationOpt = Zwave::ZwaveClient::DoorLockExtendedConfigurationOpt;

    using namespace JsonCustomGetters;

    struct {
        using Type = uint8_t;
        GettingResult operator()( Json::const_iterator it, Type& result, bool ) const {
            auto end = it->end();
            it = it->begin();

            result = 0;

            for( ; it != end; ++it ) {
                auto indexPtr = it->get< const Json::number_integer_t* >();
                if( !indexPtr ) {
                    return GettingResult::Error::Default;
                }
                if( *indexPtr >= 1 && *indexPtr <= 4 ) {
                    result |= 1 << ( *indexPtr - 1 );
                }
            }

            return GettingResult::Error::None;
        }
    } handlesMaskGetter;

    struct {
        using Type = Zwave::ZwaveClient::DoorLockConfigurationSecondsOpt;
        GettingResult operator()( Json::const_iterator it, Type& result, bool ) const {
            result.second = makeDefaultGetter< Type::first_type >()( *it, result.first );
            return GettingResult::Error::None;
        }
    } optTimeoutGetter;

    const auto configurationGetter = makeStructGetter(
        makeMemberMapping( &DoorLockConfiguration::enabledInsideHandleIdsBitmask,
                           makeFieldGetter( "enabled_inside_handle_ids", handlesMaskGetter ) ),
        makeMemberMapping( &DoorLockConfiguration::enabledOutsideHandleIdsBitmask,
                           makeFieldGetter( "enabled_outside_handle_ids", handlesMaskGetter ) ),
        makeMemberMapping( &DoorLockConfiguration::lockTimeoutOpt,
                           makeFieldGetter( "lock_timeout", optTimeoutGetter, OptionalMemberPolicy{} ) ) );

    const auto extendedConfigurationGetter = makeStructGetter(
        makeMemberMapping( &DoorLockExtendedConfiguration::autoRelockTimeout, "auto_relock_timeout" ),
        makeMemberMapping( &DoorLockExtendedConfiguration::holdAndReleaseTimeout, "hold_and_release_timeout" ),
        makeMemberMapping( &DoorLockExtendedConfiguration::isTwistAssistEnabled, "twist_assist_enabled" ),
        makeMemberMapping( &DoorLockExtendedConfiguration::isBlockToBlockEnabled, "block_to_block_enabled" ) );

    DoorLockConfiguration configuration;
    DoorLockExtendedConfigurationOpt extendedConfiguration( {}, false );

    if( !configurationGetter( jsonConfiguration, configuration ) ) {
        throw std::runtime_error( "Invalid configuration v1-3 fields: " + jsonConfiguration.dump() );
    }

    extendedConfiguration.second = extendedConfigurationGetter( jsonConfiguration, extendedConfiguration.first );

    if( !extendedConfiguration.second &&
        ( jsonConfiguration.count( "auto_relock_timeout" ) || jsonConfiguration.count( "hold_and_release_timeout" ) ||
          jsonConfiguration.count( "twist_assist_enabled" ) || jsonConfiguration.count( "block_to_block_enabled" ) ) ) {
        throw std::runtime_error( "Not all of config v4 fields are valid: " + jsonConfiguration.dump() );
    }

    createZwaveClient( luaState ).setDoorLockConfiguration( nodeId, channelId, configuration, extendedConfiguration );

    return 0;
}

int requestDoorLockConfiguration( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestDoorLockConfiguration( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request door lock configuration. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }

    return 0;
}

int requestUserCode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto userId = LuaToNative< NativeInteger >{}( luaState, 3 );

    try {
        if( userId.isValid() ) {
            createZwaveClient( luaState ).requestUserCode( nodeId, channelId, userId.value() );
        }
        else {
            createZwaveClient( luaState ).requestUserCodes( nodeId, channelId );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request user code. node id: %d, channel id: %d, user id: %s. %s",
                           nodeId,
                           channelId,
                           userId.isValid() ? std::to_string( userId.value() ).c_str() : "all",
                           e.what() );
    }

    return 0;
}

int requestUserCodesKeypadMode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestUserCodesKeypadMode( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request user codes keypad mode. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }

    return 0;
}

int setUserCodesKeypadMode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto mode = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).setUserCodesKeypadMode( nodeId, channelId, mode );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set user codes keypad mode. node id: %d, channel id: %d, mode: %s. %s",
                           nodeId,
                           channelId,
                           mode.c_str(),
                           e.what() );
    }

    return 0;
}

int setMasterCode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto code = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).setMasterCode( nodeId, channelId, code );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set master code. node id: %d, channel id: %d, code: %s. %s",
                           nodeId,
                           channelId,
                           code.c_str(),
                           e.what() );
    }

    return 0;
}

int requestMasterCode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestMasterCode( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to request master code. node id: %d, channel id: %d. %s", nodeId, channelId, e.what() );
    }

    return 0;
}

int requestUserCodesChecksum( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).requestUserCodesChecksum( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request user codes checksum. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }

    return 0;
}

int calculateUserCodesChecksum( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto jsonUserCodesArray = pullJson( luaState, 3 );

    using UserCodeElement = Zwave::ZwaveClient::UserCodeElement;
    using namespace JsonCustomGetters;

    std::vector< Zwave::ZwaveClient::UserCodeElement > userCodes;

    const auto getter = makeVectorGetter( makeStructGetter( makeMemberMapping( &UserCodeElement::id, "id" ),
                                                            makeMemberMapping( &UserCodeElement::code, "code" ),
                                                            makeMemberMapping( &UserCodeElement::status, "mode" ) ) );

    if( !jsonUserCodesArray.empty() && !getter( jsonUserCodesArray, userCodes ) ) {
        return luaL_error( luaState, "Invalid user codes array:\n%s", jsonUserCodesArray.dump().c_str() );
    }

    try {
        if( !jsonUserCodesArray.empty() && !getter( jsonUserCodesArray, userCodes ) ) {
            throw std::runtime_error( "Invalid user codes array: " + jsonUserCodesArray.dump() );
        }

        const auto checksum = createZwaveClient( luaState ).calculateUserCodesChecksum( userCodes );
        lua_pushinteger( luaState, checksum );
        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to calculate user codes checksum. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }
}

int requestAlarmSensorValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto sensorName = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).requestAlarmSensorValue( nodeId, channelId, sensorName );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request alarm sensor value. node id: %d, channel id: %d, sensor name: %s. %s",
                           nodeId,
                           channelId,
                           sensorName.c_str(),
                           e.what() );
    }

    return 0;
}

int requestNodeValues( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 2 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto requestDelay = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeUint32, 2 );
    const auto unsetPinCodesCountToStop = LuaToNative< NativeInteger >{}( luaState, 3 );
    const auto force = LuaToNative< NativeBool >{}( luaState, 4 );

    try {
        createZwaveClient( luaState )
            .requestNodeValues( nodeId,
                                std::chrono::milliseconds{ requestDelay },
                                unsetPinCodesCountToStop.valueOr( 0 ),
                                force.valueOr( false ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to request node (%d) values. %s", nodeId, e.what() );
    }

    return 0;
}

int requestAssociationGroup( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto groupId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );

    try {
        createZwaveClient( luaState ).requestAssociationGroup( nodeId, channelId, groupId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request association group. node id: %d, channel id: %d, group id: %d. %s",
                           nodeId,
                           channelId,
                           groupId,
                           e.what() );
    }

    return 0;
}

int sendIdentify( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    try {
        createZwaveClient( luaState ).sendIdentify( nodeId, channelId );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to send identify command. node id: %d, channel id: %d. %s", nodeId, channelId, e.what() );
    }

    return 0;
}

int setUserCode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 5 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    const Zwave::ZwaveClient::UserCodeElement userCode{ .id = static_cast< uint16_t >(
                                                            ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) ),
                                                        .status = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 ),
                                                        .code = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 5 ) };

    try {
        createZwaveClient( luaState ).setUserCode( nodeId, channelId, userCode );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState,
            "fail to request user code. node id: %d, channel id: %d, user id: %d, status: %s, code: %s. %s",
            nodeId,
            channelId,
            userCode.id,
            userCode.status.c_str(),
            userCode.code.c_str(),
            e.what() );
    }

    return 0;
}

int setSilentSensors( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto delay = static_cast< uint16_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto sensorNames = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeStringVector, 4 );

    try {
        createZwaveClient( luaState ).setSilentSensors( nodeId, channelId, delay, sensorNames );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to disable sensors sounding. node id: %d, channel id: %d, time id: %d. %s",
                           nodeId,
                           channelId,
                           time,
                           e.what() );
    }

    return 0;
}

namespace {
int requestScheduleEntryLockValue( lua_State* luaState,
                                   void ( Zwave::ZwaveClient::*method )( uint8_t, uint8_t, uint8_t, uint8_t ),
                                   const char* name ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto userId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto slotId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );

    try {
        ( createZwaveClient( luaState ).*method )( nodeId, channelId, userId, slotId );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request %s schedule. node id: %d, channel id: %d, user id: %d, slot id: %d. %s",
                           name,
                           nodeId,
                           channelId,
                           userId,
                           slotId,
                           e.what() );
    }

    return 0;
}

template < typename Type, typename Method, typename ValueGetter >
int setScheduleEntryLockValue( lua_State* luaState, Method method, ValueGetter&& valueGetter, const char* name ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 );

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto userId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto slotId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );

    std::pair< Type, bool > valueOpt{ {}, lua_gettop( luaState ) > 4 && lua_type( luaState, 5 ) != LUA_TNIL };

    try {
        if( valueOpt.second ) {
            const auto jsonValue = pullJson( luaState, 5 );
            if( !valueGetter( jsonValue, valueOpt.first ) ) {
                throw std::runtime_error( std::string( name ) + " schedule is invalid: " + jsonValue.dump() );
            }
        }

        ( createZwaveClient( luaState ).*method )( nodeId, channelId, userId, slotId, valueOpt );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set %s schedule. node id: %d, channel id: %d, user id: %d, slot id: %d. %s",
                           name,
                           nodeId,
                           channelId,
                           userId,
                           slotId,
                           e.what() );
    }

    return 0;
}

}  // namespace

int setWeekDayEntryLockSchedule( lua_State* luaState ) {
    using Schedule = Zwave::ZwaveClient::WeekDayEntryLockSchedule;
    using namespace JsonCustomGetters;
    using namespace Zwave::Resources::ScriptEvent;

    static const auto getter = makeStructGetter( makeMemberMapping( &Schedule::startHour, Param::StartHour ),
                                                 makeMemberMapping( &Schedule::startMin, Param::StartMin ),
                                                 makeMemberMapping( &Schedule::stopHour, Param::StopHour ),
                                                 makeMemberMapping( &Schedule::stopMin, Param::StopMin ),
                                                 makeMemberMapping( &Schedule::weekDay, Param::WeekDay ) );
    return setScheduleEntryLockValue< Schedule >(
        luaState, &Zwave::ZwaveClient::setWeekDayEntryLockSchedule, getter, "week day" );
}

int requestWeekDayEntryLockSchedule( lua_State* luaState ) {
    return requestScheduleEntryLockValue( luaState, &Zwave::ZwaveClient::requestWeekDayEntryLockSchedule, "week day" );
}

int setYearDayEntryLockSchedule( lua_State* luaState ) {
    using Schedule = Zwave::ZwaveClient::YearDayEntryLockSchedule;
    using namespace JsonCustomGetters;
    using namespace Zwave::Resources::ScriptEvent;

    static const auto getter = makeStructGetter( makeMemberMapping( &Schedule::startYear, Param::StartYear ),
                                                 makeMemberMapping( &Schedule::startMonth, Param::StartMonth ),
                                                 makeMemberMapping( &Schedule::startDay, Param::StartDay ),
                                                 makeMemberMapping( &Schedule::startHour, Param::StartHour ),
                                                 makeMemberMapping( &Schedule::startMin, Param::StartMin ),
                                                 makeMemberMapping( &Schedule::stopYear, Param::StopYear ),
                                                 makeMemberMapping( &Schedule::stopMonth, Param::StopMonth ),
                                                 makeMemberMapping( &Schedule::stopDay, Param::StopDay ),
                                                 makeMemberMapping( &Schedule::stopHour, Param::StopHour ),
                                                 makeMemberMapping( &Schedule::stopMin, Param::StopMin ) );
    return setScheduleEntryLockValue< Schedule >(
        luaState, &Zwave::ZwaveClient::setYearDayEntryLockSchedule, getter, "year day" );
}

int requestYearDayEntryLockSchedule( lua_State* luaState ) {
    return requestScheduleEntryLockValue( luaState, &Zwave::ZwaveClient::requestYearDayEntryLockSchedule, "year day" );
}

int setDailyRepeatingEntryLockSchedule( lua_State* luaState ) {
    using Schedule = Zwave::ZwaveClient::DailyRepeatingEntryLockSchedule;
    using namespace JsonCustomGetters;
    using namespace Zwave::Resources::ScriptEvent;

    static const auto getter = makeStructGetter( makeMemberMapping( &Schedule::startHour, Param::StartHour ),
                                                 makeMemberMapping( &Schedule::startMin, Param::StartMin ),
                                                 makeMemberMapping( &Schedule::durationHour, Param::DurationHour ),
                                                 makeMemberMapping( &Schedule::durationMin, Param::DurationMin ),
                                                 makeMemberMapping( &Schedule::weekDays, Param::WeekDays ) );

    return setScheduleEntryLockValue< Schedule >(
        luaState, &Zwave::ZwaveClient::setDailyRepeatingEntryLockSchedule, getter, "daily repeating" );
}

int requestDailyRepeatingEntryLockSchedule( lua_State* luaState ) {
    return requestScheduleEntryLockValue(
        luaState, &Zwave::ZwaveClient::requestDailyRepeatingEntryLockSchedule, "daily repeating" );
}

int setEntryLockScheduleEnable( lua_State* luaState ) {
    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto userId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto enable = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeBool, 4 ) );

    try {
        createZwaveClient( luaState ).setEntryLockScheduleEnable( nodeId, channelId, userId, enable );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set schedule entry lock enable. node id: %d, channel id: %d. %s",
                           nodeId,
                           channelId,
                           e.what() );
    }

    return 1;
}

int isScheduleStructureAcceptable( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto scheduleId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto scheduleIdBlock = LuaToNative< NativeUint8 >{}( luaState, 4 );

    bool hasValue = ZWAVE_LUA_HAS_INPUT_PARAMETERS( 5 );

    std::pair< bool, std::string > acceptable;

    if( hasValue ) {
        auto jsonValue = pullJson( luaState, 5 );
        acceptable = createZwaveClient( luaState )
                         .isScheduleStructureAcceptable(
                             nodeId,
                             channelId,
                             scheduleId,
                             scheduleIdBlock.isValid() ? scheduleIdBlock.value() : common::optional< uint8_t >(),
                             std::move( jsonValue ) );
    }
    else {
        acceptable = std::make_pair( false,
                                     std::string( "The schedule's value isn't acceptable. node id: " ) +
                                         std::to_string( nodeId ) + std::string( ", channel id: " ) +
                                         std::to_string( channelId ) );
    }

    pushJsonAsLuaObj( luaState, JSON( "acceptable" << acceptable.first << "message" << acceptable.second ) );
    return 1;
}

int setSchedule( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto scheduleId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto scheduleIdBlock = LuaToNative< NativeUint8 >{}( luaState, 4 );

    bool hasValue = ZWAVE_LUA_HAS_INPUT_PARAMETERS( 5 );

    try {
        auto jsonValue = JSON_EMPTY_OBJ;

        if( hasValue ) {
            jsonValue = pullJson( luaState, 5 );
        }

        createZwaveClient( luaState )
            .setSchedule( nodeId,
                          channelId,
                          scheduleId,
                          scheduleIdBlock.isValid() ? scheduleIdBlock.value() : common::optional< uint8_t >(),
                          std::move( jsonValue ) );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to set schedule info. node id: %d, channel id: %d. %s", nodeId, channelId, e.what() );
    }

    return 0;
}

int requestSchedule( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto scheduleId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto scheduleIdBlock = LuaToNative< NativeUint8 >{}( luaState, 4 );
    const auto activeIdCtl = LuaToNative< NativeBool >{}( luaState, 5 );

    createZwaveClient( luaState )
        .requestSchedule( nodeId,
                          channelId,
                          scheduleId,
                          scheduleIdBlock.isValid() ? scheduleIdBlock.value() : common::optional< uint8_t >(),
                          activeIdCtl.isValid() ? activeIdCtl.value() : common::optional< bool >() );

    return 0;
}

int setScheduleState( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 5 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto scheduleId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto scheduleIdBlock = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );
    const auto scheduleState = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 5 ) );

    createZwaveClient( luaState ).setScheduleState( nodeId, channelId, scheduleId, scheduleIdBlock, scheduleState );
    return 0;
}

int requestScheduleState( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto scheduleIdBlock = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );

    createZwaveClient( luaState ).requestScheduleState( nodeId, channelId, scheduleIdBlock );
    return 0;
}

int setBarrierSignalValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto signalName = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto value = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 4 ) );

    try {
        createZwaveClient( luaState ).setBarrierSignalValue( nodeId, channelId, signalName, value );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to set barrier signal value. node id %u, channel id %u, signal name %s, value %u, %s",
                           nodeId,
                           channelId,
                           signalName.c_str(),
                           value,
                           e.what() );
    }

    return 0;
}

int requestBarrierSignalValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto signalName = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).requestBarrierSignalValue( nodeId, channelId, signalName );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request barrier signal value. node id %u, channel id %u, signal name %s, %s",
                           nodeId,
                           channelId,
                           signalName.c_str(),
                           e.what() );
    }

    return 0;
}

int reconstructNetwork( lua_State* luaState ) {
    createZwaveClient( luaState ).reconstructNetwork();
    return 0;
}

int resetMeter( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    const auto value = LuaToNative< NativeInteger >{}( luaState, 3 );
    const auto size = LuaToNative< NativeInteger >{}( luaState, 4 );

    try {
        createZwaveClient( luaState )
            .resetMeter( nodeId,
                         channelId,
                         value.valueOr( 0 ),
                         value.isValid() ? static_cast< uint8_t >( size.valueOr( 4 ) ) : 0 );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to reset meter. node id %u, channel id %u, %s", nodeId, channelId, e.what() );
    }

    return 0;
}

int requestMeter( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto scale = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).requestMeter( nodeId, channelId, scale );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState,
                           "fail to request meter. node id %u, channel id %u, scale %s, %s",
                           nodeId,
                           channelId,
                           scale.c_str(),
                           e.what() );
    }

    return 0;
}

int updateNodesFirmware( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeIds = pullUint8Vector( luaState, 1 );
    const auto jsFirmwaresInfo = pullJson( luaState, 2 );

    using namespace JsonCustomGetters;
    using NodeFirmwareInfo = Zwave::ZwaveClient::NodeFirmwareInfo;

    const auto getter = makeVectorGetter(
        makeStructGetter( makeMemberMapping( &NodeFirmwareInfo::target, "target" ),
                          makeMemberMapping( &NodeFirmwareInfo::image, "image" ),
                          makeMemberMapping( &NodeFirmwareInfo::checksum, "checsum", OptionalMemberPolicy() ) ) );

    std::vector< NodeFirmwareInfo > firmwaresInfo;

    getter( jsFirmwaresInfo, firmwaresInfo );

    try {
        if( !getter( jsFirmwaresInfo, firmwaresInfo ) ) {
            throw std::runtime_error( "Fail to get firmwares info form " + jsFirmwaresInfo.dump() );
        }
        createZwaveClient( luaState ).updateNodesFirmware( nodeIds, firmwaresInfo );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to update nodes firmware. %s", e.what() );
    }

    return 0;
}

int downloadNodeFirmware( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto target = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto path = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );

    try {
        createZwaveClient( luaState ).downloadNodeFirmware( nodeId, target, path );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to download firmware. %s", e.what() );
    }

    return 0;
}

int getProvisioningInfo( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );

    try {
        const auto json = createZwaveClient( luaState ).getProvisioningInfoAsJson( nodeId );
        return pushJsonAsLuaObj( luaState, json );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get provision info for node %d: %s", nodeId, e.what() );
    }
}

int setProvisioningInfo( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );

    std::unique_ptr< std::string > name;
    std::unique_ptr< std::string > room;

    if( !isParameterNil( luaState, 2 ) ) {
        name = std::make_unique< std::string >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 2 ) );
    }
    if( !isParameterNil( luaState, 3 ) ) {
        room = std::make_unique< std::string >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 ) );
    }

    try {
        createZwaveClient( luaState ).setProvisioningInfo( nodeId, name, room );
        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to set provision info for node %d: %s", nodeId, e.what() );
    }
}

int getInfo( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 0 )

    try {
        const auto info = createZwaveClient( luaState ).getInfo();
        return pushJsonAsLuaObj( luaState, info );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get info: %s", e.what() );
    }
}

int getFirmwareRegion( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 0 )

    try {
        const std::string region = createZwaveClient( luaState ).getFirmwareRegion();
        lua_pushlstring( luaState, region.c_str(), region.size() );
        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get info: %s", e.what() );
    }
}

int startRediscover( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto settingsFilePath = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 2 );

    try {
        createZwaveClient( luaState ).startRediscover( nodeId, settingsFilePath );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get info: %s", e.what() );
    }

    return 0;
}

int requestNotificationValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto type = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto eventType = LuaToNative< NativeString >{}( luaState, 4 );

    createZwaveClient( luaState )
        .requestNotificationValue( nodeId, channelId, type, eventType.valueOr( std::string() ) );
    return 0;
}

int requestAlarmV1Value( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto type = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );

    createZwaveClient( luaState ).requestAlarmV1Value( nodeId, channelId, type );
    return 0;
}

int setThermostatSetbackValue( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto overrideType = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto mode = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );
    const auto value = LuaToNative< NativeNumber >{}( luaState, 5 );

    createZwaveClient( luaState )
        .setThermostatSetbackValue(
            nodeId,
            channelId,
            overrideType,
            mode,
            Zwave::ZwaveClient::ThermostatSetbackValueOpt( value.valueOr( 0.f ), value.isValid() ) );
    return 0;
}

int checkReachableStatus( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );

    createZwaveClient( luaState ).checkReachableStatus( nodeId );
    return 0;
}

int checkNodeValues( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto requestDelay = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeUint32, 2 );

    createZwaveClient( luaState )
        .checkNodeValues( nodeId, std::chrono::milliseconds{ requestDelay }, pullJson( luaState, 3 ) );
    return 0;
}

int lockAntiTheft( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    auto magicCode = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 3 );
    auto hint = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );

    createZwaveClient( luaState ).lockAntiTheft( nodeId, channelId, std::move( magicCode ), std::move( hint ) );

    return 0;
}

int lockAntiTheftDefault( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );

    createZwaveClient( luaState ).lockAntiTheftDefault( nodeId, channelId );

    return 0;
}

int unlockAntiTheft( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    auto magicCode = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );

    createZwaveClient( luaState ).unlockAntiTheft( nodeId, channelId, classId, std::move( magicCode ) );

    return 0;
}

int unlockAntiTheftDefault( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 4 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto classId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    auto hint = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 4 );

    createZwaveClient( luaState ).unlockAntiTheftDefault( nodeId, channelId, classId, std::move( hint ) );

    return 0;
}

int addDeviceSpecificConfiguration( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto deviceTypeId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto configuration = pullJson( luaState, 2 );

    createZwaveClient( luaState ).addDeviceSpecificConfiguration( deviceTypeId, configuration );

    return 0;
}

int addDeviceSpecificAssociation( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto deviceTypeId = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto association = pullJson( luaState, 2 );

    createZwaveClient( luaState ).addDeviceSpecificAssociation( deviceTypeId, association );

    return 0;
}

int clearDeviceSpecific( lua_State* luaState ) {
    createZwaveClient( luaState ).clearDeviceSpecific();
    return 0;
}

int setThermostatMode( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_OPTIONAL_PARAMETERS( 3 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto channelId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 2 ) );
    const auto mode = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 3 ) );
    const auto manufacturerSpecificDataOpt = LuaToNative< NativeString >{}( luaState, 4 );

    createZwaveClient( luaState )
        .setThermostatMode( nodeId, channelId, mode, manufacturerSpecificDataOpt.valueOr( std::string() ) );

    return 0;
}

int synchronizeDeviceTime( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 1 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );

    createZwaveClient( luaState ).synchronizeDeviceTime( nodeId );

    return 0;
}

int setWakeUpInterval( lua_State* luaState ) {
    ZWAVE_LUA_CHECK_INPUT_PARAMETERS( 2 )

    const auto nodeId = static_cast< uint8_t >( ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeInteger, 1 ) );
    const auto value = ZWAVE_LUA_GET_INPUT_PARAMETERS( NativeUint32, 2 );

    createZwaveClient( luaState ).setWakeUpInterval( nodeId, value );

    return 0;
}

const char* const moduleName = "zwave";
const luaL_Reg module[] = {
    { "start_include", errorCatchWrapper< startInclude > },
    { "start_exclude", errorCatchWrapper< startExclude > },
    { "stop_include", errorCatchWrapper< stopInclude > },
    { "stop_exclude", errorCatchWrapper< stopExclude > },
    { "start_learn", errorCatchWrapper< startLearn > },
    { "stop_learn", errorCatchWrapper< stopLearn > },
    { "stop_security_include", errorCatchWrapper< stopSecurityInclude > },
    { "set_auth_mode", errorCatchWrapper< setAuthenticationMode > },
    { "set_device_specific_key", errorCatchWrapper< setDeviceSpecificKey > },
    { "set_devices_poll_intervals", errorCatchWrapper< setDevicesPollIntervals > },
    { "set_unresponsive_device_ignore_interval", errorCatchWrapper< setUnresponsiveDeviceIgnoreInterval > },
    { "set_default_device_configurations_file", errorCatchWrapper< setDefaultDeviceConfigurationsFile > },
    { "set_door_lock_mode", errorCatchWrapper< setDoorLockMode > },
    { "request_door_lock_state", errorCatchWrapper< requestDoorLockState > },
    { "set_door_lock_configuration", errorCatchWrapper< setDoorLockConfiguration > },
    { "request_door_lock_configuration", errorCatchWrapper< requestDoorLockConfiguration > },
    { "subscribe", errorCatchWrapper< subscribe > },
    { "set_subscription_filters", errorCatchWrapper< setSubscriptionFilters > },
    { "unsubscribe", errorCatchWrapper< unsubscribe > },
    { "get_all_nodes_ids", errorCatchWrapper< getAllNodesIds > },
    { "get_node", errorCatchWrapper< getNode > },
    { "deserialize_node", errorCatchWrapper< deserializeNode > },
    { "has_node", errorCatchWrapper< hasNode > },
    { "get_node_info", errorCatchWrapper< getNodeInfo > },
    { "get_command_class", errorCatchWrapper< getCommandClass > },
    { "set_value", errorCatchWrapper< setValue > },
    { "set_value_multicast", errorCatchWrapper< setValueMulticast > },
    { "set_token_value", errorCatchWrapper< setTokenValue > },
    { "set_token_value_multicast", errorCatchWrapper< setTokenValueMulticast > },
    { "set_rgb_value", errorCatchWrapper< setRgbValue > },
    { "set_rgb_value_multicast", errorCatchWrapper< setRgbValueMulticast > },
    { "set_sound_switch_configuration", errorCatchWrapper< setSoundSwitchConfiguration > },
    { "request_sound_switch_configuration", errorCatchWrapper< requestSoundSwitchConfiguration > },
    { "set_sound_switch_tone_play", errorCatchWrapper< setSoundSwitchTonePlay > },
    { "request_sound_switch_tone_play", errorCatchWrapper< requestSoundSwitchTonePlay > },
    { "set_setpoint_value", errorCatchWrapper< setSetpointValue > },
    { "set_configuration_value", errorCatchWrapper< setConfigurationValue > },
    { "add_configuration", errorCatchWrapper< addConfiguration > },
    { "remove_configuration", errorCatchWrapper< removeConfiguration > },
    { "add_node_to_association_group", errorCatchWrapper< addNodeToConfigurationGroup > },
    { "remove_node_from_association_group", errorCatchWrapper< removeNodeFromConfigurationGroup > },
    { "reset_configuration_value", errorCatchWrapper< resetConfigurationValue > },
    { "reset_all_configuration_values", errorCatchWrapper< resetAllConfigurationValues > },
    { "request_configuration_value", errorCatchWrapper< requestConfigurationValue > },
    { "switch_multilevel_level_change", errorCatchWrapper< switchMultilevelLevelChange > },
    { "color_switch_level_change", errorCatchWrapper< colorSwitchLevelChange > },
    { "request_value", errorCatchWrapper< requestValue > },
    { "request_setpoint_value", errorCatchWrapper< requestSetpointValue > },
    { "request_user_code", errorCatchWrapper< requestUserCode > },
    { "request_user_codes_checksum", errorCatchWrapper< requestUserCodesChecksum > },
    { "calculate_user_codes_checksum", errorCatchWrapper< calculateUserCodesChecksum > },
    { "request_master_code", errorCatchWrapper< requestMasterCode > },
    { "set_master_code", errorCatchWrapper< setMasterCode > },
    { "request_user_codes_keypad_mode", errorCatchWrapper< requestUserCodesKeypadMode > },
    { "set_user_codes_keypad_mode", errorCatchWrapper< setUserCodesKeypadMode > },
    { "request_alarm_sensor_value", errorCatchWrapper< requestAlarmSensorValue > },
    { "request_node_values", errorCatchWrapper< requestNodeValues > },
    { "request_association_group", errorCatchWrapper< requestAssociationGroup > },
    { "set_user_code", errorCatchWrapper< setUserCode > },
    { "set_silent_sensors", errorCatchWrapper< setSilentSensors > },
    { "set_week_day_entry_lock_schedule", errorCatchWrapper< setWeekDayEntryLockSchedule > },
    { "request_week_day_entry_lock_schedule", errorCatchWrapper< requestWeekDayEntryLockSchedule > },
    { "set_year_day_entry_lock_schedule", errorCatchWrapper< setYearDayEntryLockSchedule > },
    { "request_year_day_entry_lock_schedule", errorCatchWrapper< requestYearDayEntryLockSchedule > },
    { "set_daily_repeating_entry_lock_schedule", errorCatchWrapper< setDailyRepeatingEntryLockSchedule > },
    { "request_daily_repeating_entry_lock_schedule", errorCatchWrapper< requestDailyRepeatingEntryLockSchedule > },
    { "set_entry_lock_schedule_enable", errorCatchWrapper< setEntryLockScheduleEnable > },
    { "set_barrier_signal_value", errorCatchWrapper< setBarrierSignalValue > },
    { "request_barrier_signal_value", errorCatchWrapper< requestBarrierSignalValue > },
    { "send_identify", errorCatchWrapper< sendIdentify > },
    { "reset", errorCatchWrapper< reset > },
    { "get_manufacturer_name", errorCatchWrapper< getManufacturerName > },
    { "reconstruct_network", errorCatchWrapper< reconstructNetwork > },
    { "reset_meter", errorCatchWrapper< resetMeter > },
    { "request_meter", errorCatchWrapper< requestMeter > },
    { "update_nodes_firmware", errorCatchWrapper< updateNodesFirmware > },
    { "download_node_firmware", errorCatchWrapper< downloadNodeFirmware > },
    { "get_provisioning_info", errorCatchWrapper< getProvisioningInfo > },
    { "set_provisioning_info", errorCatchWrapper< setProvisioningInfo > },
    { "get_info", errorCatchWrapper< getInfo > },
    { "get_firmware_region", errorCatchWrapper< getFirmwareRegion > },
    { "start_rediscover", errorCatchWrapper< startRediscover > },
    { "request_notification_value", errorCatchWrapper< requestNotificationValue > },
    { "request_alarm_v1_value", errorCatchWrapper< requestAlarmV1Value > },
    { "set_thermostat_setback_value", errorCatchWrapper< setThermostatSetbackValue > },
    { "check_reachable_status", errorCatchWrapper< checkReachableStatus > },
    { "check_node_values", errorCatchWrapper< checkNodeValues > },
    { "lock_anti_theft", errorCatchWrapper< lockAntiTheft > },
    { "lock_anti_theft_default", errorCatchWrapper< lockAntiTheftDefault > },
    { "unlock_anti_theft", errorCatchWrapper< unlockAntiTheft > },
    { "unlock_anti_theft_default", errorCatchWrapper< unlockAntiTheftDefault > },
    { "add_device_specific_configuration", errorCatchWrapper< addDeviceSpecificConfiguration > },
    { "add_device_specific_association", errorCatchWrapper< addDeviceSpecificAssociation > },
    { "clear_device_specific", errorCatchWrapper< clearDeviceSpecific > },
    { "set_thermostat_mode", errorCatchWrapper< setThermostatMode > },
    { "synchronize_device_time", errorCatchWrapper< synchronizeDeviceTime > },
    { "set_wake_up_interval", errorCatchWrapper< setWakeUpInterval > },

    { "is_schedule_structure_acceptable", errorCatchWrapper< isScheduleStructureAcceptable > },
    { "set_schedule", errorCatchWrapper< setSchedule > },
    { "request_schedule", errorCatchWrapper< requestSchedule > },

    { "set_schedule_state", errorCatchWrapper< setScheduleState > },
    { "request_schedule_state", errorCatchWrapper< requestScheduleState > },

    { nullptr, nullptr }
};

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
