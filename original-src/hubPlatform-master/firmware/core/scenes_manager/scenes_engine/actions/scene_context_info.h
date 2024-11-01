#pragma once

#include "scene_single_context.h"
#include "scenes_manager/scenes_engine/actions/iscene_action_execute.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "cored/remote_interface/cored_interface.h"
#include "service_interfaces/task_loop_timer.h"
#include "service_interfaces/task_loop.h"

namespace SceneEngine {
class SceneContextManager;
using SceneContextManagerPtr = std::shared_ptr< SceneContextManager >;

class SceneExecutionContext {
public:
    SceneExecutionContext( const std::string& id,
                           const std::string& sceneId,
                           int64_t timestamp,
                           IDbClient& dbClient,
                           const JsonObject& message );
    void remove();
    void setSceneId( const std::string& setSceneId );
    std::string getSceneId() const;
    int64_t getTimestamp() const;
    void addActionInfo( SingleSceneContextPtr actionInfo );
    void removeActionInfo( int actionId );
    SingleSceneContextPtr getActionInfo( const std::string actionId );
    std::map< std::string, SingleSceneContextPtr > getActionInfos() const;
    void createDbItem();
    void updateDbItem();
    void loadFromDb();
    void fromJson( const JsonObject& data );
    JsonObject toJson();

    void stopAllActions( RunSceneActionBlockType block );
    bool isStopped( RunSceneActionBlockType blockType );
    bool isStoppedActions();

    bool isStopWasSent();
    void stopWasSent( bool stopWasSent );
    JsonObject getMessage() const;

private:
    void printDB();

private:
    std::string m_sceneId;
    int64_t m_timestamp;
    std::string m_id;
    std::map< std::string, SingleSceneContextPtr > m_singleActionInfos;
    IDbClient& m_dbClient;
    bool m_stopped = false;
    bool m_stopWasSent = false;
    RunSceneActionBlockType m_stopBlockType = RunSceneActionBlockType::Empty;
    JsonObject m_message;
};

}  // namespace SceneEngine
