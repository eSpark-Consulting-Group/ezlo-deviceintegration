#include "scenes_manager/scenes_engine/actions/scene_action_execute.h"
#include "scenes_manager/scenes_engine/actions/action_executor.h"
#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "service_interfaces/service.h"
#include "log_utils/ezlo_log.h"
#include <utility>
#include "log_utils/ezlo_log.h"
#include "database_client/dbid.h"
#include "database_client/lmdb++.h"

namespace BlockTypes = HubPlatform::ScenesManager::ActionBlockTypes;

namespace SceneEngine {

SceneActionExecute::SceneActionExecute( ActionBlocks&& actions,
                                        SceneSharedPtr scenePtr,
                                        HubPlatform::Services::Service& hostService,
                                        HubPlatform::CoreDRemoteInterface::CoreDInterface& coreInterface,
                                        ControllerFactoryInterface& controllerFactory,
                                        BroadcastCallback sendBroadcast,
                                        StatesRegistry& statesRegistry,
                                        SceneContextManagerPtr sceneActionContextManager )
: m_scenePtr( std::move( scenePtr ) )
, m_actions( std::move( actions ) )
, m_hostService( hostService )
, m_coreInterface( coreInterface )
, m_controllerFactory( controllerFactory )
, m_sendBroadcast( std::move( sendBroadcast ) )
, m_statesRegistry( statesRegistry )
, m_sceneContextManager( sceneActionContextManager ) {
    m_actions.forEach( [this]( ActionSharedPtr action ) { initializeAction( std::move( action ) ); } );
}

SceneActionExecute::~SceneActionExecute() {
    m_actions.forEach( [this]( ActionSharedPtr action ) { deinitializeAction( std::move( action ) ); } );
}

bool SceneActionExecute::isEnabled() const {
    return m_scenePtr->isEnabled();
}

bool SceneActionExecute::isHouseModeAllowed() const {
    auto houseModeId = m_coreInterface.getCurrentHouseModeId();
    const auto& houseModeList = m_scenePtr->getHouseModes();
    if( !houseModeList.empty() && houseModeList.find( houseModeId ) == houseModeList.end() ) {
        LOGGER_WARN( "wrong house mode" );
        return false;
    }
    return true;
}

bool SceneActionExecute::runThenActions( const std::string& contextId ) const {
    return runActions( m_actions.m_then, true, "then", contextId );
}

bool SceneActionExecute::runThenActions( const std::string& whenBlockID, const std::string& contextId ) const {
    auto groupIter = m_actions.m_thenGroups.find( whenBlockID );
    return groupIter != m_actions.m_thenGroups.end() ? runActions( groupIter->second, false, "thenGroups", contextId ) :
                                                       false;
}

bool SceneActionExecute::runElseActions( const std::string& contextId ) const {
    return runActions( m_actions.m_else, false, "else", contextId );
}

bool SceneActionExecute::runElseActions( const std::string& whenBlockID, const std::string& contextId ) const {
    auto groupIter = m_actions.m_elseGroups.find( whenBlockID );
    return groupIter != m_actions.m_elseGroups.end() ? runActions( groupIter->second, false, "elseGroups", contextId ) :
                                                       false;
}

JsonObject SceneActionExecute::getBroadcastMessage( const std::string& sceneStatus ) const {
    JsonObject broadcast;
    broadcast[ "scene_id" ] = m_scenePtr->getId();
    broadcast[ "scene_name" ] = m_scenePtr->getName();
    broadcast[ "userNotification" ] = sceneStatus != "started";
    broadcast[ "status" ] = sceneStatus;
    broadcast[ "notifications" ] = JsonArray( m_scenePtr->getUserNotifications() );
    const std::string& parentId = m_scenePtr->getParentId();
    if( !parentId.empty() ) {
        broadcast[ "room_id" ] = parentId;
    }
    const std::string& roomName = m_scenePtr->getRoomName();
    if( !roomName.empty() ) {
        broadcast[ "room_name" ] = roomName;
    }
    return broadcast;
}

JsonObject SceneActionExecute::getGlobalBroadcastMessage( const std::string& sceneStatus ) const {
    JsonObject broadcast;
    broadcast[ "sceneId" ] = m_scenePtr->getId();
    broadcast[ "status" ] = sceneStatus;
    broadcast[ "tag" ] = m_scenePtr->getTag();
    broadcast[ "error" ] = nullptr;
    broadcast[ "isGlobal" ] = true;
    if( !m_scenePtr->getName().empty() ) {
        broadcast[ "sceneName" ] = m_scenePtr->getName();
    }
    return broadcast;
}

void SceneActionExecute::sendProgressBroadcast( const std::string& status ) const {
    LOGGER_TRACE( "status: " + status );
    JsonObject broadcast;
    if( m_scenePtr->isGlobal() ) {
        broadcast = getGlobalBroadcastMessage( status );
    }
    else {
        broadcast = getBroadcastMessage( status );
    }
    broadcast[ "status" ] = status;
    m_sendBroadcast( broadcast );
}

SceneSharedPtr SceneActionExecute::getScene() const {
    return m_scenePtr;
}

Scene::SceneID SceneActionExecute::getSceneId() const {
    return m_scenePtr->getId();
}

ActionBlocks::Actions SceneActionExecute::getActions( const std::string& name ) const {
    ActionBlocks::Actions result;
    m_actions.forEach( [&name, &result]( const ActionSharedPtr& action ) {
        if( action->getName() == name ) {
            result.push_back( action );
        }
    } );
    return result;
}

ActionBlocks::Actions SceneActionExecute::getThenActions( const std::string& name ) const {
    ActionBlocks::Actions result;
    for( const ActionSharedPtr& action : m_actions.m_then ) {
        if( action->getName() == name ) {
            result.push_back( action );
        }
    }
    return result;
}

bool SceneActionExecute::runActions( const ActionBlocks::Actions& actions,
                                     bool sendBroadcasts,
                                     const std::string& blockName,
                                     const std::string& contextId ) const {
    try {
        if( !m_scenePtr->isEnabled() ) {
            LOGGER_TRACE( "scene is not enabled" );
            return false;
        }
        if( !isHouseModeAllowed() ) {
            LOGGER_TRACE( "house mode is not allowed" );
            return false;
        }
        using namespace std::chrono;

        if( m_sceneContextManager ) {
            m_sceneContextManager->setSceneId( contextId, m_scenePtr->getId() );
        }

        LOGGER_INFO( cc::info( "start \"" ) + cc::note( blockName ) +
                     cc::info( "\" action block of a scene with ID=" ) + cc::note( m_scenePtr->getId() ) );
        if( sendBroadcasts ) {
            runDelayedActions( actions, contextId );
        }
        else {
            ActionExecutorPtr actionExecutor = std::make_shared< ActionExecutor >( m_hostService,
                                                                                   m_scenePtr->getId(),
                                                                                   BroadcastCallback{},
                                                                                   JsonObject{},
                                                                                   m_sceneContextManager,
                                                                                   contextId );
            if( getGlobalRestrictionState() ) {
                actionExecutor->setActions( ActionExecutor::ActionList( actions.begin(), actions.end() ) );
                actionExecutor->runNextAction( contextId );
            }
            else {
                LOGGER_INFO( cc::info( "Skipping actions execution due to global restriction, scene id: " ) +
                             cc::note( m_scenePtr->getId() ) );
            }
        }
    }
    catch( lmdb::MapFullError& ex ) {
        LOGGER_ERROR( cc::error( "actions running exception: " ) + cc::note( ex.what() ) +
                      cc::note( " scene contextId: " + contextId ) );
        LOGGER_INFO( "clear execution context info and retry run action" );
        if( m_sceneContextManager ) {
            m_sceneContextManager->clearAllContexts();
        }

        return runActions( actions, sendBroadcasts, blockName, contextId );
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( cc::error( "actions running exception: " ) + cc::note( ex.what() ) );
        return false;
    }
    return true;
}

void SceneActionExecute::runDelayedActions( const ActionBlocks::Actions& actions, const std::string& contextId ) const {
    LOGGER_TRACE( "run delayed action" );
    sendProgressBroadcast( "started" );
    JsonObject broadcast;
    if( m_scenePtr->isGlobal() ) {
        broadcast = getGlobalBroadcastMessage( "finished" );
    }
    else {
        broadcast = getBroadcastMessage( "finished" );
    }

    ActionExecutorPtr actionExecutor = std::make_shared< ActionExecutor >(
        m_hostService, m_scenePtr->getId(), m_sendBroadcast, broadcast, m_sceneContextManager, contextId );
    if( getGlobalRestrictionState() ) {
        actionExecutor->setActions( ActionExecutor::ActionList( actions.begin(), actions.end() ) );
        actionExecutor->runNextAction( contextId );
    }
    else {
        LOGGER_INFO( cc::info( "Skipping actions execution due to global restriction, scene id: " ) +
                     cc::note( m_scenePtr->getId() ) );
    }
}

void SceneActionExecute::initializeAction( ActionSharedPtr action ) {
    if( action->getSubscriptionState() == SubscriptionState::Item ) {
        action->setInitialized( false );
        auto controllerInterface = m_controllerFactory.getControllerInterface( action->getControllerId() );
        Json itemIds;
        json_get( action->getArgs(), "item", itemIds );
        if( itemIds.is_array() ) {
            for( const auto& itemId : itemIds ) {
                controllerInterface->subscribeOnItemChanges( json_get_str( itemId ),
                                                             [action]() { action->setInitialized( true ); } );
            }
        }
        else if( itemIds.is_string() ) {
            controllerInterface->subscribeOnItemChanges( itemIds, [action]() { action->setInitialized( true ); } );
        }
        else {
            LOGGER_ERROR( "Bad item ids: " + cc::j( itemIds ) );
        }
    }
    else if( action->getSubscriptionState() == SubscriptionState::Device ) {
        action->setInitialized( false );
        auto controllerInterface = m_controllerFactory.getControllerInterface( action->getControllerId() );
        auto deviceId = json_get_str( action->getArgs(), "device" );
        controllerInterface->subscribeOnDevicesChanges( deviceId, [action]() { action->setInitialized( true ); } );
    }
}

void SceneActionExecute::deinitializeAction( ActionSharedPtr action ) {
    auto subscriptionState = action->getSubscriptionState();
    if( subscriptionState == SubscriptionState::Item ) {
        auto controllerInterface = m_controllerFactory.getControllerInterface( action->getControllerId() );
        Json itemIds;
        json_get( action->getArgs(), "item", itemIds );
        if( itemIds.is_array() ) {
            for( const auto& itemId : itemIds ) {
                controllerInterface->unsubscribeFromItemChanges( json_get_str( itemId ) );
            }
        }
        else if( itemIds.is_string() ) {
            controllerInterface->unsubscribeFromItemChanges( json_get_str( action->getArgs(), "item" ) );
        }
        else {
            LOGGER_ERROR( "Bad item ids: " + cc::j( itemIds ) );
        }
    }
    else if( subscriptionState == SubscriptionState::Device ) {
        auto controllerInterface = m_controllerFactory.getControllerInterface( action->getControllerId() );
        controllerInterface->unsubscribeFromDevicesChanges( json_get_str( action->getArgs(), "device" ) );
    }
}

bool SceneActionExecute::hasThenActions() const {
    return !m_actions.m_then.empty();
}

bool SceneActionExecute::hasElseActions() const {
    return !m_actions.m_else.empty();
}

void SceneActionExecute::stopScene( const std::string& sceneId, const RunSceneActionBlockType blockType ) {
    if( m_sceneContextManager && m_sceneContextManager->stopActions( sceneId, blockType, [this, sceneId]() {
            auto broadcast = getBroadcastMessage( "stopped" );
            m_sendBroadcast( broadcast );
        } ) ) {
    }
}

bool SceneActionExecute::getGlobalRestrictionState() const {
    bool globalRestrictionState = true;

    if( !m_scenePtr->getGlobalRestrictionId().empty() ) {
        auto globalRestrictionData =
            m_statesRegistry.readBlockState( m_scenePtr->getId(), m_scenePtr->getGlobalRestrictionId() );
        globalRestrictionState = AbstractOperator::toBool( globalRestrictionData.state );
    }
    return globalRestrictionState;
}

}  // namespace SceneEngine
