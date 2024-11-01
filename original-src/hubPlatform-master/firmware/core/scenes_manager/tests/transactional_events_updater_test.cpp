#include "scenes_manager/scenes_engine/operators/operators.h"

#include "scenes_manager/scenes_engine/events/transactional_events_updater.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"
#include "scenes_manager/scenes_engine/events/string_operation_event.h"

#include "service_interfaces/tests/task_loop_test_helper.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. Single event
DONE        1.1 trigger
DONE        1.2 reset
DONE        1.3 reset silent
DONE        1.4 disabled event
DONE    2. Multiple events
DONE        2.1 triggered
DONE        2.2 reset
DONE        2.3 reset silent
DONE        2.4 disabled events
DONE        2.5 mixed case
DONE    3. Events with parent operators
DONE        3.1 2 events with AND parent
DONE        3.2 2 events with OR parent
DONE        3.3 event with FOLLOW function parent

*/

class TransactionalEventsUpdaterTest : public testing::Test {
public:
    enum class StateKind { Scene, Operator };

    using OperatorState = AbstractOperator::OperatorState;
    using SharedOperator = AbstractOperator::SharedOperator;

    using StateEventEntry = std::tuple< std::weak_ptr< AbstractOperator >, StateKind, OperatorState >;
    using StateEvents = std::vector< StateEventEntry >;

public:
    template < typename T, typename... Args >
    std::shared_ptr< T > createOperator( Args&&... args );
    template < typename T, typename... Args >
    std::shared_ptr< T > createEvent( Args&&... args );

    void subscribeToStateEvents( const SharedOperator& op );
    void compareStateEvents( const StateEvents& expected ) const;

    static bool getEventOperatorInternalState( const EventBase& event );
    static StateEventEntry makeStateEntry( const EventBase& event, StateKind kind, OperatorState state );

public:
    StateEvents m_stateEvents;
};

template < typename T, typename... Args >
inline std::shared_ptr< T > TransactionalEventsUpdaterTest::createOperator( Args&&... args ) {
    return std::make_shared< T >( std::forward< Args >( args )... );
}

template < typename T, typename... Args >
inline std::shared_ptr< T > TransactionalEventsUpdaterTest::createEvent( Args&&... args ) {
    static_assert( std::is_base_of< EventBase, T >::value, "Event type expected" );

    auto result = std::make_shared< T >( std::forward< Args >( args )... );
    subscribeToStateEvents( result->getOperator() );

    return result;
}

void TransactionalEventsUpdaterTest::subscribeToStateEvents( const SharedOperator& op ) {
    auto weakOp = std::weak_ptr< AbstractOperator >( op );
    op->setSceneStateEventHandler( [this, weakOp]( OperatorState newState ) {
        m_stateEvents.push_back( StateEventEntry{ weakOp, StateKind::Scene, newState } );
    } );
    op->setOperatorStateEventHandler( [this, weakOp]( OperatorState newState ) {
        m_stateEvents.push_back( StateEventEntry{ weakOp, StateKind::Operator, newState } );
    } );
}

void TransactionalEventsUpdaterTest::compareStateEvents( const StateEvents& expected ) const {
    ASSERT_EQ( m_stateEvents.size(), expected.size() );
    for( size_t i = 0; i < m_stateEvents.size(); ++i ) {
        const StateEventEntry& actualEntry = m_stateEvents[ i ];
        const StateEventEntry& expectedEntry = expected[ i ];

        SharedOperator actualOperator = std::get< 0 >( actualEntry ).lock();
        SharedOperator expectedOperator = std::get< 0 >( expectedEntry ).lock();
        EXPECT_NE( actualOperator, nullptr ) << "i = " + std::to_string( i );
        EXPECT_NE( expectedOperator, nullptr ) << "i = " + std::to_string( i );
        EXPECT_EQ( actualOperator, expectedOperator ) << "i = " + std::to_string( i );

        EXPECT_EQ( std::get< 1 >( actualEntry ), std::get< 1 >( expectedEntry ) ) << "i = " + std::to_string( i );
        EXPECT_EQ( std::get< 2 >( actualEntry ), std::get< 2 >( expectedEntry ) ) << "i = " + std::to_string( i );
    }
}

bool TransactionalEventsUpdaterTest::getEventOperatorInternalState( const EventBase& event ) {
    auto eventOp = std::dynamic_pointer_cast< EventOperator >( event.getOperator() );
    if( !eventOp )
        throw std::logic_error( "Event are expected to hold an instance of EventOperator" );

    return eventOp->m_state;
}

TransactionalEventsUpdaterTest::StateEventEntry TransactionalEventsUpdaterTest::makeStateEntry( const EventBase& event,
                                                                                                StateKind kind,
                                                                                                OperatorState state ) {
    auto weakOperator = std::weak_ptr< AbstractOperator >( event.getOperator() );
    return StateEventEntry{ weakOperator, kind, state };
}

TEST_F( TransactionalEventsUpdaterTest, singleEventTriggered ) {  // 1.1
    auto event = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );

    TransactionalEventsUpdater updater;
    updater.requestTrigger( event );
    updater.flush();

    compareStateEvents( { makeStateEntry( *event, StateKind::Operator, OperatorState::True ),
                          makeStateEntry( *event, StateKind::Scene, OperatorState::True ) } );
}

TEST_F( TransactionalEventsUpdaterTest, singleEventReset ) {  // 1.2
    auto event = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );

    TransactionalEventsUpdater updater;
    updater.requestReset( event );
    updater.flush();

    compareStateEvents( { makeStateEntry( *event, StateKind::Operator, OperatorState::False ),
                          makeStateEntry( *event, StateKind::Scene, OperatorState::False ) } );
}

TEST_F( TransactionalEventsUpdaterTest, singleEventResetSilent ) {  // 1.3
    auto event = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    event->setOperatorState( true );

    TransactionalEventsUpdater updater;
    updater.requestResetSilent( event );
    updater.flush();

    compareStateEvents( { makeStateEntry( *event, StateKind::Operator, OperatorState::False ) } );
    EXPECT_EQ( getEventOperatorInternalState( *event ), false );
}

TEST_F( TransactionalEventsUpdaterTest, singleDisabledEvent ) {  // 1.4
    auto event = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    event->setEnabled( false );

    TransactionalEventsUpdater updater;
    updater.requestTrigger( event );
    updater.flush();

    ASSERT_TRUE( m_stateEvents.empty() );
    EXPECT_EQ( event->getOperator()->checkChildren(), OperatorState::Disabled );
    EXPECT_EQ( getEventOperatorInternalState( *event ), true );
}

TEST_F( TransactionalEventsUpdaterTest, multipleEventsTriggered ) {  // 2.1
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< StringOperationEvent >( ItemValueWrapper( Json( 3 ) ), "length" );
    auto event3 = createEvent< ItemEvent >();

    TransactionalEventsUpdater updater;
    updater.requestTrigger( event1 );
    updater.requestTrigger( event2 );
    updater.requestTrigger( event3 );
    updater.flush();

    const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event3, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event1, StateKind::Scene, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Scene, OperatorState::True ),
                                   makeStateEntry( *event3, StateKind::Scene, OperatorState::True ) };
    compareStateEvents( expected );
}

TEST_F( TransactionalEventsUpdaterTest, multipleEventsReset ) {  // 2.2
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< StringOperationEvent >( ItemValueWrapper( Json( 3 ) ), "length" );
    auto event3 = createEvent< ItemEvent >();

    TransactionalEventsUpdater updater;
    updater.requestReset( event1 );
    updater.requestReset( event2 );
    updater.requestReset( event3 );
    updater.flush();

    const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::False ),
                                   makeStateEntry( *event2, StateKind::Operator, OperatorState::False ),
                                   makeStateEntry( *event3, StateKind::Operator, OperatorState::False ),
                                   makeStateEntry( *event1, StateKind::Scene, OperatorState::False ),
                                   makeStateEntry( *event2, StateKind::Scene, OperatorState::False ),
                                   makeStateEntry( *event3, StateKind::Scene, OperatorState::False ) };
    compareStateEvents( expected );
}

TEST_F( TransactionalEventsUpdaterTest, multipleEventsResetSilent ) {  // 2.3
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< StringOperationEvent >( ItemValueWrapper( Json( 3 ) ), "length" );
    auto event3 = createEvent< ItemEvent >();

    TransactionalEventsUpdater updater;
    for( auto& event : std::initializer_list< EventBasePtr >{ event1, event2, event3 } ) {
        event->setOperatorState( true );
        updater.requestResetSilent( event );
    }
    updater.flush();

    compareStateEvents( { makeStateEntry( *event1, StateKind::Operator, OperatorState::False ),
                          makeStateEntry( *event2, StateKind::Operator, OperatorState::False ),
                          makeStateEntry( *event3, StateKind::Operator, OperatorState::False ) } );
    EXPECT_EQ( getEventOperatorInternalState( *event1 ), false );
    EXPECT_EQ( getEventOperatorInternalState( *event2 ), false );
    EXPECT_EQ( getEventOperatorInternalState( *event3 ), false );
}

TEST_F( TransactionalEventsUpdaterTest, multipleEventsDisabeld ) {  // 2.4
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< StringOperationEvent >( ItemValueWrapper( Json( 3 ) ), "length" );
    auto event3 = createEvent< ItemEvent >();

    TransactionalEventsUpdater updater;
    for( auto& event : std::initializer_list< EventBasePtr >{ event1, event2, event3 } ) {
        event->setEnabled( false );
        updater.requestTrigger( event );
    }
    updater.flush();

    ASSERT_TRUE( m_stateEvents.empty() );
    for( auto& event : std::initializer_list< EventBasePtr >{ event1, event2, event3 } ) {
        EXPECT_EQ( event->getOperator()->checkChildren(), OperatorState::Disabled );
        EXPECT_EQ( getEventOperatorInternalState( *event ), true );
    }
}

TEST_F( TransactionalEventsUpdaterTest, multipleEventsMixed ) {  // 2.5
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< StringOperationEvent >( ItemValueWrapper( Json( 3 ) ), "length" );
    auto event3 = createEvent< ItemEvent >();

    TransactionalEventsUpdater updater;
    updater.requestTrigger( event1 );
    updater.requestReset( event2 );
    updater.requestResetSilent( event3 );
    updater.flush();

    const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Operator, OperatorState::False ),
                                   makeStateEntry( *event3, StateKind::Operator, OperatorState::False ),
                                   makeStateEntry( *event1, StateKind::Scene, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Scene, OperatorState::False ) };
    compareStateEvents( expected );
}

TEST_F( TransactionalEventsUpdaterTest, eventsWithAndParent ) {  // 3.1
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< ItemEvent >();
    auto parent = createOperator< AndOperator >();

    event1->setOperator( parent );
    event2->setOperator( parent );

    TransactionalEventsUpdater updater;
    updater.requestTrigger( event1 );
    updater.requestTrigger( event2 );
    updater.flush();

    const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event1, StateKind::Scene, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Scene, OperatorState::True ) };
    compareStateEvents( expected );
}

TEST_F( TransactionalEventsUpdaterTest, eventsWithOrParent ) {  // 3.2
    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< ItemEvent >();
    auto parent = createOperator< OrOperator >();

    event1->setOperator( parent );
    event2->setOperator( parent );

    TransactionalEventsUpdater updater;
    updater.requestTrigger( event1 );
    updater.requestTrigger( event2 );
    updater.flush();

    const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Operator, OperatorState::True ),
                                   makeStateEntry( *event1, StateKind::Scene, OperatorState::True ),
                                   makeStateEntry( *event2, StateKind::Scene, OperatorState::True ) };
    compareStateEvents( expected );
}

TEST_F( TransactionalEventsUpdaterTest, eventsWithFollowFunctionParent ) {  // 3.3
    HubPlatform::Services::TaskLoop taskLoop;
    HubPlatform::Services::UT::TaskLoopTestHelper taskLoopHelper;

    std::future< void > taskLoopFuture = taskLoopHelper.runInThread( taskLoop );
    taskLoopHelper.setTimeProviderForLoop( taskLoop );

    const std::chrono::seconds followDelay( 1 );
    auto parent = createOperator< FollowFunctionOperator >( taskLoop, followDelay );
    subscribeToStateEvents( parent );

    auto event1 = createEvent< CompareNumbersEvent >( ItemValueWrapper(), CompareNumbersEvent::Comparator() );
    auto event2 = createEvent< ItemEvent >();
    event1->setOperator( parent );
    event2->setOperator( parent );

    {
        TransactionalEventsUpdater updater;
        updater.requestTrigger( event1 );
        updater.requestTrigger( event2 );
        updater.flush();

        const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::True ),
                                       makeStateEntry( *event2, StateKind::Operator, OperatorState::True ),
                                       StateEventEntry{ parent, StateKind::Operator, OperatorState::True },
                                       StateEventEntry{ parent, StateKind::Scene, OperatorState::True } };
        compareStateEvents( expected );
        m_stateEvents.clear();
    }
    {
        TransactionalEventsUpdater updater;
        updater.requestReset( event1 );
        updater.requestReset( event2 );
        updater.flush();

        taskLoopHelper.sleep( taskLoop, followDelay );
        taskLoopHelper.waitForTasks( taskLoop );

        const StateEvents expected = { makeStateEntry( *event1, StateKind::Operator, OperatorState::False ),
                                       makeStateEntry( *event2, StateKind::Operator, OperatorState::False ),
                                       StateEventEntry{ parent, StateKind::Operator, OperatorState::False },
                                       StateEventEntry{ parent, StateKind::Scene, OperatorState::False } };
        compareStateEvents( expected );
    }

    taskLoop.stop();
    const std::future_status status = taskLoopFuture.wait_for( std::chrono::milliseconds( 100 ) );
    EXPECT_EQ( status, std::future_status::ready );
}

}  // namespace UT
}  // namespace SceneEngine
