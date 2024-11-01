#include "scenes_manager/scenes_engine/operators/scene_operator.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace {
const char* OperatorStateTrueStr = "true";
const char* OperatorStateFalseStr = "false";
const char* OperatorStateDisabledStr = "disabled";
}  // namespace

namespace SceneEngine {

bool AbstractOperator::isEnabled() const {
    return m_isEnabled;
}

void AbstractOperator::setEnabled( bool enabled ) {
    m_isEnabled = enabled;
}

void AbstractOperator::setParent( SharedOperator parent ) {
    m_parent = std::move( parent );
    if( m_parent ) {
        m_parent->registerChild( shared_from_this() );
    }
}

AbstractOperator::SharedOperator AbstractOperator::getParent() const {
    return m_parent;
}

void AbstractOperator::setBlockID( std::string blockId ) {
    m_blockID = std::move( blockId );
}

const std::string& AbstractOperator::getBlockID() const {
    return m_blockID;
}

void AbstractOperator::setBlockName( std::string name ) {
    m_blockName = std::move( name );
}

const std::string& AbstractOperator::getBlockName() const {
    return m_blockName;
}

bool AbstractOperator::toBool( OperatorState state ) {
    if( state == OperatorState::True ) {
        return true;
    }
    return false;
}

AbstractOperator::OperatorState AbstractOperator::toOperatorState( bool state ) {
    return ( state ) ? OperatorState::True : OperatorState::False;
}

AbstractOperator::OperatorState AbstractOperator::checkParent( SharedOperator child, OperatorState state ) {
    if( m_parent ) {
        return m_parent->checkParent( std::move( child ), state );
    }
    return state;
}

void AbstractOperator::setOperatorStateEventHandler( OperatorStateEventHandler handler ) {
    m_operatorStateEventHandler = std::move( handler );
}

void AbstractOperator::setSceneStateEventHandler( SceneStateEventHandler handler ) {
    m_sceneStateEventHandler = std::move( handler );
}

void AbstractOperator::setSceneStateEventHandlerDisabled( bool value ) {
    m_sceneStateEventHandlerDisabled = value;
}

bool AbstractOperator::isSceneStateEventHandlerDisabled() const {
    return m_sceneStateEventHandlerDisabled;
}

void AbstractOperator::notifyOperatorStateEvent( OperatorState state ) {
    if( m_operatorStateEventHandler ) {
        m_operatorStateEventHandler( state );
    }
}

void AbstractOperator::notifySceneStateEvent( OperatorState state ) {
    if( !isSceneStateEventHandlerDisabled() ) {
        if( m_sceneStateEventHandler ) {
            m_sceneStateEventHandler( state );
        }
    }
}

std::string operatorStateToString( AbstractOperator::OperatorState state ) {
    switch( state ) {
    case AbstractOperator::OperatorState::True:
        return OperatorStateTrueStr;
    case AbstractOperator::OperatorState::False:
        return OperatorStateFalseStr;
    case AbstractOperator::OperatorState::Disabled:
        return OperatorStateDisabledStr;
    }

    LOGGER_ERROR( cc::error( "Cannot convert OperatorState to string, numeric value: " ) +
                  cc::num10( static_cast< int >( state ) ) );
    return {};
}

bool operatorStateFromString( const std::string& str, AbstractOperator::OperatorState& state ) {
    if( str == OperatorStateTrueStr ) {
        state = AbstractOperator::OperatorState::True;
        return true;
    }
    if( str == OperatorStateFalseStr ) {
        state = AbstractOperator::OperatorState::False;
        return true;
    }
    if( str == OperatorStateDisabledStr ) {
        state = AbstractOperator::OperatorState::Disabled;
        return true;
    }

    return false;
}

}  // namespace SceneEngine
