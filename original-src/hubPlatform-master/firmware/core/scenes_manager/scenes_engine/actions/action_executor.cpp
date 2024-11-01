// Extracted from action.h/cpp
#include "scenes_manager/scenes_engine/actions/scene_context_manager.h"
#include "service_interfaces/service.h"
#include "service_interfaces/postponed_task.h"
#include "scenes_manager/scenes_engine/actions/action_executor.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "database_client/dbid.h"
#include "common_utils/cpp14.h"
#include <utility>

namespace SceneEngine {

ActionExecutor::ActionExecutor( HubPlatform::Services::Service& hostService,
                                std::string sceneId,
                                BroadcastCallback sendBroadcast,
                                JsonObject broadcast,
                                SceneContextManagerPtr contextInfos,
                                std::string contextId )
: m_hostService( hostService )
, m_sceneId( std::move( sceneId ) )
, m_sendBroadcast( std::move( sendBroadcast ) )
, m_broadcast( std::move( broadcast ) )
, m_contextManager( contextInfos )
, m_contextId( contextId ) {}

ActionExecutor::~ActionExecutor() {
    if( !m_sendBroadcast || m_broadcast.empty() ) {
        return;
    }

    using namespace SceneEngine::Model::Resources::ScenesState;

    m_broadcast[ "status" ] = Finished;
    std::size_t failedActionQuantity = 0;

    std::string contextId;
    if( !m_resultList.empty() ) {
        contextId = m_resultList.front().contextId;
    }

    if( !contextId.empty() ) {
        if( m_contextManager->isStopWasSent( contextId ) ) {
            m_contextManager->removeContext( contextId );
            return;
        }
    }

    for( const auto& action : m_resultList ) {
        auto contextId = action.contextId;
        m_contextManager->removeContext( contextId );
        if( action.result != false ) {
            continue;
        }

        if( action.executionPolicy == ExecutionPolicy::CheckResult ) {
            m_broadcast[ "status" ] = Failed;
            break;
        }
        if( action.executionPolicy == ExecutionPolicy::IgnoreResult ||
            action.executionPolicy == ExecutionPolicy::WaitResult ) {
            m_broadcast[ "status" ] = PartiallyFinished;
            ++failedActionQuantity;
        }
    }
    if( m_actionQuantity > 0 && m_actionQuantity == failedActionQuantity ) {
        m_broadcast[ "status" ] = Failed;
    }

    m_contextManager->removeContext( m_contextId );
    m_sendBroadcast( m_broadcast );
}

bool ActionExecutor::setResult( const ActionResult& result ) {
    m_resultList.push_back( result );
    if( !result.contextId.empty() ) {
        m_contextManager->removeActonFromRun( result.contextId, result.id );
    }
    return true;
}

void ActionExecutor::setActions( const ActionList& actions ) {
    m_actions = actions;
    m_actionQuantity = actions.size();
}

void ActionExecutor::setActions( ActionExecutor::ActionList&& actions ) {
    m_actionQuantity = actions.size();
    m_actions = std::move( actions );
}

void ActionExecutor::runNextAction( std::string contextId ) {
    using namespace HubPlatform::Services;
    while( !m_actions.empty() ) {
        auto action = m_actions.front();
        if( m_contextManager ) {
            if( m_contextManager->isStopped( contextId, action->getBlockType() ) ) {
                return;
            }
        }
        auto actionId = action->getId();
        auto executor = shared_from_this();

        if( m_contextManager ) {
            m_contextManager->addActonInfoToRun( contextId, m_sceneId, action->getName(), actionId );
            action->setContextId( contextId );
            auto sceneId = m_contextManager->getSceneId( contextId );
            action->setSceneId( sceneId );
            action->setMessage( m_contextManager->getMessage( contextId ) );
        }

        using namespace std::chrono;
        auto runner = [action, executor]( PostponedTask& ) { action->requestRun( executor ); };

        PostponedTaskPtr timer = m_hostService.createPostponedTask( runner );
        timer->start( action->getDelay(), /*isPeriodic*/ false );

        if( m_contextManager ) {
            m_contextManager->setTask( m_contextId, actionId, timer );
            m_contextManager->setPosition( m_contextId, actionId, m_taskPosition++ );
            int64_t now = duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
            m_contextManager->setTimestamp( contextId, actionId, now );
            m_contextManager->setActionType( contextId, actionId, action->getBlockType() );
        }

        m_actions.pop_front();
        if( action->getExecutionPolicy() != ExecutionPolicy::IgnoreResult ) {
            break;
        }
    }
}

void ActionExecutor::setSceneId( std::string sceneId ) {
    m_sceneId = std::move( sceneId );
}

const std::string& ActionExecutor::getSceneId() const {
    return m_sceneId;
}

}  // namespace SceneEngine
