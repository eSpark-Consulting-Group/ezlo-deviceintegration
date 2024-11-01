#pragma once

#include <string>
#include <map>

class IDbClient;

namespace SceneEngine {
class SceneManager;
class ActionBlocks;
}  // namespace SceneEngine

namespace HubPlatform {

namespace ScenesManager {

using PluginAcquiredInfo = std::map< std::string, int >;

class AnonymousPluginOperations {
public:
    AnonymousPluginOperations( IDbClient& client, SceneEngine::SceneManager& sceneManager );

    ~AnonymousPluginOperations() = default;

    void resetAllPluginAcquiredInfo();
    void resetPluginAcquiredInfoForScene( const std::string& sceneId );
    void updatePluginAcquiredCountForScene( const std::string& pluginId, int updateCount, const std::string& sceneId );
    PluginAcquiredInfo getAnonymousPluginCountInScene( std::string& sceneId, SceneEngine::ActionBlocks& actions );
    void setPluginCountForScene( const std::string& pluginId, int count, const std::string& sceneId );

private:
    IDbClient& m_dbClient;
    SceneEngine::SceneManager& m_manager;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
