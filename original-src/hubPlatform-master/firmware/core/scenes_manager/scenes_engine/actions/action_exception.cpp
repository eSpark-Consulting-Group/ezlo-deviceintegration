#include "scenes_manager/scenes_engine/actions/action_exception.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/modify_scene/resources.h"

namespace SceneEngine {

ActionException::ActionException( ActionExceptionData exceptionData,
                                  const StatesRegistry& statesRegistry,
                                  const Expressions::ExpressionManager& expressions )
: m_exceptionData( std::move( exceptionData ) )
, m_statesRegistry( statesRegistry )
, m_expressions( expressions ) {}

ActionException::State ActionException::evaluate() {
    // TODO replace with mpark::visit + polymorphic lambda after moving to C++14
    if( mpark::holds_alternative< mpark::monostate >( m_exceptionData.storage ) ) {
        return State::Satisfied;
    }
    else if( mpark::holds_alternative< BlockReference >( m_exceptionData.storage ) ) {
        return evaluate( mpark::get< BlockReference >( m_exceptionData.storage ) );
    }
    else if( mpark::holds_alternative< ExpressionReference >( m_exceptionData.storage ) ) {
        return evaluate( mpark::get< ExpressionReference >( m_exceptionData.storage ) );
    }

    return State::Dissatisfied;
}

ActionException::State ActionException::evaluate( const BlockReference& blockReference ) const {
    const auto record = m_statesRegistry.readBlockState( blockReference.sceneId, blockReference.blockId );
    if( !record.isValid ) {
        auto message = "State of the block not found, scene id: " + blockReference.sceneId +
                       " block id: " + blockReference.blockId;
        throw ActionExceptionError( message );
    }

    return AbstractOperator::toBool( record.state ) ? State::Satisfied : State::Dissatisfied;
}

ActionException::State ActionException::evaluate( const ExpressionReference& expressionReference ) const {
    using Status = SceneEngine::Expressions::ExpressionManager::FetchResult::Status;
    auto result = m_expressions.fetchValue( expressionReference.name );
    if( Status::NotFound == result.status )
        throw ActionExceptionError( "Expression value cannot be fetched, expression: " + expressionReference.name );

    if( !result.isOk() )
        throw ActionExceptionError( "Expression has a value error" );

    ItemValueWrapper actual( std::move( result.value ) );
    if( !actual.isValid() )
        throw ActionExceptionError( "Cannot create a valid value from expression's value" );

    ItemValueWrapper expected = getExpectedExpressionValue();
    if( !actual.isComparableWith( expected ) )
        throw ActionExceptionError( "Expression's value is incomparable with 'bool' type" );

    return actual == expected ? State::Satisfied : State::Dissatisfied;
}

ItemValueWrapper ActionException::getExpectedExpressionValue() {
    return { JSON( Fields::TypeField << Types::BoolType << Fields::ValueField << true ) };
}

}  // namespace SceneEngine
