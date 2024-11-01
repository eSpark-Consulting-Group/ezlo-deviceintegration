#include "extensions_manager/common/anonymous_plugin_utils.h"
#include "extensions_manager/resources/em_resources.h"

#include "common_utils/cpp14.h"
#include "jsonrpc/jsonrpc.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbid.h"
#include "database_client/dbtools.h"

#include <cstring>
#include <sstream>

namespace HubPlatform {
namespace ExtensionsManager {

using namespace Resources::AnonymousPlugin;

AnonymousPlugin::AnonymousPlugin()
: m_plugin( JSON_EMPTY_OBJ ) {}

AnonymousPlugin::AnonymousPlugin( const Json& plugin )
: m_plugin( plugin ) {}

bool AnonymousPlugin::operator!() const {
    return m_plugin.empty();
}

Json AnonymousPlugin::toJson() const {
    return m_plugin;
}

AnonymousPluginError::AnonymousPluginError()
: m_errorCode( Errors::OK ) {}

AnonymousPluginError::AnonymousPluginError( Errors::Error errorCode )
: m_errorCode( errorCode ) {}

AnonymousPluginError::AnonymousPluginError( const Json& errorJson ) {
    m_errorCode = static_cast< Errors::Error >( json_get_int( errorJson, "error" ) );
}

AnonymousPluginError::operator bool() const {
    return m_errorCode == 0;
}

bool AnonymousPluginError::operator!() const {
    return m_errorCode != 0;
}

Errors::Error AnonymousPluginError::getError() const {
    return m_errorCode;
}

Json AnonymousPluginError::toJson() const {
    return JSON( "error" << m_errorCode );
}

IAnonymousPluginUtils::~IAnonymousPluginUtils() {}

AnonymousPluginUtils::AnonymousPluginUtils( Services::Service& service )
: m_service( service )
, m_dbClient( m_service.getDBClient() ) {}

AnonymousPluginUtils::PluginId AnonymousPluginUtils::addPlugin( const std::string& scriptCode,
                                                                const std::string& scriptName ) {
    if( !checkPluginCode( scriptCode ) || !checkPluginName( scriptName ) ) {
        return "";
    }

    const auto pluginId = db::DbId::genOID();

    m_dbClient.insert( Resources::DBTableNames::AnonymousPlugins,
                       JSON( db::ID_FIELD_NAME << pluginId << Fields::Name << scriptName << Fields::Code << scriptCode
                                               << Fields::Acquired << 0 << Fields::References
                                               << JSON( Fields::Scenes << JSON_EMPTY_OBJ ) ) );
    // TODO: do not modify table of lua addon
    m_dbClient.create( getPluginDBName( pluginId ) );

    Log::info( cc::info( "An anonymous plugin has been added, pluginId: " ) + cc::note( pluginId ) );

    return pluginId;
}

AnonymousPluginError AnonymousPluginUtils::changePlugin( const std::string& pluginId,
                                                         const std::string& scriptCode,
                                                         const std::string& scriptName ) {
    auto pluginJson = JSON_EMPTY_OBJ;

    if( !pluginExists( pluginId ) ) {
        return AnonymousPluginError( Errors::ANONYMOUS_PLUGIN_DOES_NOT_EXIST );
    }

    /* code and name parameters are optional for a change/set method */
    if( ( !scriptCode.empty() && !checkPluginCode( scriptCode ) ) ||
        ( !scriptName.empty() && !checkPluginName( scriptName ) ) || ( scriptCode.empty() && scriptName.empty() ) ) {
        return AnonymousPluginError( Errors::BAD_PARAMS );
    }

    if( !scriptCode.empty() ) {
        pluginJson[ Fields::Code ] = scriptCode;
    }

    if( !scriptName.empty() ) {
        pluginJson[ Fields::Name ] = scriptName;
    }

    m_dbClient.update( Resources::DBTableNames::AnonymousPlugins,
                       JSON( db::ID_FIELD_NAME << pluginId ),
                       JSON( "$set" << pluginJson ) );

    Log::info( cc::info( "An anonymous plugin has been changed, pluginId: " ) + cc::note( pluginId ) );

    return AnonymousPluginError();
}

AnonymousPluginUtils::AnonymousPlugins AnonymousPluginUtils::listPlugins() const {
    AnonymousPlugins plugins;

    const JsonObject returnFields = JSON( db::ID_FIELD_NAME << db::ID_FIELD_NAME << Fields::Name << Fields::Name );
    const auto result = m_dbClient.findAll( Resources::DBTableNames::AnonymousPlugins, JSON_EMPTY_OBJ, &returnFields );

    for( const auto& elm : result ) {
        plugins.push_back( AnonymousPlugin( elm ) );
    }

    return plugins;
}

AnonymousPluginError AnonymousPluginUtils::deletePlugin( const std::string& pluginId ) {
    if( !pluginExists( pluginId ) ) {
        return AnonymousPluginError( Errors::ANONYMOUS_PLUGIN_DOES_NOT_EXIST );
    }

    if( isPluginAcquired( pluginId ) ) {
        return AnonymousPluginError( Errors::ANONYMOUS_PLUGIN_ACQUIRED );
    }

    removePlugin( pluginId );

    Log::info( cc::info( "An anonymous plugin has been deleted, pluginId: " ) + cc::note( pluginId ) );

    return AnonymousPluginError();
}

AnonymousPluginError AnonymousPluginUtils::runPlugin( const std::string& pluginId ) {
    if( !pluginExists( pluginId ) ) {
        return AnonymousPluginError( Errors::ANONYMOUS_PLUGIN_DOES_NOT_EXIST );
    }

    Log::info( cc::info( "Running anonymous plugin script: " ) + cc::note( pluginId ) );

    const auto scriptId = getScriptIdByPluginId( pluginId );

    auto data = JSON_EMPTY_OBJ;
    data[ "script" ] = scriptId;
    data[ "operationId" ] = "";
    data[ "isUserRunScript" ] = true;

    const auto result = m_service.sendRequest( "addon.lua", data, EXECUTE_SCRIPT );
    if( hasError( result ) ) {
        return AnonymousPluginError( Errors::LUA_SCRIPT_ERROR );
    }

    return AnonymousPluginError();
}

bool AnonymousPluginUtils::checkPluginCode( const std::string& scriptCode ) const {
    return scriptCode.size() < AnonymousPluginContentMaxSize && !scriptCode.empty();
}

bool AnonymousPluginUtils::checkPluginName( const std::string& scriptName ) const {
    return scriptName.size() < AnonymousPluginNameMaxSize;
}

std::string AnonymousPluginUtils::getPluginScriptContent( const std::string& pluginId ) const {
    const auto queryJson = JSON( db::ID_FIELD_NAME << pluginId );
    const auto resultJson = m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, queryJson, nullptr );

    return json_get_str( resultJson, Fields::Code );
}

AnonymousPlugin AnonymousPluginUtils::getPlugin( const std::string& pluginId ) const {
    const auto queryJson = JSON( db::ID_FIELD_NAME << pluginId );
    auto plugin = m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, queryJson, nullptr );
    plugin.erase( Fields::References );
    return AnonymousPlugin( plugin );
}

AnonymousPluginUtils::ScriptId AnonymousPluginUtils::getScriptIdByPluginId( const std::string& pluginId ) const {
    return AnonymousPluginScriptsPrefix + pluginId;
}

AnonymousPluginUtils::PluginId AnonymousPluginUtils::getPluginIdByScriptId( const std::string& scriptId ) const {
    const auto anonymousPrefixPos = scriptId.find( AnonymousPluginScriptsPrefix );
    if( anonymousPrefixPos != std::string::npos ) {
        return scriptId.substr( anonymousPrefixPos + std::strlen( AnonymousPluginScriptsPrefix ) );
    }

    return "";
}

bool AnonymousPluginUtils::isPluginScript( const std::string& scriptId ) const {
    return scriptId.find( AnonymousPluginScriptsPrefix ) == 0U;
}

AnonymousPluginUtils::ScriptStreamUPtr AnonymousPluginUtils::getLuaScript( const std::string& scriptId ) const {
    const auto pluginId = getPluginIdByScriptId( scriptId );
    auto pluginContent = getPluginScriptContent( pluginId );

    return std::make_unique< std::stringstream >( std::move( pluginContent ), std::ios_base::in );
}

std::string AnonymousPluginUtils::getLuaScriptFilePath( const std::string& scriptId ) const {
    return scriptId;
}

std::string AnonymousPluginUtils::getDataFilePath( const std::string& scriptId ) const {
    return scriptId;
}

bool AnonymousPluginUtils::pluginExists( const std::string& pluginId ) const {
    return m_dbClient.has( Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ) );
}

bool AnonymousPluginUtils::isPluginAcquired( const std::string& pluginId ) {
    const auto resultJson =
        m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );
    if( resultJson.empty() || json_get_int( resultJson, Fields::Acquired ) == 0 ) {
        return false;
    }

    return true;
}

AnonymousPluginError AnonymousPluginUtils::acquirePlugin( const std::string& pluginId, bool shouldIncrementCount ) {
    auto updateJson = JSON_EMPTY_OBJ;

    const auto queryJson = JSON( db::ID_FIELD_NAME << pluginId );
    const auto resultJson = m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, queryJson, nullptr );

    int count = 0;
    if( !json_get( resultJson, Fields::Acquired, count ) ) {
        Log::error( cc::error( "An anonymous plugin pluginId: " ) + cc::note( pluginId ) +
                    cc::error( "acquired field not found" ) );
        Log::error( cc::error( "An anonymous plugin can't acquired, pluginId:" ) + cc::error( pluginId ) );
        return AnonymousPluginError( Errors::INTERNAL_OPERATION_ERROR );
    }

    updateJson[ Fields::Acquired ] = shouldIncrementCount ? count + 1 : count;

    if( !pluginExists( pluginId ) ) {
        return AnonymousPluginError( Errors::ANONYMOUS_PLUGIN_DOES_NOT_EXIST );
    }

    m_dbClient.update( Resources::DBTableNames::AnonymousPlugins,
                       JSON( db::ID_FIELD_NAME << pluginId ),
                       JSON( "$set" << updateJson ) );

    Log::info( cc::info( "An anonymous plugin has been acquired, pluginId: " ) + cc::note( pluginId ) );

    return AnonymousPluginError();
}

void AnonymousPluginUtils::releasePlugin( const std::string& pluginId ) {
    if( !pluginExists( pluginId ) ) {
        Log::warn( cc::warn( "attempt to release an inexistent anonymous plugin: " ) + cc::note( pluginId ) );
        return;
    }

    const auto queryJson = JSON( db::ID_FIELD_NAME << pluginId );
    const auto resultJson = m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, queryJson, nullptr );

    int count;
    json_get( resultJson, Fields::Acquired, count );

    JsonObject updateJson;

    if( count == 0 ) {
        Log::error( cc::error( "An anonymous plugin can't release, pluginId: " ) + cc::note( pluginId ) +
                    cc::error( " acquired count is already 0" ) );
        return;
    }

    updateJson[ Fields::Acquired ] = count - 1;

    m_dbClient.update( Resources::DBTableNames::AnonymousPlugins,
                       JSON( db::ID_FIELD_NAME << pluginId ),
                       JSON( "$set" << updateJson ) );

    Log::info( cc::info( "An anonymous plugin has been released, pluginId: " ) + cc::note( pluginId ) );
}

void AnonymousPluginUtils::removePlugin( const std::string& pluginId ) {
    // TODO: do not modify table of lua addon
    m_dbClient.drop( getPluginDBName( pluginId ) );
    m_dbClient.remove( Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ) );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
