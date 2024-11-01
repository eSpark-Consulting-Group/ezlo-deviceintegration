#include "extensions_manager/remote_interface/em_remote_interface.h"
#include "extensions_manager/remote_interface/plugin_extension_data_extractor.h"
#include "extensions_manager/resources/em_resources.h"
#include "extensions_manager/exceptions.h"
#include "extensions_manager/plugin_model/plugins_metedata_validator.h"
#include "extensions_manager/plugin_model/model_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "service_interfaces/service.h"

#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "common_utils/cpp14.h"
#include "jsonrpc/jsonrpc.h"
#include <sys/stat.h>

#include <unordered_set>
#include <cassert>
#include <utility>

namespace HubPlatform {
namespace ExtensionsManager {

namespace Limits {
constexpr const auto CustomPluginServiceCommandTimeout = std::chrono::seconds( 60 );
}

using namespace ::ExtensionsManager::Resources::ConfigFields;

RemoteInterface::RemoteAnonymousPlugin::RemoteAnonymousPlugin( RemoteInterface& remoteInterface, std::string pluginId )
: m_remoteInterface( remoteInterface )
, m_pluginId( std::move( pluginId ) ) {}

RemoteInterface::RemoteAnonymousPlugin::~RemoteAnonymousPlugin() {
    // NOTE: Don't delete a plugin from db if there is no clients in memory
}

RemoteInterface::RemoteInterface( Services::Service& hostService )
: RemoteInterface( hostService,
                   std::make_unique< PluginUtils >(),
                   std::make_unique< AnonymousPluginUtils >( hostService ) ) {}

RemoteInterface::RemoteInterface( Services::Service& hostService,
                                  PluginUtilsUPtr pluginUtils,
                                  AnonymousPluginUtilsUPtr anonymousPluginUtils )
: m_hostService( hostService )
, m_pluginUtils( std::move( pluginUtils ) )
, m_anonymousPluginUtils( std::move( anonymousPluginUtils ) ) {
    assert( m_pluginUtils );
}

JsonObject RemoteInterface::requestPluginRun( const std::string& scriptName,
                                              const Json& scriptParams,
                                              const std::string& operationId ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "script" ] = scriptName;
    params[ "scriptParams" ] = scriptParams;
    params[ "operationId" ] = operationId;
    params[ "isUserRunScript" ] = true;

    return m_hostService.sendMethodCall( GroupsNames::EXTENSIONS_MANAGER, PLUGIN_RUN_REQUEST, params );
}

JsonObject RemoteInterface::registerCustomPlugin( const std::string& componentId ) {
    return m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER,
                                      JSON( "id" << componentId ),
                                      CUSTOM_PLUGIN_REGISTER,
                                      Limits::CustomPluginServiceCommandTimeout );
}

JsonObject RemoteInterface::unregisterCustomPlugin( const std::string& componentId ) {
    return m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER,
                                      JSON( "id" << componentId ),
                                      CUSTOM_PLUGIN_UNREGISTER,
                                      Limits::CustomPluginServiceCommandTimeout );
}

JsonObject RemoteInterface::pauseCustomPlugin( const std::string& componentId ) {
    return m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER,
                                      JSON( "id" << componentId ),
                                      CUSTOM_PLUGIN_PAUSE,
                                      Limits::CustomPluginServiceCommandTimeout );
}

Json RemoteInterface::requestConfigList() {
    auto result = JSON( "configs" << JSON_EMPTY_ARR );

    // Collect plugins with configs
    auto registeredPlugins = m_pluginUtils->getInstalledPlugins();
    for( const auto& pluginId : registeredPlugins ) {
        auto extendedPlugin = m_hostService.getDBClient().findOne(
            Resources::DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );

        auto extensionsIt = extendedPlugin.find( Resources::PluginRecord::Extensions );
        if( extensionsIt != extendedPlugin.end() && extendedPlugin[ Resources::PluginRecord::Extensions ].count(
                                                        Resources::PluginRecord::ExtensionsRecord::Configs ) ) {
            auto configIt = extensionsIt->find( Resources::PluginRecord::ExtensionsRecord::Configs );
            if( configIt != extensionsIt->end() ) {
                result[ "configs" ].push_back( JSON( "plugin" << pluginId << "configs" << *configIt ) );
            }
        }
    }

    // Collect configs extensions
    for( auto& extendedPluginInfo : result[ "configs" ] ) {
        auto extensionPluginsIds = getPluginExtensionsIds( extendedPluginInfo[ "plugin" ] );

        for( auto& extendedPluginConfig : extendedPluginInfo[ "configs" ] ) {
            extendedPluginConfig[ "extensions" ] = JSON_EMPTY_ARR;

            for( const auto& extensionPluginId : extensionPluginsIds ) {
                auto extensionPlugin = m_hostService.getDBClient().findOne(
                    Resources::DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << extensionPluginId ), nullptr );

                auto extensionsIt = extensionPlugin.find( Resources::PluginRecord::Extensions );
                if( extensionsIt != extensionPlugin.end() ) {
                    auto pluginsIt = extensionsIt->find( Resources::PluginRecord::ExtensionsRecord::Plugins );
                    if( pluginsIt != extensionsIt->end() ) {
                        for( const auto& pluginExtensionInfo : *pluginsIt ) {
                            if( pluginExtensionInfo[ Resources::PluginRecord::ExtensionsRecord::Id ] ==
                                extendedPluginInfo[ "plugin" ] ) {
                                for( const auto& extensionconfig :
                                     pluginExtensionInfo[ Resources::PluginRecord::ExtensionsRecord::Configs ] ) {
                                    if( extensionconfig[ Resources::PluginRecord::ExtensionsRecord::Name ] ==
                                        extendedPluginConfig[ "name" ] ) {
                                        extendedPluginConfig[ "extensions" ].push_back( extensionPluginId );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

Json RemoteInterface::requestConfigPrioritySet( const std::string& pluginId,
                                                const std::vector< std::string >& priorities ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "pluginId" ] = pluginId;
    params[ "priorities" ] = priorities;
    const auto reply = m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, PLUGIN_CONFIG_PRIORITY_SET, params );
    return reply[ "list" ];
}

RemoteInterface::AnonymousPluginId RemoteInterface::addAnonymousPlugin( const std::string& scriptCode,
                                                                        const std::string& scriptName ) {
    using namespace Resources::AnonymousPlugin;

    auto params = JSON_EMPTY_OBJ;
    params[ Fields::Code ] = scriptCode;
    params[ Fields::Name ] = scriptName;

    const auto reply = m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, ANONYMOUS_PLUGIN_ADD, params );
    return reply[ Fields::Id ];
}

AnonymousPluginError RemoteInterface::changeAnonymousPlugin( const std::string& pluginId,
                                                             const std::string& scriptCode,
                                                             const std::string& scriptName ) {
    using namespace Resources::AnonymousPlugin;

    auto params = JSON_EMPTY_OBJ;
    params[ Fields::Id ] = pluginId;
    params[ Fields::Code ] = scriptCode;
    params[ Fields::Name ] = scriptName;

    const auto reply = m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, ANONYMOUS_PLUGIN_CHANGE, params );
    return AnonymousPluginError( reply );
}

AnonymousPlugin RemoteInterface::getAnonymousPluginContent( const std::string& pluginId ) {
    return m_anonymousPluginUtils->getPlugin( pluginId );
}

AnonymousPluginError RemoteInterface::deleteAnonymousPlugin( const std::string& pluginId ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::AnonymousPlugin::Fields::Id ] = pluginId;

    const auto reply = m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, ANONYMOUS_PLUGIN_DELETE, params );
    return AnonymousPluginError( reply );
}

RemoteInterface::AnonymousPlugins RemoteInterface::listAnonymousPlugins() {
    return m_anonymousPluginUtils->listPlugins();
}

AnonymousPluginError RemoteInterface::runAnonymousPlugin( const std::string& pluginId ) {
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::AnonymousPlugin::Fields::Id ] = pluginId;

    const auto reply = m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, ANONYMOUS_PLUGIN_RUN, params );
    return AnonymousPluginError( reply );
}

bool RemoteInterface::isAnonymousPluginAcquired( const std::string& pluginId ) {
    return m_anonymousPluginUtils->isPluginAcquired( pluginId );
}

RemoteInterface::SharedRemoteAnonymousPlugin RemoteInterface::acquireAnonymousPlugin( const std::string& pluginId ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    auto it = m_remoteAnonymousPluginsMap.find( pluginId );
    if( it != m_remoteAnonymousPluginsMap.end() ) {
        auto ptr = it->second.lock();
        if( ptr.get() )
            return ptr;
        m_remoteAnonymousPluginsMap.erase( it );
    }

    // this is a part of an acquire operation (which should be atomic), so no release a lock
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::AnonymousPlugin::Fields::Id ] = pluginId;
    AnonymousPluginError reply(
        m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, ANONYMOUS_PLUGIN_ACQUIRE, params ) );
    if( !reply ) {
        Log::error( cc::error( "failed to acquire a plugin " ) + cc::note( pluginId ) + cc::error( ", " ) +
                    cc::note( std::to_string( reply.getError() ) ) );
        return nullptr;
    }

    auto remoteAnonymousPlugin =
        std::shared_ptr< RemoteAnonymousPlugin >( new RemoteAnonymousPlugin( *this, pluginId ) );
    m_remoteAnonymousPluginsMap.insert(
        std::make_pair( pluginId, std::weak_ptr< RemoteAnonymousPlugin >( remoteAnonymousPlugin ) ) );

    return remoteAnonymousPlugin;
}

void RemoteInterface::releaseAnonymousPlugin( const std::string& pluginId ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    // this is a part of a release operation (which should be atomic), so no release a lock
    auto params = JSON_EMPTY_OBJ;
    params[ Resources::AnonymousPlugin::Fields::Id ] = pluginId;
    m_hostService.sendRequest( GroupsNames::EXTENSIONS_MANAGER, ANONYMOUS_PLUGIN_RELEASE, params );

    m_remoteAnonymousPluginsMap.erase( pluginId );
}

std::string RemoteInterface::getAnonymousScriptIdByPluginId( const std::string& pluginId ) const {
    return m_anonymousPluginUtils->getScriptIdByPluginId( pluginId );
}

bool RemoteInterface::isPluginScript( const std::string& scriptId ) const {
    return scriptId.find( Resources::PluginScriptsPrefix ) == 0U;
}

bool RemoteInterface::isAnonymousPluginScript( const std::string& scriptId ) const {
    return m_anonymousPluginUtils->isPluginScript( scriptId );
}

PluginUtils::StreamUPtr RemoteInterface::getLuaScript( const std::string& scriptId ) {
    if( isAnonymousPluginScript( scriptId ) ) {
        return m_anonymousPluginUtils->getLuaScript( scriptId );
    }
    else if( isPluginScript( scriptId ) ) {
        return m_pluginUtils->getInternalPluginLuaScriptFile( scriptId );
    }

    return nullptr;
}

std::string RemoteInterface::getPluginIdByScriptId( const std::string& scriptId ) const {
    if( isAnonymousPluginScript( scriptId ) ) {
        return m_anonymousPluginUtils->getPluginIdByScriptId( scriptId );
    }
    else if( isPluginScript( scriptId ) ) {
        return m_pluginUtils->extractPluginId( scriptId );
    }

    Log::error( cc::error( "Failed to get a plugin id by a script id: " + cc::note( scriptId ) ) );

    return "";
}

PluginUtils::SplittedFileId RemoteInterface::splitFileId( const std::string& fileId ) const {
    return m_pluginUtils->splitScriptId( fileId );
}

std::string RemoteInterface::getLuaScriptFilePath( const std::string& scriptId ) const {
    if( isAnonymousPluginScript( scriptId ) ) {
        return m_anonymousPluginUtils->getLuaScriptFilePath( scriptId );
    }
    else if( isPluginScript( scriptId ) ) {
        return m_pluginUtils->getLuaScriptFilePath( scriptId );
    }

    Log::error( cc::error( "Failed to get a lua script path: " + cc::note( scriptId ) ) );

    return "";
}

std::string RemoteInterface::getDataFilePath( const std::string& scriptId ) const {
    if( isAnonymousPluginScript( scriptId ) ) {
        return m_anonymousPluginUtils->getDataFilePath( scriptId );
    }
    else if( isPluginScript( scriptId ) ) {
        return m_pluginUtils->getFilePathById( scriptId );
    }

    Log::error( cc::error( "Failed to get a data file path: " + cc::note( scriptId ) ) );

    return "";
}

ExecutionPolicy RemoteInterface::getExecutionPolicyByPluginId( const std::string& pluginId ) const {
    const auto resultJson = m_hostService.getDBClient().findOne(
        Resources::DBTableNames::Plugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );

    ExecutionPolicy resultPolicy = ExecutionPolicy::NewStateEachScriptRun;
    if( !resultJson.empty() ) {
        std::string executionPolicy;
        if( json_get( resultJson, Resources::PluginRecord::ExecutionPolicy, executionPolicy ) ) {
            resultPolicy = toExecutionPolicy( executionPolicy );
            if( resultPolicy == ExecutionPolicy::Unknown ) {
                resultPolicy = ExecutionPolicy::NewStateEachScriptRun;
            }
        }
    }

    return resultPolicy;
}

bool RemoteInterface::checkPluginPermission( const std::string& pluginId, const std::string& permission ) const {
    using namespace Resources::PluginRecord;

    const auto queryJson =
        JSON( db::ID_FIELD_NAME << pluginId << Permissions << JSON( "$in" << JSON_ARRAY_( permission ) ) );
    const auto resultJson = m_hostService.getDBClient().findOne( Resources::DBTableNames::Plugins, queryJson, nullptr );

    return !resultJson.empty();
}

std::string RemoteInterface::getTemplateForDevice( const std::string& templateName ) {
    auto pluginStream = m_pluginUtils->getInternalPluginTemplate( templateName );
    if( !pluginStream )
        throw TemplateNotFoundError{ templateName };

    return std::string( std::istreambuf_iterator< char >( *pluginStream ), {} );
}

PluginUtils::StreamUPtr RemoteInterface::getPluginFile( const std::string& fileId ) const {
    auto stream = m_pluginUtils->getInternalPluginFileById( fileId );
    if( !stream ) {
        throw FileNotFoundError{ fileId };
    }
    return stream;
}

Json RemoteInterface::getPluginExtensionsIds( const std::string& pluginId ) {
    JsonArray result;

    auto extensions = m_hostService.getDBClient().findOne(
        Resources::DBTableNames::PluginsExtensionsPriorities, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );

    auto extensionsListIt = extensions.find( "extensions" );

    if( extensionsListIt != extensions.end() && extensionsListIt->is_array() && !extensionsListIt->empty() ) {
        for( const auto& extensionPluginId : *extensionsListIt ) {
            if( extensionPluginId.is_string() ) {
                bool isRegistered = m_hostService.getDBClient().has( Resources::DBTableNames::Plugins,
                                                                     JSON( db::ID_FIELD_NAME << extensionPluginId ) );
                if( isRegistered ) {
                    result.push_back( extensionPluginId );
                }
            }
        }
    }

    return result;
}

Json RemoteInterface::getPluginExtensionData( const std::string& pluginId,
                                              const std::string& path,
                                              const std::string& filter ) {
    PluginExtensionDataExtractor extractor( m_hostService.getDBClient(), *m_pluginUtils );
    return extractor.extractData( pluginId, path, filter );
}

std::vector< JsonObject > RemoteInterface::fetchPluginsMetadata( const std::string& pluginFullName ) {
    JsonObject query = JSON_EMPTY_OBJ;
    if( !pluginFullName.empty() ) {
        query = JSON( DbFields::Plugin << pluginFullName );
    }

    return m_hostService.getDBClient().findAll( HaExtensionsManagerCustomNodesDBName, query, nullptr, 0, 0, false );
}

bool RemoteInterface::checkId( const JsonObject& meta ) {
    if( meta.find( db::ID_FIELD_NAME ) == meta.cend() ) {
        return false;
    }

    auto id = json_get_str( meta[ db::ID_FIELD_NAME ] );
    m_hostService.getDBClient().findAll(
        HaExtensionsManagerCustomNodesDBName, JSON( db::ID_FIELD_NAME << id ), nullptr, 0, 0, false );
    return m_hostService.getDBClient()
        .findAll( HaExtensionsManagerCustomNodesDBName, JSON( db::ID_FIELD_NAME << id ), nullptr, 0, 0, false )
        .empty();
}

std::vector< JsonObject > RemoteInterface::fetchPluginsMetadataWithQuery( const JsonObject& query ) {
    return m_hostService.getDBClient().findAll( HaExtensionsManagerCustomNodesDBName, query, nullptr, 0, 0, false );
}

bool RemoteInterface::isPluginData( const std::string& fullName ) {
    const auto data = fetchPluginsMetadata( fullName );
    return !data.empty();
}

void RemoteInterface::validatePluginData( std::istream& config,
                                          std::istream& interface,
                                          std::function< bool( std::string ) >& ifExist ) {
    ::ExtensionsManager::PluginsMetadataValidator validator;
    validator.parse( config, interface, ifExist );
}

std::string RemoteInterface::setPluginData( std::istream& config,
                                            std::istream& interface,
                                            std::function< bool( std::string ) >& ifExist ) {
    ::ExtensionsManager::PluginsMetadataValidator validator;

    std::string pluginFullName;
    auto dbData = validator.parse( config, interface, ifExist );
    if( checkId( dbData ) ) {
        pluginFullName = json_get_str( dbData[ DbFields::Plugin ] );
        const auto& findObj = fetchPluginsMetadata( pluginFullName );
        if( findObj.empty() ) {
            saveToDb( dbData );
        }
        else {
            removeFromDB( findObj.front() );
            saveToDb( dbData );
        }
    }

    return pluginFullName;
}

bool RemoteInterface::removePluginData( const std::string& fullName ) {
    const auto& findObj = fetchPluginsMetadata( fullName );
    if( !findObj.empty() ) {
        removeFromDB( findObj.front() );
        return true;
    }
    return false;
}

void RemoteInterface::removeFromDB( const JsonObject& obj ) {
    m_hostService.getDBClient().remove( HaExtensionsManagerCustomNodesDBName, obj );
}

void RemoteInterface::saveToDb( const JsonObject& obj ) {
    m_hostService.getDBClient().insert( HaExtensionsManagerCustomNodesDBName, obj );
}

}  // namespace ExtensionsManager
}  // namespace HubPlatform
