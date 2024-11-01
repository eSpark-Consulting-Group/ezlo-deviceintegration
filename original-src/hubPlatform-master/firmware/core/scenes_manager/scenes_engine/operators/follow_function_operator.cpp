#include "scenes_manager/scenes_engine/operators/follow_function_operator.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

FollowFunctionOperator::FollowFunctionOperator( HubPlatform::Services::TaskLoop& taskLoop,
                                                std::chrono::seconds resetDelay )
: m_taskLoop( taskLoop )
, m_resetDelay( resetDelay )
, m_actualState( OperatorState::False ) {
    if( m_resetDelay <= std::chrono::seconds::zero() ) {
        using namespace Model::Resources::FunctionFields;
        std::string message = std::string( Follow ) + ": '" + DelayReset + "' parameter should be positive integer";
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message );
    }

    setInterceptSceneState( true );
}

FollowFunctionOperator::~FollowFunctionOperator() {
    if( m_delayedResetTask )
        m_delayedResetTask->stop();
}

AbstractOperator::OperatorState FollowFunctionOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( !m_isEnabled || getChildren().empty() ) {
        return OperatorState::Disabled;
    }

    const OperatorState previousShownState = m_delayedResetTask ? OperatorState::True : m_actualState;

    const OperatorState newState = checkChildren();
    if( newState != m_actualState ) {
        processActualStateChanged( newState );
    }

    const OperatorState shownState = m_delayedResetTask ? OperatorState::True : newState;
    const OperatorState subtreeState = AbstractOperator::checkParent( shared_from_this(), shownState );

    if( previousShownState != shownState ) {
        notifySceneStateEvent( subtreeState );
    }

    return subtreeState;
}

AbstractOperator::OperatorState FollowFunctionOperator::checkChildren() {
    return AndOperator::checkChildren();
}

void FollowFunctionOperator::processActualStateChanged( OperatorState newState ) {
    const OperatorState previousState = m_actualState;
    m_actualState = newState;

    if( previousState == OperatorState::True && m_actualState == OperatorState::False ) {
        scheduleReset();
    }
    else {
        const bool suppressOperatorStateEvent = m_delayedResetTask && m_actualState == OperatorState::True;
        if( m_delayedResetTask ) {
            m_delayedResetTask->stop();
            m_delayedResetTask.reset();
        }

        if( !suppressOperatorStateEvent )
            notifyOperatorStateEvent( m_actualState );
    }
}

void FollowFunctionOperator::scheduleReset() {
    auto sharedThis = std::static_pointer_cast< FollowFunctionOperator >( shared_from_this() );
    auto delayedResetFun = [sharedThis]( const HubPlatform::Services::PostponedTask& task ) {
        sharedThis->m_delayedResetTask.reset();

        sharedThis->notifyOperatorStateEvent( OperatorState::False );

        const OperatorState sceneState = sharedThis->AbstractOperator::checkParent( sharedThis, OperatorState::False );
        sharedThis->notifySceneStateEvent( sceneState );
    };

    m_delayedResetTask = m_taskLoop.createPostponedTask( delayedResetFun );
    m_delayedResetTask->start( m_resetDelay, false );
}

void FollowFunctionOperator::accept( OperatorVisitor& visitor ) {
    visitor.visit( *this );
}

}  // namespace SceneEngine
