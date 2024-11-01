#pragma once
#include <string>

#include "json_utils/json_fwd.h"
#include "scenes_manager/scenes_engine/scene_manager.h"

class IDbClient;

namespace UT {
class ScenesAPIFixture;
}  // namespace UT

namespace HubPlatform {
namespace ScenesManager {

class ScenesCloneManager {
public:
    ScenesCloneManager( IDbClient& client, JsonObject& cloneScene, SceneEngine::SceneManager& sceneManager );
    bool run( const std::string& name );
    void replaceAllReferencesByPlaceholders();
    std::string createClonedName( const std::string& name );
    void mapReplacedReference( const std::string& oldReference, const std::string& newReference );

private:
    IDbClient& m_dbClient;
    JsonObject& m_clonedScene;
    SceneEngine::SceneManager& m_manager;
    std::string m_oldSceneId;
    std::string m_newSceneId;
    std::map< std::string, std::string > m_innerBlockReferences;

private:
    IDbClient& getDBClient();
    const IDbClient& getDBClient() const;
    void replaceAndStorePlaceholdersForConditionBlocks( const std::string& blockType,
                                                        std::map< std::string, std::string >& mapper );
    void replaceIdForBlocks( const std::string& blockType, bool mapReference = false );
    void applyPlaceholdersForActionBlocks( const std::string& blockType, std::map< std::string, std::string >& mapper );
    void handleInnerSceneIdRelationsInActionBlocks( const std::string& blockType );
    static std::string getMethodName( const JsonObject& block );

    void findAndReplaceBlockIdReference( JsonArrayBuilder& clonedFiends, Json& field );
    void findAndReplaceSceneIdReference( JsonArrayBuilder& clonedFiends, Json& field );
};

}  // namespace ScenesManager
}  // namespace HubPlatform
