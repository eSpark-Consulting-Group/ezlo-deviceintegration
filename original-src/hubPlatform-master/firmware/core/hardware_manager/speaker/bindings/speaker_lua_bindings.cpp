#include <utility>

#include "binding_interfaces/lua/lua_binding_interface.h"
#include "hardware_manager/speaker/remote/speaker_remote_interface.h"
#include "hardware_manager/speaker/speaker.h"
#include "hardware_manager/speaker/model/resources.h"
#include "hardware_manager/speaker/model/speaker_errors.h"
#include "hardware_manager/speaker/bindings/lua_params_checker.h"
#include "hardware_manager/speaker/model/resources.h"

#include "extensions_manager/remote_interface/em_remote_interface.h"

#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <cstdint>

#define CHECK_LUA_INPUT_PARAMETERS( number ) CHECK_INPUT_PARAMETERS( ( std::string{ "speaker." } + __func__ ), number )

#define CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "speaker." } + __func__ ), number )

#define GET_LUA_INPUT_PARAMETERS( type, idx ) \
    GET_INPUT_PARAMETERS( type, idx, ( std::string{ "speaker." } + __func__ ) )

namespace HubPlatform {
namespace LuaSpeaker {

using namespace HubPlatform::HardwareManager::Speaker;

struct SpeakerErrorsMixin {
    using RequiredParameter = RequiredParameterError;
    using NoFile = NoFileError;
    using FilePermissions = FilePermissionsError;
    using RequestPermissions = RequestPermissionsError;
    using WrongParameterFormat = WrongParameterFormatError;
    using ParameterLimit = ParameterLimitError;
    using TotalLimit = PlayingSoundsLimitError;
    using NoObject = CommandIdError;
};

using SpeakerParamsChecker = LuaParamsChecker< SpeakerErrorsMixin >;

/** Lua errors:
 *
 * Required parameter didn't define: <parameter_name>
 * The file doesn't exist: <file_path>
 * Plugin doesn't have permissions to the file: <file_path>
 * Wrong format of parameter: <parameter_name>
 * Limit of parameter is reached: <parameter_name>
 * Limit of playing sounds is reached
 */
int speakerPlay( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );
    const auto paramsJson = pullJson( luaState, 1 );

    using namespace HubPlatform::HardwareManager::Speaker;
    auto& service = getHostService( luaState );
    ExtensionsManager::RemoteInterface extManager( service );

    SpeakerRemoteImpl remote( service );

    try {
        const auto pluginId = getPluginId( luaState );

        std::string songFilePath;
        std::int64_t time = 0;
        std::uint8_t volume = 80;
        std::string handlerScript;
        Json userData;

        SpeakerParamsChecker paramsChecker( service, extManager, pluginId );

        {
            auto pathStr = paramsChecker.checkRequiredString( paramsJson, Fields::path );
            songFilePath = paramsChecker.makeFullPath( pathStr, false );
            paramsChecker.checkFullPathExists( songFilePath, pathStr );
            paramsChecker.checkPluginPermissions( pluginId, pathStr );
        }

        // volume: int
        auto volumeIt = paramsJson.find( Fields::volume );
        if( volumeIt != paramsJson.end() ) {
            auto value = *volumeIt;
            if( !value.is_number() )
                throw WrongParameterFormatError( Fields::volume );

            int intValue = value.get< std::int64_t >();
            using namespace HubPlatform::HardwareManager::Speaker;
            if( intValue < Resources::volumeRangeMin || intValue > Resources::volumeRangeMax )
                throw ParameterLimitError( Fields::volume );

            volume = static_cast< std::uint8_t >( intValue );
        }

        // time: int
        auto timeIt = paramsJson.find( Fields::time );
        if( timeIt != paramsJson.end() ) {
            auto value = *timeIt;
            if( !value.is_number() )
                throw WrongParameterFormatError( Fields::time );

            time = value.get< std::int64_t >();
        }

        // handler: string optional
        handlerScript = json_get_str( paramsJson, Fields::handler );
        if( !handlerScript.empty() ) {
            const auto handlerScriptFullPath = paramsChecker.makeFullPath( handlerScript );
            paramsChecker.checkFullPathExists( handlerScriptFullPath, handlerScript );
            paramsChecker.checkPluginPermissions( pluginId, handlerScript );
        }

        auto userDataIt = paramsJson.find( Fields::userData );
        if( userDataIt != paramsJson.end() ) {
            userData = *userDataIt;
        }

        const auto commandId = remote.play( songFilePath, time, volume, pluginId, handlerScript, userData );

        lua_pushstring( luaState, commandId.c_str() );
        return 1;
    }
    catch( const std::exception& err ) {
        const char* message = err.what();
        if( !message || !*message )
            message = "Failed to perform speaker.play() action";
        LOGGER_ERROR( cc::error( "speaker remote interface error: " ) + message );
        return luaL_error( luaState, "%s", message );
    }
}

int speakerStop( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );

    const auto commandId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    using namespace HubPlatform::HardwareManager::Speaker;
    auto& service = getHostService( luaState );
    ExtensionsManager::RemoteInterface extManager( service );

    std::string pluginId = getPluginId( luaState );

    LOGGER_TRACE( cc::trace( "Stop playing: " ) + cc::note( commandId ) );

    try {
        SpeakerRemoteImpl remote( service );

        if( commandId.empty() ) {
            throw RequiredParameterError( Lua::Fields::id );
        }

        auto soundInfo = remote.getPlayingInfo( commandId );
        if( soundInfo.getId().empty() || ( soundInfo.getPluginName() != pluginId ) ) {
            throw CommandIdError( commandId );
        }

        const auto result = remote.stop( commandId );
        lua_pushboolean( luaState, result );
        return 1;
    }
    catch( const std::exception& err ) {
        const char* message = err.what();
        if( !message || !*message )
            message = "Failed to perform speaker.stop() action";
        LOGGER_ERROR( message );
        return luaL_error( luaState, "%s", message );
    }
}

int speakerInfo( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );

    const auto commandId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    using namespace HubPlatform::HardwareManager::Speaker;
    auto& service = getHostService( luaState );
    ExtensionsManager::RemoteInterface extManager( service );

    std::string pluginId = getPluginId( luaState );

    try {
        SpeakerRemoteImpl remote( service );

        if( commandId.empty() ) {
            throw RequiredParameterError( Lua::Fields::id );
        }

        auto soundInfo = remote.getPlayingInfo( commandId );
        if( soundInfo.getId().empty() || ( soundInfo.getPluginName() != pluginId ) ) {
            throw CommandIdError( commandId );
        }

        Json resultInfo = { { Lua::Fields::id, soundInfo.getId() },
                            { Fields::status, toString( soundInfo.getStatus() ) },
                            { Fields::volume, soundInfo.getVolume() },
                            { Fields::path, soundInfo.getSongFilePath() } };

        if( !soundInfo.getHandlerScriptPath().empty() ) {
            resultInfo[ Fields::handler ] = soundInfo.getHandlerScriptPath();
        }

        if( !soundInfo.getUserData().empty() ) {
            resultInfo[ Fields::userData ] = soundInfo.getUserData();
        }

        auto ret = HubPlatform::pushJsonAsLuaObj( luaState, resultInfo );
        return 1;
    }
    catch( const std::exception& err ) {
        const char* message = err.what();
        if( !message || !*message )
            message = "Failed to perform speaker.getPlayingInfo() action";
        LOGGER_ERROR( message );
        return luaL_error( luaState, "%s", message );
    }
}

const char moduleName[] = "speaker";
const luaL_Reg module[] = { { "play", speakerPlay },
                            { "stop", speakerStop },
                            { "get_playing_info", speakerInfo },
                            { nullptr, nullptr } };
}  // namespace LuaSpeaker
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::LuaSpeaker::module, HubPlatform::LuaSpeaker::moduleName );
