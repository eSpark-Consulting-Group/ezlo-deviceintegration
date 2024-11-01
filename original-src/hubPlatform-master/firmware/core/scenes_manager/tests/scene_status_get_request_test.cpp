#include "scenes_manager/tests/fixtures/states_registry_fixture.h"

#include "scenes_manager/scenes_engine/operators/visitors/operator_status_visitor.h"
#include "scenes_manager/scenes_engine/operators/visitors/operator_name_visitor.h"
#include "scenes_manager/scenes_engine/operators/operators.h"
#include "scenes_manager/scenes_engine/scene_status_resources.h"

#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/events/scene_state_event.h"
#include "scenes_manager/scenes_engine/events/group_state_event.h"
#include "scenes_manager/scenes_engine/events/is_item_state_changed_event.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"

#include "scenes_manager/scenes_engine/scene_status_builder.h"
#include "scenes_manager/scenes_engine/states_registry_resources.h"

#include "base_utils/timeutils.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. OperatorStatusVisitor
DONE        1.1 Creation of status JSON (the method from visitor is reused in tests)
DONE        1.2 Single "compareNumbers" event
DONE        1.3 Three different events
DONE        1.4 "not" operator
DONE        1.5 "or" operator with one child
DONE        1.6 "or" operator with three children
DONE        1.7 "pulse" function
DONE        1.8 "latch" function
DONE        1.9 Complex combination of different blocks
DONE        1.10 Corrupted operator state in DB

DONE    2. SceneStatusBuilder
DONE        2.1 Status of scene, which hasn't ever executed
DONE        2.2 Scene without events or operators
DONE        2.3 Scene with triggers
DONE        2.4 Scene with exceptions
DONE        2.5 Scene with triggers and exceptions
DONE        2.6 Scene with error
DONE        2.7 Scene with globalRestriction
DONE        2.8 Scene with triggers, exceptions and globalRestriction

*/

class SceneStatusGetRequestTest : public StatesRegistryFixture {
public:
    const OperatorStatusVisitor::EventMap& triggersMap() const;
    const OperatorStatusVisitor::EventMap& exceptionsMap() const;
    const OperatorStatusVisitor::EventMap& globalRestrictionMap() const;

    template < typename T, typename... Args >
    std::shared_ptr< T > createOperator( Args&&... args );
    template < typename T, typename... Args >
    std::shared_ptr< T > createTrigger( Args&&... args );
    template < typename T, typename... Args >
    std::shared_ptr< T > createException( Args&&... args );
    template < typename T, typename... Args >
    std::shared_ptr< T > createGlobalRestriction( Args&&... args );

    Json getOperatorStatus( const EventBase& event ) const;
    Json getOperatorStatus( AbstractOperator& op ) const;
    static Json createBlockStatusJson( const std::string& id,
                                       const std::string& name,
                                       const std::string& blockName,
                                       AbstractOperator::OperatorState state,
                                       StatesRegistry::TimePoint lastChangeTime );
    static Json createSceneStatusJson( const Scene::SceneID& sceneId,
                                       const Json& triggers,
                                       const Json& exceptions,
                                       const Json& globalRestriction,
                                       const Json& error,
                                       AbstractOperator::OperatorState state,
                                       StatesRegistry::TimePoint lastChangeTime );

private:
    template < typename T, typename... Args >
    std::shared_ptr< T > createEvent( OperatorStatusVisitor::EventMap& container, Args&&... args );

    int m_idCounter = 0;
    OperatorStatusVisitor::EventMap m_triggers;
    OperatorStatusVisitor::EventMap m_exceptions;
    OperatorStatusVisitor::EventMap m_globalRestriction;
};

template < typename T, typename... Args >
inline std::shared_ptr< T > SceneStatusGetRequestTest::createOperator( Args&&... args ) {
    auto op = std::make_shared< T >( std::forward< Args >( args )... );

    const std::string blockId = std::to_string( m_idCounter++ );
    op->setBlockID( blockId );

    m_statesRegistry->storeInitialBlockState( m_testSceneId, blockId, AbstractOperator::OperatorState::False );

    return op;
}

template < typename T, typename... Args >
inline std::shared_ptr< T > SceneStatusGetRequestTest::createTrigger( Args&&... args ) {
    return createEvent< T >( m_triggers, std::forward< Args >( args )... );
}

template < typename T, typename... Args >
inline std::shared_ptr< T > SceneStatusGetRequestTest::createException( Args&&... args ) {
    return createEvent< T >( m_exceptions, std::forward< Args >( args )... );
}

template < typename T, typename... Args >
inline std::shared_ptr< T > SceneStatusGetRequestTest::createGlobalRestriction( Args&&... args ) {
    return createEvent< T >( m_globalRestriction, std::forward< Args >( args )... );
}

template < typename T, typename... Args >
std::shared_ptr< T > SceneStatusGetRequestTest::createEvent( OperatorStatusVisitor::EventMap& container,
                                                             Args&&... args ) {
    auto op = std::make_shared< T >( std::forward< Args >( args )... );

    const std::string eventId = std::to_string( m_idCounter++ );
    op->setEventId( eventId );
    container.emplace( eventId, op );

    m_statesRegistry->storeInitialBlockState( m_testSceneId, eventId, AbstractOperator::OperatorState::False );

    return op;
}

const OperatorStatusVisitor::EventMap& SceneStatusGetRequestTest::triggersMap() const {
    return m_triggers;
}

const OperatorStatusVisitor::EventMap& SceneStatusGetRequestTest::exceptionsMap() const {
    return m_exceptions;
}

const OperatorStatusVisitor::EventMap& SceneStatusGetRequestTest::globalRestrictionMap() const {
    return m_globalRestriction;
}

Json SceneStatusGetRequestTest::getOperatorStatus( const EventBase& event ) const {
    const std::string& blockId = event.getOperator()->getBlockID();
    const auto blockState = readBlockState( blockId );
    return createBlockStatusJson(
        blockId, event.getMethodName(), event.getBlockName(), blockState.state, blockState.lastChangeTime );
}

Json SceneStatusGetRequestTest::getOperatorStatus( AbstractOperator& op ) const {
    OperatorNameVisitor nameVisitor;
    op.accept( nameVisitor );

    const std::string& blockId = op.getBlockID();
    const auto blockState = readBlockState( blockId );
    Json result = createBlockStatusJson(
        blockId, nameVisitor.name(), op.getBlockName(), blockState.state, blockState.lastChangeTime );

    if( nameVisitor.isFunction() )
        result[ "function" ][ "name" ] = nameVisitor.functionName();

    return result;
}

Json SceneStatusGetRequestTest::createBlockStatusJson( const std::string& id,
                                                       const std::string& name,
                                                       const std::string& blockName,
                                                       AbstractOperator::OperatorState state,
                                                       StatesRegistry::TimePoint lastChangeTime ) {
    StatesRegistry::Record record( lastChangeTime, state, true );
    return OperatorStatusVisitor::createStatusJson( id, name, blockName, record );
}

Json SceneStatusGetRequestTest::createSceneStatusJson( const Scene::SceneID& sceneId,
                                                       const Json& triggers,
                                                       const Json& exceptions,
                                                       const Json& globalRestriction,
                                                       const Json& error,
                                                       AbstractOperator::OperatorState state,
                                                       StatesRegistry::TimePoint lastChangeTime ) {
    using namespace SceneStatusFields;

    Json status;
    status[ State ] = operatorStateToString( state );
    status[ WhenBlocks ] = triggers;
    status[ Exceptions ] = exceptions;
    status[ GlobalRestriction ] = globalRestriction;

    if( lastChangeTime != StatesRegistry::TimePoint() )
        status[ LastChange ] = TimeUtils::chronoTimePointToString( lastChangeTime );
    else
        status[ LastChange ] = Json();

    if( !error.empty() )
        status[ Error ] = error;

    Json result;
    result[ Id ] = sceneId;
    result[ Status ] = std::move( status );
    return result;
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_createStatusJson ) {  // 1.1
    using namespace std::chrono;

    const std::string testId = "id";
    const std::string testMethod = "method";
    const std::string testBlockName = "blockName";

    Json expected;
    expected[ SceneStatusFields::Id ] = testId;
    expected[ SceneStatusFields::Name ] = testMethod;
    expected[ SceneStatusFields::BlockName ] = testBlockName;

    // needed because printing respects time zones
    seconds utcOffsetTime( 0 );
    std::string utcOffsetTimeStr;
    {
        const auto sampleTime = StatesRegistry::TimePoint() + hours( 20 );
        const std::time_t tt = system_clock::to_time_t( sampleTime );
        std::tm tmLocal = *std::localtime( &tt );
        std::tm tmUtc = *std::gmtime( &tt );

        utcOffsetTime = seconds( std::mktime( &tmLocal ) - std::mktime( &tmUtc ) );
        utcOffsetTimeStr = TimeUtils::timeStructToString( tmLocal, "%z" );
    }

    {
        const auto state = AbstractOperator::OperatorState::True;
        const Json status =
            createBlockStatusJson( testId, testMethod, testBlockName, state, StatesRegistry::TimePoint() );
        expected[ SceneStatusFields::State ] = operatorStateToString( state );
        expected[ SceneStatusFields::LastChange ] = Json();

        EXPECT_EQ( status, expected );
    }
    {
        const auto state = AbstractOperator::OperatorState::False;
        const auto timePoint =
            StatesRegistry::TimePoint() + hours( 10 ) + minutes( 20 ) + seconds( 30 ) - utcOffsetTime;
        const Json status = createBlockStatusJson( testId, testMethod, testBlockName, state, timePoint );
        expected[ SceneStatusFields::State ] = operatorStateToString( state );
        expected[ SceneStatusFields::LastChange ] = "1970-01-01T10:20:30" + utcOffsetTimeStr;

        EXPECT_EQ( status, expected );
    }
    {
        const auto state = AbstractOperator::OperatorState::Disabled;
        const auto timePoint =
            StatesRegistry::TimePoint() + hours( 48 ) + minutes( 20 ) + seconds( 30 ) - utcOffsetTime;
        const Json status = createBlockStatusJson( testId, testMethod, testBlockName, state, timePoint );
        expected[ SceneStatusFields::State ] = operatorStateToString( state );
        expected[ SceneStatusFields::LastChange ] = "1970-01-03T00:20:30" + utcOffsetTimeStr;

        EXPECT_EQ( status, expected );
    }
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_singleEvent ) {  // 1.2
    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    event->setBlockName( "someBlockName" );

    setClockTime( std::chrono::minutes( 10 ) );
    storeBlockState( event->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    event->getOperator()->accept( visitor );

    const Json expected = { getOperatorStatus( *event ) };
    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_threeDifferentEvents ) {  // 1.3
    auto event1 = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createTrigger< GroupStateEvent >( "", "", GroupStateEvent::State::True );
    auto event3 = createTrigger< SceneStateEvent >( "", SceneStateEvent::Action::AnyResult );

    event1->setBlockName( "someBlockName1" );
    event2->setBlockName( "someBlockName2" );
    event3->setBlockName( "someBlockName3" );

    storeBlockState( event1->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    storeBlockState( event2->getOperator()->getBlockID(), AbstractOperator::OperatorState::False );
    storeBlockState( event3->getOperator()->getBlockID(), AbstractOperator::OperatorState::Disabled );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    event1->getOperator()->accept( visitor );
    event2->getOperator()->accept( visitor );
    event3->getOperator()->accept( visitor );

    const Json expected = { getOperatorStatus( *event1 ), getOperatorStatus( *event2 ), getOperatorStatus( *event3 ) };
    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_notOpeator ) {  // 1.4
    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto op = createOperator< NotOperator >();
    event->setOperator( op );

    setClockTime( std::chrono::minutes( 10 ) );
    storeBlockState( event->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    setClockTime( std::chrono::minutes( 30 ) );
    storeBlockState( op->getBlockID(), AbstractOperator::OperatorState::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    op->accept( visitor );

    Json opStatus = getOperatorStatus( *op );
    opStatus[ SceneStatusFields::Blocks ] = { getOperatorStatus( *event ) };
    const Json expected = { std::move( opStatus ) };

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_orOperatorWithOneChild ) {  // 1.5
    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto op = createOperator< OrOperator >();
    event->setOperator( op );

    storeBlockState( event->getOperator()->getBlockID(), AbstractOperator::True );
    storeBlockState( op->getBlockID(), AbstractOperator::OperatorState::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    op->accept( visitor );

    Json opStatus = getOperatorStatus( *op );
    opStatus[ SceneStatusFields::Blocks ] = { getOperatorStatus( *event ) };
    const Json expected = { std::move( opStatus ) };

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_orOperatorWithThreeChildren ) {  // 1.6
    auto event1 = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createTrigger< GroupStateEvent >( "", "", GroupStateEvent::State::True );
    auto event3 = createTrigger< SceneStateEvent >( "", SceneStateEvent::Action::AnyResult );
    auto op = createOperator< OrOperator >();
    event1->setOperator( op );
    event2->setOperator( op );
    event3->setOperator( op );

    storeBlockState( event1->getOperator()->getBlockID(), AbstractOperator::OperatorState::Disabled );
    storeBlockState( event2->getOperator()->getBlockID(), AbstractOperator::OperatorState::False );
    storeBlockState( event3->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    storeBlockState( op->getBlockID(), AbstractOperator::OperatorState::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    op->accept( visitor );

    Json opStatus = getOperatorStatus( *op );
    opStatus[ SceneStatusFields::Blocks ] = {
        getOperatorStatus( *event1 ), getOperatorStatus( *event2 ), getOperatorStatus( *event3 )
    };
    const Json expected = { std::move( opStatus ) };

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_pulseFunctionOpeator ) {  // 1.7
    HubPlatform::Services::TaskLoop taskLoop;

    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto op = createOperator< PulseFunctionOperator >( taskLoop, std::chrono::seconds( 5 ), std::chrono::seconds( 5 ) );
    event->setOperator( op );

    setClockTime( std::chrono::minutes( 10 ) );
    storeBlockState( event->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    setClockTime( std::chrono::minutes( 30 ) );
    storeBlockState( op->getBlockID(), AbstractOperator::OperatorState::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    op->accept( visitor );

    Json opStatus = getOperatorStatus( *op );
    opStatus[ SceneStatusFields::Blocks ] = { getOperatorStatus( *event ) };
    const Json expected = { std::move( opStatus ) };

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_latchFunctionOpeator ) {  // 1.8
    using namespace SceneStatusFields;

    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto op = createOperator< LatchFunctionOperator >();
    event->setOperator( op );

    op->closeLatch();

    storeBlockState( event->getOperator()->getBlockID(), AbstractOperator::True );
    storeBlockState( op->getBlockID(), AbstractOperator::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    op->accept( visitor );

    Json opStatus = getOperatorStatus( *op );
    opStatus[ Blocks ] = { getOperatorStatus( *event ) };
    opStatus[ Function ][ IsLatched ] = true;
    const Json expected = { std::move( opStatus ) };

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_complexCombinationOfDifferentBlocks ) {  // 1.9
    auto event1 = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createTrigger< GroupStateEvent >( "", "", GroupStateEvent::State::True );
    auto event3 = createTrigger< SceneStateEvent >( "", SceneStateEvent::Action::AnyResult );
    auto op1 = createOperator< OrOperator >();
    event1->setOperator( op1 );
    event2->setOperator( op1 );
    event3->setOperator( op1 );

    auto event4 = createTrigger< ItemStateChangedEvent >( ItemValueProviderPtr{}, ItemValueProviderPtr{} );
    auto op2 = createOperator< AndOperator >();
    event4->setOperator( op2 );

    auto event5 = createTrigger< SceneStateEvent >( "", SceneStateEvent::Action::AnyResult );

    storeBlockState( event1->getOperator()->getBlockID(), AbstractOperator::OperatorState::Disabled );
    storeBlockState( event2->getOperator()->getBlockID(), AbstractOperator::OperatorState::False );
    storeBlockState( event3->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    storeBlockState( event4->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    storeBlockState( event5->getOperator()->getBlockID(), AbstractOperator::OperatorState::True );
    storeBlockState( op1->getBlockID(), AbstractOperator::OperatorState::True );
    storeBlockState( op2->getBlockID(), AbstractOperator::OperatorState::True );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    op1->accept( visitor );
    op2->accept( visitor );
    event5->getOperator()->accept( visitor );

    Json expected = JSON_EMPTY_ARR;
    {
        Json expectedOp1Status = getOperatorStatus( *op1 );
        expectedOp1Status[ SceneStatusFields::Blocks ] = {
            getOperatorStatus( *event1 ), getOperatorStatus( *event2 ), getOperatorStatus( *event3 )
        };
        expected.push_back( std::move( expectedOp1Status ) );
    }
    {
        Json expectedOp2Status = getOperatorStatus( *op2 );
        expectedOp2Status[ SceneStatusFields::Blocks ] = { getOperatorStatus( *event4 ) };
        expected.push_back( std::move( expectedOp2Status ) );
    }
    expected.push_back( getOperatorStatus( *event5 ) );

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, operatorStatusVisitor_corruptedOperatorStateInDb ) {  // 1.10
    using namespace StateRegistryDbFields;

    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );

    const std::chrono::minutes time( 10 );
    const std::string& blockId = event->getOperator()->getBlockID();
    setClockTime( time );
    storeBlockState( blockId, AbstractOperator::OperatorState::True );

    const StatesRegistry::TimePoint timePoint( time );
    const Json dbRecord = JSON( Id << blockId << SceneId << m_testSceneId << State << "corrupted" << LastChangeTime
                                   << timePoint.time_since_epoch().count() );
    dbClient().insert( HaTempSceneBlockStatesDBName, dbRecord );

    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    event->getOperator()->accept( visitor );

    Json opStatus;
    opStatus[ SceneStatusFields::Id ] = blockId;
    opStatus[ SceneStatusFields::Name ] = event->getMethodName();
    opStatus[ SceneStatusFields::State ] = SceneStatusFields::UnknownState;
    opStatus[ SceneStatusFields::LastChange ] = Json();
    const Json expected = { std::move( opStatus ) };

    const Json& status = visitor.getStatus();
    EXPECT_EQ( status, expected );
}

TEST_F( SceneStatusGetRequestTest, sceneWasntExecuted ) {  // 2.1
    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, {}, {}, {}, Json() );

    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 Json(),
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithoutEvents ) {  // 2.2
    const auto state = AbstractOperator::OperatorState::True;
    setClockTime( std::chrono::minutes( 10 ) );
    storeSceneState( state );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, {}, {}, {}, Json() );

    const auto sceneState = readSceneState();
    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 Json(),
                                                 sceneState.state,
                                                 sceneState.lastChangeTime );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithTriggers ) {  // 2.3
    auto event = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    OperatorStatusVisitor visitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    event->getOperator()->accept( visitor );

    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 visitor.getStatus(),
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 Json(),
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, triggersMap(), exceptionsMap(), {}, Json() );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithExceptions ) {  // 2.4
    auto event = createException< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    OperatorStatusVisitor visitor( m_testSceneId, exceptionsMap(), *m_statesRegistry );
    event->getOperator()->accept( visitor );

    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 JSON_EMPTY_ARR,
                                                 visitor.getStatus(),
                                                 JSON_EMPTY_ARR,
                                                 Json(),
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, triggersMap(), exceptionsMap(), {}, Json() );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithExceptionsAndTriggers ) {  // 2.5
    CompareNumbersEvent::Comparator comparator;
    auto trigger = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), comparator );
    auto exception = createException< CompareNumbersEvent >( ItemValueWrapper(), comparator );

    OperatorStatusVisitor triggersVisitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    trigger->getOperator()->accept( triggersVisitor );

    OperatorStatusVisitor exceptionVisitor( m_testSceneId, exceptionsMap(), *m_statesRegistry );
    exception->getOperator()->accept( exceptionVisitor );

    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 triggersVisitor.getStatus(),
                                                 exceptionVisitor.getStatus(),
                                                 JSON_EMPTY_ARR,
                                                 Json(),
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, triggersMap(), exceptionsMap(), {}, Json() );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithError ) {  // 2.6
    const Json error = "some_error";
    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 error,
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, triggersMap(), exceptionsMap(), {}, error );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithGlobalRestriction ) {  // 2.7
    auto event =
        createGlobalRestriction< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    OperatorStatusVisitor visitor( m_testSceneId, globalRestrictionMap(), *m_statesRegistry );
    event->getOperator()->accept( visitor );

    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 JSON_EMPTY_ARR,
                                                 JSON_EMPTY_ARR,
                                                 visitor.getStatus(),
                                                 Json(),
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, triggersMap(), exceptionsMap(), globalRestrictionMap(), Json() );

    EXPECT_EQ( expected, status );
}

TEST_F( SceneStatusGetRequestTest, sceneWithExceptionsTriggersAndGlobalRestriction ) {  // 2.8
    CompareNumbersEvent::Comparator comparator;
    auto trigger = createTrigger< CompareNumbersEvent >( ItemValueWrapper(), comparator );
    auto exception = createException< CompareNumbersEvent >( ItemValueWrapper(), comparator );
    auto globalRestriction = createGlobalRestriction< CompareNumbersEvent >( ItemValueWrapper(), comparator );

    OperatorStatusVisitor triggersVisitor( m_testSceneId, triggersMap(), *m_statesRegistry );
    trigger->getOperator()->accept( triggersVisitor );

    OperatorStatusVisitor exceptionVisitor( m_testSceneId, exceptionsMap(), *m_statesRegistry );
    exception->getOperator()->accept( exceptionVisitor );

    OperatorStatusVisitor globalRestrictionVisitor( m_testSceneId, globalRestrictionMap(), *m_statesRegistry );
    globalRestriction->getOperator()->accept( globalRestrictionVisitor );

    const Json expected = createSceneStatusJson( m_testSceneId,
                                                 triggersVisitor.getStatus(),
                                                 exceptionVisitor.getStatus(),
                                                 globalRestrictionVisitor.getStatus(),
                                                 Json(),
                                                 AbstractOperator::OperatorState::False,
                                                 StatesRegistry::TimePoint() );

    SceneStatusBuilder builder( *m_statesRegistry );
    const Json status = builder.build( m_testSceneId, triggersMap(), exceptionsMap(), globalRestrictionMap(), Json() );

    EXPECT_EQ( expected, status );
}

}  // namespace UT
}  // namespace SceneEngine
