#include "scenes_manager/scenes_processor/anonymous_plugin_operations.h"

#include "scenes_manager/scenes_processor/resources.h"
#include "scenes_manager/scenes_engine/scene_manager.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"
#include "cored/model/resources/cored_resources.h"
#include "extensions_manager/common/anonymous_plugin_utils.h"
#include "extensions_manager/resources/em_resources.h"

namespace HubPlatform {
namespace ScenesManager {

AnonymousPluginOperations::AnonymousPluginOperations( IDbClient& client, SceneEngine::SceneManager& sceneManager )
: m_dbClient( client )
, m_manager( sceneManager ) {}

void AnonymousPluginOperations::setPluginCountForScene( const std::string& pluginId,
                                                        int count,
                                                        const std::string& sceneId ) {
    using namespace HubPlatform::ExtensionsManager;
    using namespace HubPlatform::ExtensionsManager::Resources::AnonymousPlugin;

    const auto resultJson =
        m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );

    JsonObject references;
    json_get_obj( resultJson, Fields::References, references );
    JsonObject scenesReferences;
    json_get_obj( references, Fields::Scenes, scenesReferences );
    scenesReferences[ sceneId ] = count;
    JsonObject request;
    request[ Fields::References ][ Fields::Scenes ] = scenesReferences;
    m_dbClient.update(
        Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ), JSON( "$set" << request ) );
}

void AnonymousPluginOperations::resetPluginAcquiredInfoForScene( const std::string& sceneId ) {
    using namespace HubPlatform::ExtensionsManager;
    using namespace HubPlatform::ExtensionsManager::Resources::AnonymousPlugin;
    auto plugins = m_dbClient.findAll( Resources::DBTableNames::AnonymousPlugins, JSON_EMPTY_OBJ, nullptr );

    for( auto& plugin : plugins ) {
        const std::string pluginId = json_get_str( plugin, db::ID_FIELD_NAME );
        JsonObject references;
        json_get_obj( plugin, Fields::References, references );
        JsonObject scenesReferences;
        json_get_obj( references, Fields::Scenes, scenesReferences );
        int countForScene;
        if( json_get( scenesReferences, sceneId, countForScene ) ) {
            scenesReferences.erase( sceneId );
            references[ Fields::Scenes ] = scenesReferences;
            int count;
            json_get( plugin, Fields::Acquired, count );
            JsonObject request = JSON( Fields::References << references << Fields::Acquired << count - countForScene );
            m_dbClient.update( Resources::DBTableNames::AnonymousPlugins,
                               JSON( db::ID_FIELD_NAME << pluginId ),
                               JSON( "$set" << request ) );
        }
    }
}

void AnonymousPluginOperations::updatePluginAcquiredCountForScene( const std::string& pluginId,
                                                                   int updateCount,
                                                                   const std::string& sceneId ) {
    using namespace HubPlatform::ExtensionsManager;
    using namespace HubPlatform::ExtensionsManager::Resources::AnonymousPlugin;
    int count = 0;
    const auto resultJson =
        m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );

    if( !resultJson.empty() ) {
        json_get( resultJson, Fields::Acquired, count );
    }

    auto updateJson = JSON_EMPTY_OBJ;
    updateJson[ Fields::Acquired ] = count + updateCount;
    m_dbClient.update( Resources::DBTableNames::AnonymousPlugins,
                       JSON( db::ID_FIELD_NAME << pluginId ),
                       JSON( "$set" << updateJson ) );

    const auto resultJson1 =
        m_dbClient.findOne( Resources::DBTableNames::AnonymousPlugins, JSON( db::ID_FIELD_NAME << pluginId ), nullptr );
    Log::info( cc::info( "An anonymous plugin has been acquired, pluginId: " ) + cc::note( pluginId ) );

    setPluginCountForScene( pluginId, updateCount, sceneId );
}

void AnonymousPluginOperations::resetAllPluginAcquiredInfo() {
    using namespace HubPlatform::ExtensionsManager;
    using namespace HubPlatform::ExtensionsManager::Resources::AnonymousPlugin;
    auto plugins = m_dbClient.findAll( Resources::DBTableNames::AnonymousPlugins, JSON_EMPTY_OBJ, nullptr );
    for( auto& plugin : plugins ) {
        const std::string pluginId = json_get_str( plugin, db::ID_FIELD_NAME );
        m_dbClient.update( Resources::DBTableNames::AnonymousPlugins,
                           JSON( db::ID_FIELD_NAME << pluginId ),
                           JSON( "$set" << JSON( Fields::Acquired << 0 << Fields::References
                                                                  << JSON( Fields::Scenes << JSON_EMPTY_OBJ ) ) ) );
    }
}

PluginAcquiredInfo AnonymousPluginOperations::getAnonymousPluginCountInScene( std::string& sceneId,
                                                                              SceneEngine::ActionBlocks& actions ) {
    using namespace SceneEngine;
    using namespace ExtensionsManager;
    PluginAcquiredInfo res;

    actions.forEach( [&res]( ActionSharedPtr actionPtr ) {
        auto action = actionPtr.get();
        const auto& name = action->getName();
        if( name == ActionFields::ActionNames::RunCustomScript ) {
            const auto& args = action->getArgs();
            const auto pluginId = json_get_str( args, "script" );
            if( !pluginId.empty() ) {
                if( res.find( pluginId ) != res.end() ) {
                    int count = res.find( pluginId )->second;
                    count++;
                    res[ pluginId ] = count;
                }
                else {
                    res[ pluginId ] = 1;
                }
            }
        }
    } );

    return res;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
