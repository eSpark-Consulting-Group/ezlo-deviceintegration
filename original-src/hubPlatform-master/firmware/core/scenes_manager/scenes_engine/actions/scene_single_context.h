#pragma once

#include <string>
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "cored/model/values/values.h"
#include "service_interfaces/service.h"
#include "service_interfaces/service.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "scenes_manager/scenes_engine/actions/action_block.h"

namespace SceneEngine {

class SingleSceneContext;
using SingleSceneContextPtr = std::shared_ptr< SingleSceneContext >;
class SingleSceneContext {
public:
    SingleSceneContext( std::string sceneId, std::string blockName, int actionId );
    Json toJson() const;
    void fromJson( const JsonObject& data );
    void setId( int id );
    int getId() const;
    void setBlockName( const std::string& id );
    const std::string& getBlockName();
    void setBlockType( RunSceneActionBlockType blockType );
    RunSceneActionBlockType getBlockType() const;
    void setTask( HubPlatform::Services::PostponedTaskPtr& task );
    const std::string& getSceneId();
    void setSceneId( const std::string& id );
    const std::string& getStatus();
    void setStatus( const std::string& status );
    void setCurrentTaskPosition( int position );
    int getCurrentTaskPosition() const;
    void setTimestamp( int64_t timestamp );
    int64_t getTimestamp() const;
    void stop();

private:
    int m_id;
    std::string m_sceneId;
    std::string m_status;
    int m_taskPosition = 0;
    HubPlatform::Services::PostponedTaskPtr m_task = nullptr;
    std::string m_blockName;
    int m_actionId = 0;
    int64_t m_timestamp = 0;
    RunSceneActionBlockType m_blockType = RunSceneActionBlockType::Empty;
};

}  // namespace SceneEngine
