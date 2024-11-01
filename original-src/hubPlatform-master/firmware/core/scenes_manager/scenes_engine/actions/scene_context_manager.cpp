#include "scenes_manager/scenes_engine/actions/scene_context_manager.h"
#include "log_utils/ezlo_log.h"
#include "database_client/dbid.h"

namespace SceneEngine {
SceneContextManager::SceneContextManager( IDbClient& dbClient )
: m_dbClient( dbClient ) {
    clearAllContexts();
}

std::string SceneContextManager::addActonInfoToRun( const std::string& contextId,
                                                    const std::string& sceneId,
                                                    const std::string& blockName,
                                                    int actionId ) {
    auto actionInfo = std::make_shared< SingleSceneContext >( sceneId, blockName, actionId );
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        context->addActionInfo( std::move( actionInfo ) );
    }
    return std::to_string( actionId );
}

void SceneContextManager::removeActonFromRun( const std::string& contextId, int actionId ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        context->removeActionInfo( actionId );
    }
}

void SceneContextManager::setTask( const std::string& contextId,
                                   int actionId,
                                   HubPlatform::Services::PostponedTaskPtr& task ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        auto actionInfo = context->getActionInfo( std::to_string( actionId ) );
        if( actionInfo && task ) {
            actionInfo->setTask( task );
        }
    }
}

void SceneContextManager::setPosition( const std::string& contextId, int actionId, int position ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        auto actionInfo = context->getActionInfo( std::to_string( actionId ) );
        if( actionInfo ) {
            actionInfo->setCurrentTaskPosition( position );
        }
    }
}

void SceneContextManager::setActionType( const std::string& contextId,
                                         int actionId,
                                         RunSceneActionBlockType blockType ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        auto actionInfo = context->getActionInfo( std::to_string( actionId ) );
        if( actionInfo ) {
            actionInfo->setBlockType( blockType );
        }
    }
}

void SceneContextManager::setTimestamp( const std::string& contextId, int actionId, int64_t timestamp ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        auto actionInfo = context->getActionInfo( std::to_string( actionId ) );
        if( actionInfo ) {
            actionInfo->setTimestamp( timestamp );
        }
    }
}

void SceneContextManager::removeContext( const std::string& contextId ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        context->remove();
        m_sceneExecutionContexts.erase( contextId );
    }
}

JsonObject SceneContextManager::getMessage( const std::string& contextId ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        return context->getMessage();
    }

    return JSON_EMPTY_OBJ;
}

std::string SceneContextManager::getSceneId( const std::string& contextId ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        return context->getSceneId();
    }

    return "";
}

void SceneContextManager::setSceneId( const std::string& contextId, const std::string& sceneId ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        return context->setSceneId( sceneId );
    }
}

const std::string SceneContextManager::createNewExecutionContext( const std::string& sceneId,
                                                                  int64_t timestamp,
                                                                  const JsonObject& message ) {
    auto contextId = db::DbId::genOID();
    if( !contextId.empty() ) {
        auto context = std::make_shared< SceneExecutionContext >( contextId, sceneId, timestamp, m_dbClient, message );
        m_sceneExecutionContexts[ contextId ] = std::move( context );
    }
    return contextId;
}

bool SceneContextManager::stopActions( const std::string& sceneId,
                                       const RunSceneActionBlockType blockType,
                                       std::function< void() > sendBroadcast ) {
    for( auto context : m_sceneExecutionContexts ) {
        if( context.second ) {
            if( context.second->getSceneId() == sceneId ) {
                context.second->stopAllActions( blockType );
                if( context.second->isStoppedActions() ) {
                    if( sendBroadcast && blockType != RunSceneActionBlockType::Else ) {
                        sendBroadcast();
                        context.second->stopWasSent( true );
                        context.second->remove();
                    }
                    return true;
                }
            }
        }
    }

    return false;
}

bool SceneContextManager::isStopped( const std::string& contextId, const RunSceneActionBlockType blockType ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        return context->isStopped( blockType );
    }
    return false;
}

bool SceneContextManager::isStopWasSent( const std::string& contextId ) {
    auto context = m_sceneExecutionContexts[ contextId ];
    if( context ) {
        return context->isStopWasSent();
    }
    return false;
}

void SceneContextManager::clearAllContexts() {
    m_dbClient.clear( HaTempDBSceneExecutionContextDBName );
}

}  // namespace SceneEngine
