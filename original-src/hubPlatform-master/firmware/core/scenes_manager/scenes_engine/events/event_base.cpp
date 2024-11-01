#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"
#include "scenes_manager/scenes_engine/operators/event_operator.h"
#include <utility>

namespace SceneEngine {

EventBase::EventBase()
: m_eventOperatorPtr( std::make_shared< EventOperator >() ) {}

void EventBase::setEventId( const EventId& eventId ) {
    m_eventId = eventId;
    m_eventOperatorPtr->setBlockID( eventId );
}

const EventBase::EventId& EventBase::getEventId() const {
    return m_eventId;
}

void EventBase::setBlockName( std::string name ) {
    m_eventOperatorPtr->setBlockName( std::move( name ) );
}

const std::string& EventBase::getBlockName() const {
    return m_eventOperatorPtr->getBlockName();
}

const Scene& EventBase::getScene() const {
    if( !m_actionExecutorPtr ) {
        throw EventException( "Empty action executor, event id: " + getEventId() );
    }
    return *m_actionExecutorPtr->getScene();
}

Scene::SceneID EventBase::getSceneId() const {
    if( !m_actionExecutorPtr ) {
        throw EventException( "Empty action executor, event id: " + getEventId() );
    }
    return m_actionExecutorPtr->getSceneId();
}

void EventBase::setSceneActionExecutor( SceneActionExecutePtr actionExecutor ) {
    m_actionExecutorPtr = std::move( actionExecutor );
}

bool EventBase::hasSceneActionExecutor() const {
    return m_actionExecutorPtr != nullptr;
}

void EventBase::setOperator( AbstractOperator::SharedOperator parentOperatorPtr ) {
    m_eventOperatorPtr->setParent( std::move( parentOperatorPtr ) );
}

AbstractOperator::SharedOperator EventBase::getOperator() const {
    return m_eventOperatorPtr;
}

void EventBase::setEnabled( bool enabled ) {
    m_eventOperatorPtr->setEnabled( enabled );
}

void EventBase::setOperatorState( bool state ) {
    m_eventOperatorPtr->setState( state );
}

AbstractOperator::OperatorState EventBase::evaluateSubtree() {
    return m_eventOperatorPtr->evaluateSubtree();
}

void EventBase::notifyOperatorState() {
    m_eventOperatorPtr->notifyOperatorState();
}

void EventBase::triggerOperatorAndCheckCondition() {
    setOperatorAndCheckCondition( true );
    m_eventOperatorPtr->setState( false );
}

void EventBase::setOperatorAndCheckCondition( bool state ) {
    m_eventOperatorPtr->setState( state );
    m_eventOperatorPtr->notifyOperatorState();
    m_eventOperatorPtr->evaluateSubtree();
}

bool EventBase::runActionsIfSucceededCondition( bool state ) {
    if( m_eventOperatorPtr->isEnabled() ) {
        setOperatorAndCheckCondition( state );
        return true;
    }

    return false;
}

bool EventBase::runActionsIfTriggeredCondition() {
    if( m_eventOperatorPtr->isEnabled() ) {
        triggerOperatorAndCheckCondition();
        return true;
    }
    return false;
}

EventBase::ValueKeys EventBase::getValueKeys() const {
    return {};
}

}  // namespace SceneEngine
