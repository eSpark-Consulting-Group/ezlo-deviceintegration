#include "scenes_manager/scenes_engine/operators/visitors/operator_status_visitor.h"
#include "scenes_manager/scenes_engine/operators/visitors/operator_name_visitor.h"
#include "scenes_manager/scenes_engine/operators/operators.h"
#include "scenes_manager/scenes_engine/scene_status_resources.h"

#include "base_utils/timeutils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

OperatorStatusVisitor::OperatorStatusVisitor( Scene::SceneID sceneId,
                                              const EventMap& sceneEvents,
                                              const StatesRegistry& statesRegistry )
: m_sceneId( std::move( sceneId ) )
, m_sceneEvents( sceneEvents )
, m_statesRegistry( statesRegistry )
, m_fullStatus( JSON_EMPTY_ARR )
, m_parent( &m_fullStatus ) {}

const Json& OperatorStatusVisitor::getStatus() const {
    return m_fullStatus;
}

void OperatorStatusVisitor::visit( EventOperator& op ) {
    if( !markOperatorVisited( op ) )
        return;

    const std::string& blockId = op.getBlockID();
    auto it = m_sceneEvents.find( blockId );
    if( it != m_sceneEvents.end() ) {
        const auto registryRecord = m_statesRegistry.readBlockState( m_sceneId, blockId );
        Json status = createStatusJson( blockId, it->second->getMethodName(), op.getBlockName(), registryRecord );
        m_parent->push_back( std::move( status ) );
    }
    else {
        LOGGER_ERROR_RAW(
            cc::error( "Didn't find event for the operator while getting status of the block with id: " ) +
            cc::note( blockId ) );
    }
}

void OperatorStatusVisitor::visit( NotOperator& op ) {
    if( !markOperatorVisited( op ) )
        return;

    Json status = getOperatorStatus( op );

    Json childrenBlocks = JSON_EMPTY_ARR;
    Json* oldParent = m_parent;
    m_parent = &childrenBlocks;

    if( auto& child = op.getChild() ) {
        child->accept( *this );
        if( !childrenBlocks.empty() )
            status[ SceneStatusFields::Blocks ] = std::move( childrenBlocks );
    }

    m_parent = oldParent;
    m_parent->push_back( std::move( status ) );
}

void OperatorStatusVisitor::visit( MultipleChildrenOperator& op ) {
    if( !markOperatorVisited( op ) )
        return;

    Json status = getOperatorStatus( op );
    status[ SceneStatusFields::Blocks ] = collectChildrenStatus( op );

    m_parent->push_back( std::move( status ) );
}

void OperatorStatusVisitor::visit( OrOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( XorOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( AndOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( ForLessFunctionOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( ForAtLeastFunctionOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( RepeatFunctionOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( FollowFunctionOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( PulseFunctionOperator& op ) {
    visit( static_cast< MultipleChildrenOperator& >( op ) );
}

void OperatorStatusVisitor::visit( LatchFunctionOperator& op ) {
    using namespace SceneStatusFields;

    if( !markOperatorVisited( op ) )
        return;

    Json status = getOperatorStatus( op );
    status[ Function ][ IsLatched ] = op.isLatched();
    status[ Blocks ] = collectChildrenStatus( op );

    m_parent->push_back( std::move( status ) );
}

bool OperatorStatusVisitor::markOperatorVisited( const AbstractOperator& op ) {
    auto insertResult = m_visitedOperators.insert( op.getBlockID() );
    return insertResult.second;
}

Json OperatorStatusVisitor::getOperatorStatus( AbstractOperator& op ) {
    using namespace SceneStatusFields;

    OperatorNameVisitor nameVisitor;
    op.accept( nameVisitor );

    const std::string& blockId = op.getBlockID();
    const auto stateRecord = m_statesRegistry.readBlockState( m_sceneId, blockId );
    Json status = createStatusJson( blockId, nameVisitor.name(), op.getBlockName(), stateRecord );

    if( nameVisitor.isFunction() )
        status[ Function ][ Name ] = nameVisitor.functionName();

    return status;
}

Json OperatorStatusVisitor::collectChildrenStatus( MultipleChildrenOperator& op ) {
    Json childrenBlocks = JSON_EMPTY_ARR;
    Json* oldParent = m_parent;
    m_parent = &childrenBlocks;

    op.forEach( [this]( AbstractOperator& child ) { child.accept( *this ); } );

    m_parent = oldParent;
    return childrenBlocks;
}

Json OperatorStatusVisitor::createStatusJson( const std::string& id,
                                              const std::string& name,
                                              const std::string& blockName,
                                              const StatesRegistry::Record& registryRecord ) {
    using namespace SceneStatusFields;

    Json lastChangeTime;
    if( registryRecord.lastChangeTime != StatesRegistry::TimePoint() )
        lastChangeTime = TimeUtils::chronoTimePointToString( registryRecord.lastChangeTime );

    std::string stateStr;
    if( registryRecord.isValid )
        stateStr = operatorStateToString( registryRecord.state );
    else
        stateStr = UnknownState;

    Json status;
    status[ Id ] = id;
    status[ Name ] = name;
    status[ State ] = stateStr;
    status[ LastChange ] = std::move( lastChangeTime );

    if( !blockName.empty() )
        status[ BlockName ] = blockName;

    return status;
}

}  // namespace SceneEngine
