#include "binding_interfaces/lua/lua_binding_interface.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "scenes_manager/scenes_engine/scene_status_resources.h"

#include "json_utils/json_fwd.h"
#include "log_utils/ezlo_log.h"

#include <string>

#include "resources.h"

#define CHECK_LUA_INPUT_PARAMETERS( number ) CHECK_INPUT_PARAMETERS( ( std::string{ "scenes." } + __func__ ), number )

#define CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "scenes." } + __func__ ), number )

#define GET_LUA_INPUT_PARAMETERS( type, idx ) GET_INPUT_PARAMETERS( type, idx, ( std::string{ "scenes." } + __func__ ) )

namespace HubPlatform {

namespace ExpressionFields {
const char* const Value = "value";
const char* const ValueTypeRPC = "valueType";
const char* const Metadata = "metadata";
const char* const Params = "params";
const char* const Code = "code";

namespace Lua {
const char* const ValueType = "value_type";
}
}  // namespace ExpressionFields

namespace {

/* arguments:  commands ( array of tables )
 *               possible commands:
 *               [
 *                  {
 *                       [ "command" ] = "replace_token",
 *                       [ "to_replace" ] = string,
 *                       [ "replace_by" ] = string,
 *                  }
 *                  {
 *                       [ "command" ] = "replace_language_text",
 *                       [ "language_tag" ] = string,
 *                       [ "replace_by_text" ] = string,
 *                  }
 *              [
 *            block id ( string )
 */
int modifyScenes( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );

    const auto commands = GET_LUA_INPUT_PARAMETERS( Json, 1 );
    auto blockIds = LuaToNative< Json >{}( luaState, 2 );

    ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
    scenesManagerRemoteInterface.modifyScenes( commands, blockIds.valueOr( Json() ) );

    return 0;
}

int getExpressionValue( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );
        const auto name = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto options = LuaToNative< Json >{}( luaState, 2 ).valueOr( Json{} );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );

        const auto json = scenesManagerRemoteInterface.getExpressionValue( name, options );
        if( json.count( Errors::ERROR_FIELD ) == 1 && !json[ Errors::ERROR_FIELD ].empty() ) {
            return luaL_error( luaState, "%s", json_get_str( json[ Errors::ERROR_FIELD ], "message" ).c_str() );
        }

        Json result = json[ "result" ];
        result[ ExpressionFields::Lua::ValueType ] = result[ ExpressionFields::ValueTypeRPC ];
        json_remove( result, ExpressionFields::ValueTypeRPC );

        return pushJsonAsLuaObj( luaState, result );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error get expression value: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error get expression value: " }.append( ex.what() ).c_str() );
    }
}

int getSceneInfo( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );
        const auto name = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto json = scenesManagerRemoteInterface.getScenesInfo( name );
        if( json.count( Errors::ERROR_FIELD ) == 1 && !json_get_str( json, "error" ).empty() ) {
            return luaL_error(
                luaState, "failed to fetch scene information: %s", json_get_str( json, "error" ).c_str() );
        }

        return pushJsonAsLuaObj( luaState, json );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error get expression value: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error get expression value: " }.append( ex.what() ).c_str() );
    }
}

int getWhenBlockState( lua_State* luaState ) {
    namespace SceneStatusFields = SceneEngine::SceneStatusFields;
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );
        const auto sceneId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto blockId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto response = scenesManagerRemoteInterface.getSceneStatus( sceneId );
        JsonObject error;

        if( json_get( response, Errors::ERROR_FIELD, error ) && !error.is_null() ) {
            if( json_get_int( error, "code" ) == static_cast< int >( Errors::Codes::APPLICATION_ERROR ) ) {
                return luaL_error( luaState, "The scene doesn't exist." );
            }
            else {
                return luaL_error(
                    luaState, "failed to fetch scene status: %s", json_get_str( error, "message" ).c_str() );
            }
        }

        Json blockState;
        std::function< bool( const Json& ) > searchBlock =
            [&blockState, &blockId, &searchBlock]( const Json& whenBlock ) -> bool {
            if( blockId == whenBlock[ SceneStatusFields::Id ] ) {
                blockState = whenBlock;
                return true;
            }
            JsonArray nestedBlocks = JSON_EMPTY_ARR;
            json_get_arr( whenBlock, SceneStatusFields::Blocks, nestedBlocks );
            std::find_if( nestedBlocks.begin(), nestedBlocks.end(), searchBlock );
            return !blockState.empty();
        };
        const Json& sceneStatus = response[ SceneStatusFields::Status ];
        const Json& whenBlocks = sceneStatus[ SceneStatusFields::WhenBlocks ];
        std::find_if( whenBlocks.begin(), whenBlocks.end(), searchBlock );
        if( blockState.empty() ) {
            const Json& exceptions = sceneStatus[ SceneStatusFields::Exceptions ];
            std::find_if( exceptions.begin(), exceptions.end(), searchBlock );
            if( blockState.empty() ) {
                const Json& globalRestriction = sceneStatus[ SceneStatusFields::GlobalRestriction ];
                std::find_if( globalRestriction.begin(), globalRestriction.end(), searchBlock );
                if( blockState.empty() ) {
                    return luaL_error( luaState, "The block doesn't exist." );
                }
            }
        }

        Json responseBlock;
        responseBlock[ "method" ] = blockState[ SceneStatusFields::Name ];
        responseBlock[ "last_change" ] = blockState[ SceneStatusFields::LastChange ];
        responseBlock[ "state" ] = blockState[ SceneStatusFields::State ];
        if( json_contains( blockState, SceneStatusFields::BlockName ) ) {
            responseBlock[ "block_name" ] = blockState[ SceneStatusFields::BlockName ];
        }
        if( json_contains( blockState, SceneStatusFields::Function ) ) {
            const Json& function = blockState[ SceneStatusFields::Function ];
            responseBlock[ "function" ][ "name" ] = function[ SceneStatusFields::Name ];
            if( json_contains( function, SceneStatusFields::IsLatched ) ) {
                responseBlock[ "function" ][ "is_latched" ] = function[ SceneStatusFields::IsLatched ];
            }
        }
        responseBlock[ "blocks" ] = JSON_EMPTY_ARR;
        if( json_contains( blockState, SceneStatusFields::Blocks ) ) {
            for( const auto& containedBlock : blockState[ SceneStatusFields::Blocks ] ) {
                responseBlock[ "blocks" ].push_back( containedBlock[ SceneStatusFields::Id ] );
            }
        }
        return pushJsonAsLuaObj( luaState, responseBlock );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error get when block: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error get when block: " }.append( ex.what() ).c_str() );
    }
}

const std::string makeErrorStringFromObject( const JsonObject& errorObject ) {
    std::string res;

    if( errorObject.find( "message" ) != errorObject.end() ) {
        res += json_get_str( errorObject, "message" );
    }

    if( errorObject.find( "reason" ) != errorObject.end() || errorObject.find( "field" ) != errorObject.end() ) {
        auto reason = json_get_str( errorObject, "reason" );
        if( !reason.empty() ) {
            res += ": ";
            res += reason;
        }

        auto field = json_get_str( errorObject, "field" );
        if( !field.empty() ) {
            if( !field.empty() ) {
                res += ": ";
                res += field;
            }
            else {
                res += " ";
                res += field;
            }
        }
    }
    return res;
}

int setExpression( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );
        const auto name = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        auto params = pullJson( luaState, 2 );

        if( params.empty() || params.is_null() ) {
            return luaL_error( luaState, "Required parameter wasn't defined: 'data'" );
        }

        JsonObject itemParam = JSON_EMPTY_OBJ;
        json_get_obj( params, ExpressionFields::Params, itemParam );

        std::string valueType = json_get_str( params, ExpressionFields::Lua::ValueType );

        std::string code = json_get_str( params, ExpressionFields::Code );

        JsonObject metadata = JSON_EMPTY_OBJ;
        json_get( params, ExpressionFields::Metadata, metadata );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto json = scenesManagerRemoteInterface.updateExpression( name, itemParam, code, valueType, metadata );
        JsonObject error;
        if( json.count( Errors::ERROR_FIELD ) == 1 && json_get_obj( json, "error", error ) ) {
            return luaL_error( luaState, "%s", makeErrorStringFromObject( error ).c_str() );
        }

        return pushJsonAsLuaObj( luaState, json );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error set expression value: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error set expression value: " }.append( ex.what() ).c_str() );
    }
}

int setVariable( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );
        const auto name = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        auto params = pullJson( luaState, 2 );

        JsonObject value = JSON_EMPTY_OBJ;
        json_get( params, ExpressionFields::Value, value );

        std::string valueType = json_get_str( params, ExpressionFields::Lua::ValueType );

        JsonObject metadata = JSON_EMPTY_OBJ;
        json_get( params, ExpressionFields::Metadata, metadata );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto json = scenesManagerRemoteInterface.updateExpressionVariable( name, value, valueType, metadata );
        JsonObject error;
        if( json.count( Errors::ERROR_FIELD ) == 1 && json_get_obj( json, "error", error ) ) {
            return luaL_error( luaState, "%s", makeErrorStringFromObject( error ).c_str() );
        }

        return 1;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error set variable value: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error set variable value: " }.append( ex.what() ).c_str() );
    }
}

int setSceneEnabled( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );
        const auto sceneId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        auto enabled = GET_LUA_INPUT_PARAMETERS( NativeBool, 2 );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto json = scenesManagerRemoteInterface.enableScene( sceneId, enabled, false );
        JsonObject error;
        if( json.count( Errors::ERROR_FIELD ) == 1 && json_get_obj( json, "error", error ) ) {
            return luaL_error( luaState, "%s", Resources::LuaBindingsError::getMessageWithErrorCode( error ).c_str() );
        }

        lua_pushnil( luaState );
        return 1;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error set scene enabled: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error set scene enabled: " }.append( ex.what() ).c_str() );
    }
}

int stopScene( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );
        const auto sceneId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto block = LuaToNative< NativeString >()( luaState, 2 ).valueOr( std::string() );
        const auto groupId = LuaToNative< NativeString >()( luaState, 3 ).valueOr( std::string() );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );

        const auto json = scenesManagerRemoteInterface.stopScene( sceneId, false, block, groupId );
        const auto errorResponse = json[ "error" ][ "reason" ].dump();

        if( ( json.count( Errors::ERROR_FIELD ) == 1 ) && !errorResponse.empty() ) {
            return luaL_error( luaState, errorResponse.c_str() );
        }
        return pushJsonAsLuaObj( luaState, json );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error stop scene: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error stop scene: " }.append( ex.what() ).c_str() );
    }
}

int resetLatch( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );
        const auto sceneId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto blockId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto json = scenesManagerRemoteInterface.sceneBlockStatusReset( sceneId, blockId );
        JsonObject error;
        if( json.count( Errors::ERROR_FIELD ) == 1 && json_get_obj( json, "error", error ) ) {
            return luaL_error( luaState, "%s", Resources::LuaBindingsError::getMessageWithErrorCode( error ).c_str() );
        }

        lua_pushnil( luaState );
        return 1;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error reset latch: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error reset latch: " }.append( ex.what() ).c_str() );
    }
}

int resetSceneLatches( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );
        const auto sceneId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

        ScenesManager::RemoteInterface scenesManagerRemoteInterface( getHostService( luaState ) );
        const auto json = scenesManagerRemoteInterface.sceneAllBlocksStatusReset( sceneId );
        JsonObject error;
        if( json.count( Errors::ERROR_FIELD ) == 1 && json_get_obj( json, "error", error ) ) {
            return luaL_error( luaState, "%s", Resources::LuaBindingsError::getMessageWithErrorCode( error ).c_str() );
        }

        lua_pushnil( luaState );
        return 1;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Error reset latches: " ) + cc::note( ex.what() ) );
        return luaL_error( luaState, std::string{ "Error reset latches: " }.append( ex.what() ).c_str() );
    }
}

const char moduleName[] = "scenes";
const luaL_Reg module[] = { { "modify_scenes", modifyScenes },
                            { "get_expression_value", getExpressionValue },
                            { "get_scene_info", getSceneInfo },
                            { "get_scene_when_block_state", getWhenBlockState },
                            { "set_expression", setExpression },
                            { "set_variable", setVariable },
                            { "set_scene_enabled", setSceneEnabled },
                            { "set_scene_enabled", setSceneEnabled },
                            { "stop_scene", stopScene },
                            { "reset_latch", resetLatch },
                            { "reset_scene_latches", resetSceneLatches },
                            { nullptr, nullptr } };
}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
