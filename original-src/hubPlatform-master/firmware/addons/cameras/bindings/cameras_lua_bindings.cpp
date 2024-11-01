#include "cameras/bindings/cameras_client.h"
#include "cameras/cameras_addon.h"
#include "cameras/common_res.h"

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "addon_interfaces/addon.h"
#include "jsonrpc/jsonrpc.h"

#define CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( number ) \
    CHECK_INPUT_PARAMETERS( ( std::string{ "cameras." } + __func__ ), number )

#define CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "cameras." } + __func__ ) )

#define CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "cameras." } + __func__ ), number )

namespace HubPlatform {
namespace {

constexpr int MinSegmentDurationSec = 30;

CamerasClient createCamerasClient( lua_State* luaState ) {
    auto& addon = dynamic_cast< Addons::Addon& >( getHostService( luaState ) );
    return CamerasClient( addon );
}

int checkError( lua_State* luaState, const Json& json ) {
    if( hasError( json ) ) {
        auto& error = json[ Errors::ERROR_FIELD ];
        std::string reason = json_get_str( error, "reason" );
        if( reason.empty() ) {
            reason = json_get_str( error, "message" );
        }
        Log::error( cc::error( "Cameras addon request failed: " ) + cc::j( json ) );
        return luaL_error( luaState, "Cameras addon request failed: %s", reason.c_str() );
    }
    return pushJsonAsLuaObj( luaState, true );
}

int subscribe( lua_State* luaState ) {
    auto eventsSubscriber =
        createFunctionalityCreator( luaState )
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( CamerasAddonName ) );
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
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 2 );

    const auto subscriberId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules(
            luaState, Addons::Addon::createQualifiedName( CamerasAddonName ), subscriberId, rules );
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
            .getPluginEventsSubscribeFunctionality( Addons::Addon::createQualifiedName( CamerasAddonName ) );
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

int addCameraStream( lua_State* luaState ) {
    CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( 3 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto streamName = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 2 );
    const auto rtspUrl = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 3 );
    const auto description = LuaToNative< NativeString >()( luaState, 4 );
    const auto pluginId = getPluginId( luaState );
    const auto json =
        createCamerasClient( luaState )
            .addCameraStream( pluginId, streamId, streamName, rtspUrl, description.valueOr( std::string() ) );
    return checkError( luaState, json );
}

int removeCameraStream( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto json = createCamerasClient( luaState ).removeCameraStream( streamId );
    return checkError( luaState, json );
}

int createLiveRtspStream( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    createCamerasClient( luaState ).createLiveRtspStream( streamId );
    return 0;
}

int deleteLiveRtspStream( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    createCamerasClient( luaState ).deleteLiveRtspStream( streamId );
    return 0;
}

int createStreamSavingToCloud( lua_State* luaState ) {
    CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( 2 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto segmentDurationSec = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto secondsBeforeStart = LuaToNative< NativeInteger >()( luaState, 3 );
    const auto segmentsLimit = LuaToNative< NativeInteger >()( luaState, 4 );
    const auto format = LuaToNative< NativeString >()( luaState, 5 );
    if( segmentDurationSec < MinSegmentDurationSec ) {
        std::string message = "Minimum segment duration: " + std::to_string( MinSegmentDurationSec ) + " sec";
        return luaL_error( luaState, message.c_str() );
    }
    createCamerasClient( luaState )
        .createStreamSavingToCloud( streamId,
                                    segmentDurationSec,
                                    secondsBeforeStart.valueOr( 0 ),
                                    segmentsLimit.valueOr( 0 ),
                                    format.valueOr( std::string() ) );
    return 0;
}

int deleteStreamSavingToCloud( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    createCamerasClient( luaState ).deleteStreamSavingToCloud( streamId );
    return 0;
}

int startStreamSavingToCloud( lua_State* luaState ) {
    CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto metaData = pullJson( luaState, 2 );
    const auto makeVideoParticle = LuaToNative< NativeBool >()( luaState, 3 );
    createCamerasClient( luaState ).startStreamSavingToCloud( streamId, metaData, makeVideoParticle.valueOr( false ) );
    return 0;
}

int stopStreamSavingToCloud( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    createCamerasClient( luaState ).stopStreamSavingToCloud( streamId );
    return 0;
}

int setVideoParticleSettings( lua_State* luaState ) {
    CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( 2 );
    const auto preRecordDurationSec = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto recordDurationSec = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto resolutionDivisor = LuaToNative< NativeInteger >()( luaState, 3 );
    const auto format = LuaToNative< NativeString >()( luaState, 4 );
    const auto pluginId = getPluginId( luaState );
    createCamerasClient( luaState )
        .setVideoParticleSettings( pluginId,
                                   preRecordDurationSec,
                                   recordDurationSec,
                                   resolutionDivisor.valueOr( 0 ),
                                   format.valueOr( std::string() ) );
    return 0;
}

int makeSnapshot( lua_State* luaState ) {
    CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( 2 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto snapshotId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 2 );
    const auto metaData = pullJson( luaState, 3 );
    createCamerasClient( luaState ).makeSnapshot( streamId, snapshotId, metaData );
    return 0;
}

int createAudioStream( lua_State* luaState ) {
    CHECK_CAMERAS_INPUT_OPTIONAL_PARAMETERS( 2 );
    Cameras::AudioStreamInfo info;
    info.streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    info.requestInfo = Cameras::RequestInfo::fromJson( pullJson( luaState, 2 ) );
    info.userData = pullJson( luaState, 3 );
    createCamerasClient( luaState ).createAudioStream( info );
    return 0;
}

int deleteAudioStream( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    createCamerasClient( luaState ).deleteAudioStream( streamId );
    return 0;
}

int getAudioStreamInfo( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto info = createCamerasClient( luaState ).getAudioStreamInfo( streamId );
    return pushJsonAsLuaObj( luaState, info );
}

int getStreamInfo( lua_State* luaState ) {
    CAMERAS_CAMERAS_CHECK_INPUT_PARAMETERS( 1 );
    const auto streamId = CAMERAS_CAMERAS_GET_INPUT_PARAMETERS( NativeString, 1 );
    const auto info = createCamerasClient( luaState ).getStreamInfo( streamId );
    if( !info.empty() )
        return pushJsonAsLuaObj( luaState, info );
    lua_pushnil( luaState );
    return 0;
}

const char* const moduleName = "cameras";
const luaL_Reg module[] = { { "subscribe", errorCatchWrapper< subscribe > },
                            { "set_subscription_filters", errorCatchWrapper< setSubscriptionFilters > },
                            { "unsubscribe", errorCatchWrapper< unsubscribe > },
                            { "add_camera_stream", errorCatchWrapper< addCameraStream > },
                            { "remove_camera_stream", errorCatchWrapper< removeCameraStream > },
                            { "create_live_rtsp_stream", errorCatchWrapper< createLiveRtspStream > },
                            { "delete_live_rtsp_stream", errorCatchWrapper< deleteLiveRtspStream > },
                            { "create_stream_saving_to_cloud", errorCatchWrapper< createStreamSavingToCloud > },
                            { "delete_stream_saving_to_cloud", errorCatchWrapper< deleteStreamSavingToCloud > },
                            { "start_stream_saving_to_cloud", errorCatchWrapper< startStreamSavingToCloud > },
                            { "stop_stream_saving_to_cloud", errorCatchWrapper< stopStreamSavingToCloud > },
                            { "set_video_particle_settings", errorCatchWrapper< setVideoParticleSettings > },
                            { "make_snapshot", errorCatchWrapper< makeSnapshot > },
                            { "create_audio_stream", errorCatchWrapper< createAudioStream > },
                            { "delete_audio_stream", errorCatchWrapper< deleteAudioStream > },
                            { "get_audio_stream_info", errorCatchWrapper< getAudioStreamInfo > },
                            { "get_stream_info", errorCatchWrapper< getStreamInfo > },
                            { nullptr, nullptr } };
}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
