#pragma once

#include "scene_context_info.h"

namespace SceneEngine {
class SceneContextManager;
using SceneContextManagerPtr = std::shared_ptr< SceneContextManager >;

class SceneContextManager {
public:
    SceneContextManager( IDbClient& );
    std::string addActonInfoToRun( const std::string& contextId,
                                   const std::string& sceneId,
                                   const std::string& blockName,
                                   int actionId );
    void removeActonFromRun( const std::string& contextId, int actionId );

    void setTask( const std::string& contextId, int actionId, HubPlatform::Services::PostponedTaskPtr& task );
    void setPosition( const std::string& contextId, int actionId, int position );
    void setTimestamp( const std::string& contextId, int actionId, int64_t timestamp );
    void setActionType( const std::string& contextId, int actionId, const RunSceneActionBlockType blockType );

    void removeContext( const std::string& contextId );

    const std::string createNewExecutionContext( const std::string& sceneId,
                                                 int64_t timestamp,
                                                 const JsonObject& message );

    bool stopActions( const std::string& sceneId,
                      const RunSceneActionBlockType blockType,
                      std::function< void() > sendBroadcast );

    JsonObject getMessage( const std::string& contextId );

    void setSceneId( const std::string& contextId, const std::string& sceneId );
    std::string getSceneId( const std::string& contextId );

    bool isStopped( const std::string& contextId, const RunSceneActionBlockType blockType );
    bool checkContext( const std::string& contextId );
    bool isStopWasSent( const std::string& contextId );
    void clearAllContexts();

private:
    std::map< std::string, std::shared_ptr< SceneExecutionContext > > m_sceneExecutionContexts;
    IDbClient& m_dbClient;
};

}  // namespace SceneEngine
