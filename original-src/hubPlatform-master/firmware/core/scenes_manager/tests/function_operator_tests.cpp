#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/scenes_engine/operators/event_operator.h"
#include "scenes_manager/scenes_engine/operators/follow_function_operator.h"
#include "scenes_manager/scenes_engine/operators/pulse_function_operator.h"
#include "scenes_manager/scenes_engine/operators/operator_factory.h"
#include "scenes_manager/scenes_engine/exceptions.h"

#include "service_interfaces/tests/task_loop_test_helper.h"
#include "service_interfaces/task_loop.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "base_utils/utils.h"
#include "scenes_manager/scenes_engine/operators/latch_function_operator.h"
#include "scenes_manager/scenes_engine/operators/visitors/latch_visitor.h"
#include "scenes_manager/scenes_engine/operators/visitors/operator_name_visitor.h"
#include "scenes_manager/scenes_engine/operators/operators.h"
#include "database_client/dbid.h"
#include "system_utils/thread_unique_safe_wrapper.h"
#include <gtest/gtest.h>

#include <thread>
#include <utility>

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. FOR Function operator
DONE        1.1 Function FOR/LESS
DONE        1.2 Function FOR/AT_LEAST
DONE        1.3 Function FOR/LESS for multiple children
DONE        1.4 Function FOR/AT_LEAST for multiple children
DONE        1.5 Function FOR/AT_LEAST in composition: AND(FOR, OR, EVENT)

DONE    2. Function.Repeat tests
DONE        2.1 repeat, successful
DONE        2.2 repeat, discard old records, move repeat period window
DONE        2.2 repeat, too long intervals

DONE    3. Function.Follow
DONE        3.1 switch to "true" happens instantly
DONE        3.2 delayed reset with one child
DONE        3.3 delayed reset with three children
DONE        3.4 transition not triggered if not all of operator's children are in "true" state
DONE        3.5 invalid reset delay
DONE        3.6 reset is discarded if operator returned to "true" state
DONE        3.7 reset timer restarted if operator switched to "true" while a reset was scheduled

DONE    4. Function.Pulse
DONE        4.1 one transition to "true" (one-shot)
DONE        4.2 multiple transitions
DONE        4.3 invalid "truePeriod" param
DONE        4.4 invalid "falsePeriod" param
DONE        4.5 invalid "times" param
DONE        4.6 child reset when pulse was in "true" state
DONE        4.7 child changed state twice and returned to "true", when pulse was in "true" state
DONE        4.8 child changed state twice and returned to "true", when pulse was in "false" state
DONE        4.9 child changed state twice on last activation (counter not restarted)
DONE        4.10 child updated with "true" -> "true" transition
DONE        4.11 infinite pulses

DONE    5. Function.Latch
DONE        5.1 create and latch
DONE        5.2 unlatch using parent AND operator
DONE        5.3 LATCH / NOT / sensor

DONE    6. Operator Visitors
DONE        6.1 Latch Visitors
DONE        6.2 Operator Name Visitors
*/

constexpr std::chrono::milliseconds PulseTestSleepStep{ 100 };

class FunctionOperatorTests : public testing::Test, public HubPlatform::Services::UT::TaskLoopTestHelper {
public:
    using DelayedState = std::pair< AbstractOperator::OperatorState, Interval /* delay */ >;

    struct AutoStoppedTaskLoop {
        explicit AutoStoppedTaskLoop( FunctionOperatorTests& fixture );
        ~AutoStoppedTaskLoop();

        FunctionOperatorTests& m_fixture;
        HubPlatform::Services::TaskLoop m_taskLoop;
        std::future< void > m_taskLoopFuture;
    };

    struct OperatorStatesCollector {
        explicit OperatorStatesCollector( AbstractOperator& op, HubPlatform::Services::TaskLoop& taskLoop );

        HubPlatform::Services::TaskLoop& m_taskLoop;
        std::vector< DelayedState > m_states;
    };

public:
    static void SetUpTestCase();
    void sleepStepped( HubPlatform::Services::TaskLoop& taskLoop,
                       std::chrono::milliseconds total,
                       std::chrono::milliseconds step );
    void sleepAndSync( HubPlatform::Services::TaskLoop& taskLoop, std::chrono::milliseconds sleepDuration );

    static void checkFollowDelayedReset( const OperatorStatesCollector& statesCollector, std::chrono::seconds delay );
    static void checkPulseTransitions( const OperatorStatesCollector& statesCollector,
                                       std::chrono::seconds truePeriod,
                                       std::chrono::seconds falsePeriod,
                                       int times );

    template < typename T, typename... Args >
    static void checkOperatorConstructionFails( const std::string& message, Args&&... args );

    static std::shared_ptr< EventOperator > createEventOperator( const AbstractOperator::SharedOperator& parent,
                                                                 bool disableSceneState = false );
    static std::shared_ptr< AndOperator > createAndOperator( AbstractOperator::SharedOperator parent );
    static AbstractOperator::SharedOperator createNotOperator( AbstractOperator::SharedOperator parent );
    static std::shared_ptr< LatchFunctionOperator > createLatchOperator( AbstractOperator::SharedOperator parent );

    static void setBlockId( AbstractOperator* op );
    static bool evaluateSubtree( EventOperator& op, bool state );
};

FunctionOperatorTests::AutoStoppedTaskLoop::AutoStoppedTaskLoop( FunctionOperatorTests& fixture )
: m_fixture( fixture ) {
    m_fixture.setTimeProviderForLoop( m_taskLoop );
    m_taskLoopFuture = m_fixture.runInThread( m_taskLoop );
}

FunctionOperatorTests::AutoStoppedTaskLoop::~AutoStoppedTaskLoop() {
    m_taskLoop.stop();
    const std::future_status status = m_taskLoopFuture.wait_for( std::chrono::seconds( 1 ) );
    EXPECT_EQ( status, std::future_status::ready );
}

FunctionOperatorTests::OperatorStatesCollector::OperatorStatesCollector( AbstractOperator& op,
                                                                         HubPlatform::Services::TaskLoop& taskLoop )
: m_taskLoop( taskLoop ) {
    op.setSceneStateEventHandler( [this]( AbstractOperator::OperatorState state ) {
        const Interval timePassed = getTimeFromProvider( m_taskLoop ).time_since_epoch();
        m_states.emplace_back( state, timePassed );
    } );
}

void FunctionOperatorTests::sleepStepped( HubPlatform::Services::TaskLoop& taskLoop,
                                          std::chrono::milliseconds total,
                                          std::chrono::milliseconds step ) {
    std::chrono::milliseconds timePassed( 0 );
    while( timePassed < total ) {
        sleepAndSync( taskLoop, step );
        timePassed += step;
    }
}

void FunctionOperatorTests::sleepAndSync( HubPlatform::Services::TaskLoop& taskLoop,
                                          std::chrono::milliseconds sleepDuration ) {
    sleep( taskLoop, sleepDuration );
    waitForTasks( taskLoop );
}

template < typename T, typename... Args >
void FunctionOperatorTests::checkOperatorConstructionFails( const std::string& expectedMessage, Args&&... args ) {
    try {
        std::make_shared< T >( std::forward< Args >( args )... );
        ADD_FAILURE() << "Expected 'SCENE_WRONG_WHEN_BLOCK' exception to be thrown, but it wasn't";
    }
    catch( const SceneEngine::SceneException& ex ) {
        EXPECT_EQ( ex.what(), expectedMessage );
    }
    catch( ... ) {
        ADD_FAILURE() << "Expected 'SCENE_WRONG_WHEN_BLOCK' exception to be thrown, but it wasn't";
    }
}

void FunctionOperatorTests::checkFollowDelayedReset( const OperatorStatesCollector& statesCollector,
                                                     std::chrono::seconds delay ) {
    EXPECT_EQ( statesCollector.m_states.size(), 1 );
    if( statesCollector.m_states.size() != 1 )
        return;

    DelayedState expected( AbstractOperator::OperatorState::False, delay );
    EXPECT_EQ( statesCollector.m_states.front(), expected );
}

void FunctionOperatorTests::checkPulseTransitions( const OperatorStatesCollector& statesCollector,
                                                   std::chrono::seconds truePeriod,
                                                   std::chrono::seconds falsePeriod,
                                                   int times ) {
    const auto& delayedStates = statesCollector.m_states;
    EXPECT_EQ( delayedStates.size(), times * 2 );  // each time there are 2 transitions: FALSE -> TRUE, TRUE -> FALSE
    if( delayedStates.size() != times * 2 )
        return;

    std::chrono::seconds checkTime( 0 );
    for( size_t i = 0; i < delayedStates.size(); i += 2 ) {
        const DelayedState& transitionTrue = delayedStates[ i ];
        EXPECT_EQ( transitionTrue.first, AbstractOperator::OperatorState::True );
        EXPECT_EQ( transitionTrue.second, checkTime );
        checkTime += truePeriod;

        const DelayedState& transitionFalse = delayedStates[ i + 1 ];
        EXPECT_EQ( transitionFalse.first, AbstractOperator::OperatorState::False );
        EXPECT_EQ( transitionFalse.second, checkTime );
        checkTime += falsePeriod;
    }
}

std::shared_ptr< EventOperator > FunctionOperatorTests::createEventOperator(
    const AbstractOperator::SharedOperator& parent,
    bool disableSceneState ) {
    auto eventOperator = std::make_shared< EventOperator >();
    setBlockId( eventOperator.get() );
    if( parent.get() != nullptr ) {
        eventOperator->setParent( parent );

        if( disableSceneState ) {
            EXPECT_TRUE( eventOperator->isSceneStateEventHandlerDisabled() )
                << "Should delegate scene state management to a parent operator";
            if( !eventOperator->isSceneStateEventHandlerDisabled() )
                throw std::logic_error( "Function's children should have state event handler disabled" );
        }
    }

    return eventOperator;
}

std::shared_ptr< AndOperator > FunctionOperatorTests::createAndOperator( AbstractOperator::SharedOperator parent ) {
    auto op = std::make_shared< AndOperator >();
    setBlockId( op.get() );
    if( parent )
        op->setParent( std::move( parent ) );
    return op;
}

void FunctionOperatorTests::setBlockId( AbstractOperator* op ) {
    if( op ) {
        auto blockId = db::DbId::genOID();
        op->setBlockID( blockId );
    }
}

bool FunctionOperatorTests::evaluateSubtree( EventOperator& op, bool state ) {
    op.setState( state );
    op.notifyOperatorState();
    auto subtreeState = op.evaluateSubtree();
    return AbstractOperator::toBool( subtreeState );
}

std::shared_ptr< LatchFunctionOperator > FunctionOperatorTests::createLatchOperator(
    AbstractOperator::SharedOperator parent ) {
    auto op = std::make_shared< LatchFunctionOperator >( true );
    setBlockId( op.get() );
    if( parent )
        op->setParent( std::move( parent ) );
    return op;
}

AbstractOperator::SharedOperator FunctionOperatorTests::createNotOperator( AbstractOperator::SharedOperator parent ) {
    auto op = OperatorFactory::makeNotOperator();
    setBlockId( op.get() );
    op->setParent( std::move( parent ) );
    return op;
}

void FunctionOperatorTests::SetUpTestCase() {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

TEST_F( FunctionOperatorTests, forLess ) {  // 1.1
    HubPlatform::Services::TaskLoop taskLoop;

    const auto functionWaitDuration = std::chrono::milliseconds( 150 );
    const auto shortWait = std::chrono::milliseconds( 50 );
    const auto longWait = std::chrono::milliseconds( 200 );

    auto forOperator = OperatorFactory::makeForFunctionOperator( "less", functionWaitDuration, taskLoop );
    auto eventOperator1 = createEventOperator( forOperator );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    std::this_thread::sleep_for( shortWait );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) )
        << "transition in less than configured duration, but checkCondition result should be false "
        << " plus notifySceneStateChanged( true )";

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    std::this_thread::sleep_for( longWait );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) )
        << "transition in more than configured duration second - should be false";
}

TEST_F( FunctionOperatorTests, forAtLeast ) {  // 1.2
    HubPlatform::Services::TaskLoop taskLoop;

    auto future = runInThread( taskLoop );

    const std::chrono::milliseconds forFunctionTarget{ 200 };
    const std::chrono::milliseconds smallTransition{ 50 };
    const std::chrono::milliseconds longTransition{ 300 };

    auto forOperator = OperatorFactory::makeForFunctionOperator( "least", forFunctionTarget, taskLoop );
    auto eventOperator1 = createEventOperator( forOperator );

    // only one child needed
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    std::this_thread::sleep_for( smallTransition );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) ) << "transition in less than 200 ms - false";

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    // transition in 200 ms - true; notified indirectly via a callback
    std::this_thread::sleep_for( longTransition );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) )
        << "transition in more than 200 ms: true c=via callback -> false at this point";

    taskLoop.stop();

    auto futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
    ASSERT_EQ( futureStatus, std::future_status::ready ) << "Should finish the async operation in a reasonable time";
}

TEST_F( FunctionOperatorTests, forLessWithMultipleChildren ) {  // 1.3
    HubPlatform::Services::TaskLoop taskLoop;

    const auto functionWaitDuration = std::chrono::milliseconds( 150 );
    const auto shortWait = std::chrono::milliseconds( 50 );
    const auto longWait = std::chrono::milliseconds( 200 );

    auto forOperator = OperatorFactory::makeForFunctionOperator( "less", functionWaitDuration, taskLoop );

    auto eventOperator1 = createEventOperator( forOperator );
    auto eventOperator2 = createEventOperator( forOperator );

    std::vector< AbstractOperator::OperatorState > sceneStateChanges;
    forOperator->setSceneStateEventHandler( [&]( AbstractOperator::OperatorState state ) {
        Log::trace(
            cc::trace( "forOperator setSceneState event handler: " + cc::flag( AbstractOperator::toBool( state ) ) ) );
        sceneStateChanges.push_back( state );
    } );

    {
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
        EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
        std::this_thread::sleep_for( shortWait );
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) )
            << "transition in less than configured duration, but checkCondition result should be false "
            << " plus notifySceneStateChanged( true )";
        ASSERT_EQ( sceneStateChanges.size(), 2 ) << "Should be two transitions";
        ASSERT_TRUE( AbstractOperator::toBool( sceneStateChanges[ 0 ] ) );
        ASSERT_FALSE( AbstractOperator::toBool( sceneStateChanges[ 1 ] ) );
        sceneStateChanges.clear();
    }

    {
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
        EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
        std::this_thread::sleep_for( longWait );
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) )
            << "transition in more than configured duration second - should be false";

        ASSERT_EQ( sceneStateChanges.size(), 0 ) << "No new transitions expected transitions";
    }
}

TEST_F( FunctionOperatorTests, forAtLeastWithMultipleChildren ) {  // 1.4
    HubPlatform::Services::TaskLoop taskLoop;

    auto future = runInThread( taskLoop );
    utils::AfterReturn onExit( [&]() {
        Log::trace( cc::trace( "clean-up on exit: stop task loop" ) );
        taskLoop.stop();

        Log::trace( cc::trace( "wait for a thread" ) );
        auto futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
        Log::trace( cc::trace( "wait completed" ) );
        ASSERT_EQ( futureStatus, std::future_status::ready )
            << "Should finish the async operation in a reasonable time";
    } );

    const std::chrono::milliseconds forFunctionTarget{ 200 };
    const std::chrono::milliseconds shortWait{ 50 };
    const std::chrono::milliseconds longWait{ 300 };

    auto forOperator = OperatorFactory::makeForFunctionOperator( "least", forFunctionTarget, taskLoop );
    auto eventOperator1 = createEventOperator( forOperator );
    auto eventOperator2 = createEventOperator( forOperator );

    std::vector< AbstractOperator::OperatorState > sceneStateChanges;

    AbstractOperator::OperatorState operatorState = AbstractOperator::OperatorState::False;
    HubPlatform::threads::UniqueSafeWrapper< AbstractOperator::OperatorState > forOperatorState(
        std::move( operatorState ) );

    forOperator->setSceneStateEventHandler( [&]( AbstractOperator::OperatorState state ) {
        Log::trace(
            cc::trace( "forOperator setSceneState event handler: " + cc::flag( AbstractOperator::toBool( state ) ) ) );
        if( AbstractOperator::toBool( state ) ) {
            Log::trace( "Log only True states" );
            sceneStateChanges.push_back( state );
        }
    } );
    forOperator->setOperatorStateEventHandler( [&]( AbstractOperator::OperatorState state ) {
        Log::trace( cc::trace( "forOperator setOperatorState event handler: " +
                               cc::flag( AbstractOperator::toBool( state ) ) ) );
        forOperatorState.set( state );
    } );

    auto fetchForOperatorState = [&forOperatorState]() -> bool {
        AbstractOperator::OperatorState state;
        forOperatorState.get( state );
        return AbstractOperator::toBool( state );
    };
    {
        EXPECT_FALSE( fetchForOperatorState() );
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
        EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
        EXPECT_FALSE( fetchForOperatorState() );
        std::this_thread::sleep_for( shortWait );
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
        EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
        EXPECT_FALSE( fetchForOperatorState() );

        ASSERT_EQ( sceneStateChanges.size(), 0 ) << "Should be no transitions";
        LOGGER_TRACE( cc::trace( "point 1, sceneStateChanges: " ) + cc::num10( sceneStateChanges.size() ) );
    }

    {
        EXPECT_FALSE( fetchForOperatorState() );
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
        EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
        EXPECT_FALSE( fetchForOperatorState() );
        std::this_thread::sleep_for( longWait );
        EXPECT_TRUE( fetchForOperatorState() ) << "for/least operator state should be true";

        ASSERT_EQ( sceneStateChanges.size(), 1 ) << "Scene state transition False -> True";
        ASSERT_TRUE( AbstractOperator::toBool( sceneStateChanges[ 0 ] ) );

        // make child operator false, FOR/AT_LEAST function should change to false state
        EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
        LOGGER_TRACE( cc::trace( "point 2, sceneStateChanges: " ) + cc::num10( sceneStateChanges.size() ) );
        EXPECT_FALSE( fetchForOperatorState() ) << "for/least operator state should be false";
    }

    {
        EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
        EXPECT_FALSE( fetchForOperatorState() ) << "for/least operator state should be false";
    }
}

/** Scene:
 *  AND:
 *      FOR/AT_LEAST * compareNumbers(item1)
 *      OR:
 *          compareNumbers(item2);
 *          compareNumbers(item3);
 *      compareNumbers(item4)
 *
 *  Bug: Scene started multiple times.
 *  Expected: scene should start only after the timer expired.
 * */
TEST_F( FunctionOperatorTests, forAtLeastInComposition ) {  // 1.5
    HubPlatform::Services::TaskLoop taskLoop;

    auto future = runInThread( taskLoop );
    utils::AfterReturn onExit( [&]() {
        Log::trace( cc::trace( "clean-up on exit: stop task loop" ) );
        taskLoop.stop();

        Log::trace( cc::trace( "wait for a thread" ) );
        auto futureStatus = future.wait_for( std::chrono::seconds( 5 ) );
        Log::trace( cc::trace( "wait completed" ) );
        ASSERT_EQ( futureStatus, std::future_status::ready )
            << "Should finish the async operation in a reasonable time";
    } );

    const std::chrono::milliseconds forFunctionTarget{ 200 };
    const std::chrono::milliseconds shortWait{ 50 };
    const std::chrono::milliseconds longWait{ 300 };

    // Init:

    auto rootAndOperator = OperatorFactory::makeAndOperator();
    rootAndOperator->setBlockID( "rootAnd" );

    auto forOperator = OperatorFactory::makeForFunctionOperator( "least", forFunctionTarget, taskLoop );
    forOperator->setBlockID( "for/at_least" );
    auto eventOperator1 = createEventOperator( forOperator );
    eventOperator1->setBlockID( "event1" );

    forOperator->setParent( rootAndOperator );

    auto orOperator = OperatorFactory::makeOrOperator();
    orOperator->setBlockID( "or" );
    auto eventOperator2 = createEventOperator( orOperator );
    eventOperator2->setBlockID( "event2" );
    auto eventOperator3 = createEventOperator( orOperator );
    eventOperator3->setBlockID( "event3" );

    orOperator->setParent( rootAndOperator );

    auto eventOperator4 = createEventOperator( rootAndOperator );
    eventOperator4->setBlockID( "event4" );

    // Prepare links:
    std::vector< AbstractOperator::OperatorState > sceneStateChanges;

    AbstractOperator::OperatorState operatorState = AbstractOperator::OperatorState::False;
    HubPlatform::threads::UniqueSafeWrapper< AbstractOperator::OperatorState > forOperatorState(
        AbstractOperator::OperatorState::False );
    HubPlatform::threads::UniqueSafeWrapper< AbstractOperator::OperatorState > orOperatorState(
        AbstractOperator::OperatorState::False );
    HubPlatform::threads::UniqueSafeWrapper< AbstractOperator::OperatorState > rootAndOperatorState(
        AbstractOperator::OperatorState::False );

    forOperator->setSceneStateEventHandler( [&]( AbstractOperator::OperatorState state ) {
        Log::trace(
            cc::trace( "forOperator setSceneState event handler: " + cc::flag( AbstractOperator::toBool( state ) ) ) );
        if( AbstractOperator::toBool( state ) ) {
            Log::trace( "Log only True states" );
            sceneStateChanges.push_back( state );
        }
    } );
    forOperator->setOperatorStateEventHandler( [&]( AbstractOperator::OperatorState state ) {
        Log::trace( cc::trace( "forOperator setOperatorState event handler: " +
                               cc::flag( AbstractOperator::toBool( state ) ) ) );
        forOperatorState.set( state );
    } );

    orOperator->setSceneStateEventHandler( [&sceneStateChanges, orOperator]( AbstractOperator::OperatorState state ) {
        Log::trace(
            cc::trace( "forOperator setSceneState event handler: " + cc::flag( AbstractOperator::toBool( state ) ) ) );
        if( AbstractOperator::toBool( state ) ) {
            Log::trace( "Log only True states" );
            sceneStateChanges.push_back( state );
        }
    } );
    orOperator->setOperatorStateEventHandler( [orOperator, &orOperatorState]( AbstractOperator::OperatorState state ) {
        Log::trace( cc::trace( "orOperator setOperatorState event handler: " +
                               cc::flag( AbstractOperator::toBool( state ) ) ) );
        orOperatorState.set( state );
    } );

    rootAndOperator->setSceneStateEventHandler( [&sceneStateChanges]( AbstractOperator::OperatorState state ) {
        Log::trace( cc::trace( "rootAndOperator setSceneState event handler: " +
                               cc::flag( AbstractOperator::toBool( state ) ) ) );
        if( AbstractOperator::toBool( state ) ) {
            Log::trace( "Log only True states" );
            sceneStateChanges.push_back( state );
        }
    } );
    rootAndOperator->setOperatorStateEventHandler( [&]( AbstractOperator::OperatorState state ) {
        Log::trace( cc::trace( "rootAndOperator setOperatorState event handler: " +
                               cc::flag( AbstractOperator::toBool( state ) ) ) );
        rootAndOperatorState.set( state );
    } );

    auto fetchForOperatorState = [&forOperatorState]() -> bool {
        AbstractOperator::OperatorState state;
        forOperatorState.get( state );
        return AbstractOperator::toBool( state );
    };
    auto fetchAndOperatorState = [&rootAndOperatorState]() -> bool {
        AbstractOperator::OperatorState state;
        rootAndOperatorState.get( state );
        return AbstractOperator::toBool( state );
    };

    // Test:
    // Step 1: set event operators: 4, 3, 2 to true; only eventOperator1 is false, and FOR/AT_LEAST ARE FALSE.
    EXPECT_FALSE( evaluateSubtree( *eventOperator4, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    // Step 2: set eventOperator1 to true.
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    std::this_thread::sleep_for( longWait );

    // Check: scene run immediately (bug)
    // Check: scene run after the forOperator period expires (expected)
    EXPECT_TRUE( fetchForOperatorState() ) << "for/least operator state should be true";
    EXPECT_TRUE( fetchAndOperatorState() ) << "AND operator state should be true";

    ASSERT_EQ( sceneStateChanges.size(), 1 ) << "Scene state transitions False -> True: only once";
    ASSERT_TRUE( AbstractOperator::toBool( sceneStateChanges[ 0 ] ) );
}

TEST_F( FunctionOperatorTests, repeat ) {  // 2.1
    const auto functionWaitDuration = std::chrono::milliseconds( 150 );
    const auto shortWait = std::chrono::milliseconds( 50 );
    const auto longWait = std::chrono::milliseconds( 200 );

    auto forOperator = OperatorFactory::makeRepeatFunctionOperator( 3, functionWaitDuration );
    auto eventOperator1 = createEventOperator( forOperator );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    std::this_thread::sleep_for( shortWait );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    std::this_thread::sleep_for( shortWait );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
}

TEST_F( FunctionOperatorTests, repeatLongWait ) {  // 2.2
    const auto functionWaitDuration = std::chrono::milliseconds( 150 );
    const auto shortWait = std::chrono::milliseconds( 50 );
    const auto longWait = std::chrono::milliseconds( 200 );

    auto forOperator = OperatorFactory::makeRepeatFunctionOperator( 3, functionWaitDuration );
    auto eventOperator1 = createEventOperator( forOperator );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );

    std::this_thread::sleep_for( longWait );

    // here a window should be moved to a transition number 3, numbers 1 and 2 should be dropped
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    std::this_thread::sleep_for( shortWait );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    std::this_thread::sleep_for( shortWait );
    EXPECT_TRUE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
}

TEST_F( FunctionOperatorTests, repeatFailed ) {  // 2.3
    const auto functionWaitDuration = std::chrono::milliseconds( 150 );
    const auto shortWait = std::chrono::milliseconds( 50 );
    const auto longWait = std::chrono::milliseconds( 200 );

    auto forOperator = OperatorFactory::makeRepeatFunctionOperator( 3, functionWaitDuration );
    auto eventOperator1 = createEventOperator( forOperator );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    std::this_thread::sleep_for( longWait );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    std::this_thread::sleep_for( longWait );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
}

TEST_F( FunctionOperatorTests, followInstantSwitchToTrue ) {  // 3.1
    AutoStoppedTaskLoop taskLoop( *this );

    auto followOperator = std::make_shared< FollowFunctionOperator >( taskLoop.m_taskLoop, std::chrono::seconds( 3 ) );
    auto eventOperator = createEventOperator( followOperator );

    OperatorStatesCollector statesCollector( *followOperator, taskLoop.m_taskLoop );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const auto& delayedStates = statesCollector.m_states;
    ASSERT_EQ( delayedStates.size(), 1 );
    DelayedState expected( AbstractOperator::OperatorState::True, std::chrono::seconds( 0 ) );
    EXPECT_EQ( delayedStates.front(), expected );
}

TEST_F( FunctionOperatorTests, followDelayedResetOneChild ) {  // 3.2
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds resetDelay( 3 );
    auto followOperator = std::make_shared< FollowFunctionOperator >( taskLoop.m_taskLoop, resetDelay );
    auto eventOperator = createEventOperator( followOperator );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    OperatorStatesCollector statesCollector( *followOperator, taskLoop.m_taskLoop );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 1 ) );

    checkFollowDelayedReset( statesCollector, resetDelay );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
}

TEST_F( FunctionOperatorTests, followDelayedResetMultipleChildren ) {  // 3.3
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds resetDelay( 3 );
    auto followOperator = std::make_shared< FollowFunctionOperator >( taskLoop.m_taskLoop, resetDelay );
    auto eventOperator1 = createEventOperator( followOperator );
    auto eventOperator2 = createEventOperator( followOperator );
    auto eventOperator3 = createEventOperator( followOperator );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, true ) );

    OperatorStatesCollector statesCollector( *followOperator, taskLoop.m_taskLoop );

    EXPECT_TRUE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator3, false ) );

    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );
    for( auto& eventOperator : { eventOperator1, eventOperator2, eventOperator3 } ) {
        EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    }

    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 1 ) );
    checkFollowDelayedReset( statesCollector, resetDelay );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );
}

TEST_F( FunctionOperatorTests, followTransitionNotTriggeredIfNotAllChildrenAreTrue ) {  // 3.4
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds resetDelay( 1 );
    auto followOperator = std::make_shared< FollowFunctionOperator >( taskLoop.m_taskLoop, resetDelay );
    auto eventOperator1 = createEventOperator( followOperator );
    auto eventOperator2 = createEventOperator( followOperator );
    auto eventOperator3 = createEventOperator( followOperator );

    OperatorStatesCollector statesCollector( *followOperator, taskLoop.m_taskLoop );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );
    EXPECT_TRUE( statesCollector.m_states.empty() );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );
    EXPECT_TRUE( statesCollector.m_states.empty() );

    EXPECT_FALSE( evaluateSubtree( *eventOperator1, false ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator2, true ) );
    EXPECT_FALSE( evaluateSubtree( *eventOperator3, true ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );
    EXPECT_TRUE( statesCollector.m_states.empty() );
}

TEST_F( FunctionOperatorTests, followInvalidResetDelay ) {  // 3.5
    HubPlatform::Services::TaskLoop taskLoop;

    const std::string expectedMessage = "follow: 'delayReset' parameter should be positive integer";
    checkOperatorConstructionFails< FollowFunctionOperator >( expectedMessage, taskLoop, std::chrono::seconds( -1 ) );
    checkOperatorConstructionFails< FollowFunctionOperator >( expectedMessage, taskLoop, std::chrono::seconds( 0 ) );
}

TEST_F( FunctionOperatorTests, followResetDiscardedIfOperatorReturnedToTrue ) {  // 3.6
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds resetDelay( 3 );
    auto followOperator = std::make_shared< FollowFunctionOperator >( taskLoop.m_taskLoop, resetDelay );
    auto eventOperator = createEventOperator( followOperator );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    OperatorStatesCollector statesCollector( *followOperator, taskLoop.m_taskLoop );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 30 ) );

    EXPECT_TRUE( statesCollector.m_states.empty() );
}

TEST_F( FunctionOperatorTests, followTimerRestartedAfterPreviousTransition ) {  // 3.7
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds discardedResetWaitTime( 2 );

    const std::chrono::seconds resetDelay( 3 );
    auto followOperator = std::make_shared< FollowFunctionOperator >( taskLoop.m_taskLoop, resetDelay );
    auto eventOperator = createEventOperator( followOperator );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    OperatorStatesCollector statesCollector( *followOperator, taskLoop.m_taskLoop );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( discardedResetWaitTime ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 2 ) );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepAndSync( taskLoop.m_taskLoop, std::chrono::seconds( 1 ) );

    checkFollowDelayedReset( statesCollector, resetDelay + discardedResetWaitTime );
}

TEST_F( FunctionOperatorTests, pulseSingleTransitionToTrue ) {  // 4.1
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 2 );
    const std::chrono::seconds falsePeriod( 0 );
    auto pulseOperator = std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, 1 );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const std::chrono::seconds noExtraTransitionsCheckTime( 3 );
    sleepStepped( taskLoop.m_taskLoop, truePeriod + noExtraTransitionsCheckTime, PulseTestSleepStep );
    checkPulseTransitions( statesCollector, truePeriod, falsePeriod, 1 );
}

TEST_F( FunctionOperatorTests, pulseMultipleTransitions ) {  // 4.2
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    const int times = 3;
    auto pulseOperator =
        std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, times );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const std::chrono::seconds noExtraTransitionsCheckTime( 3 );
    const auto totalTestTime = ( truePeriod + falsePeriod ) * times + noExtraTransitionsCheckTime;
    sleepStepped( taskLoop.m_taskLoop, totalTestTime, PulseTestSleepStep );
    checkPulseTransitions( statesCollector, truePeriod, falsePeriod, times );
}

TEST_F( FunctionOperatorTests, pulseInvalidTruePeriodParam ) {  // 4.3
    HubPlatform::Services::TaskLoop taskLoop;
    const std::chrono::seconds truePeriod( 0 );
    const std::chrono::seconds falsePeriod( 0 );
    const std::string message = "pulse: 'truePeriod' parameter should be a positive integer";
    checkOperatorConstructionFails< PulseFunctionOperator >( message, taskLoop, truePeriod, falsePeriod );
}

TEST_F( FunctionOperatorTests, pulseInvalidFalsePeriodParam ) {  // 4.4
    HubPlatform::Services::TaskLoop taskLoop;
    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( -1 );
    const std::string message = "pulse: 'falsePeriod' parameter should be a non-negative integer";
    checkOperatorConstructionFails< PulseFunctionOperator >( message, taskLoop, truePeriod, falsePeriod );
}

TEST_F( FunctionOperatorTests, pulseInvalidTimesParam ) {  // 4.5
    HubPlatform::Services::TaskLoop taskLoop;
    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    const int times = 0;
    const std::string message = "pulse: 'times' parameter should be a positive integer";
    checkOperatorConstructionFails< PulseFunctionOperator >( message, taskLoop, truePeriod, falsePeriod, times );
}

TEST_F( FunctionOperatorTests, pulseChildResetWhilePulseWasInTrueState ) {  // 4.6
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    const int times = 3;
    auto pulseOperator =
        std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, times );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    const auto falseTransitionTime = truePeriod / 2;

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    sleepStepped( taskLoop.m_taskLoop, falseTransitionTime, PulseTestSleepStep );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );

    const auto remainingTestTime = falseTransitionTime + truePeriod * ( times - 1 ) + falsePeriod * times;
    sleepStepped( taskLoop.m_taskLoop, remainingTestTime, PulseTestSleepStep );

    const std::vector< DelayedState >& states = statesCollector.m_states;
    ASSERT_EQ( states.size(), 2 );
    EXPECT_EQ( states[ 0 ].first, AbstractOperator::OperatorState::True );
    EXPECT_EQ( states[ 0 ].second, std::chrono::milliseconds( 0 ) );
    EXPECT_EQ( states[ 1 ].first, AbstractOperator::OperatorState::False );
    // in TRUE state, function cannot be interrupted, only on next transition to FALSE
    EXPECT_EQ( states[ 1 ].second, truePeriod );
}

TEST_F( FunctionOperatorTests, pulseChildChangedStateTwiceOnTruePeriod ) {  // 4.7
    // child state:
    //    ___   ________________________________________________
    // ___| |___|
    // expected pulse output:
    //    _________         _________         _________
    // ___|       |_________|       |_________|       |_________

    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    const int times = 3;
    auto pulseOperator =
        std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, times );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    const auto stateChangeTime = std::chrono::milliseconds( truePeriod ) / 5;

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    sleepStepped( taskLoop.m_taskLoop, stateChangeTime, PulseTestSleepStep );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepStepped( taskLoop.m_taskLoop, stateChangeTime, PulseTestSleepStep );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const std::chrono::seconds noExtraTransitionsCheckTime( 3 );
    const auto remainingTestTime = stateChangeTime + truePeriod * 2 + falsePeriod * 3 + noExtraTransitionsCheckTime;
    sleepStepped( taskLoop.m_taskLoop, remainingTestTime, PulseTestSleepStep );
    checkPulseTransitions( statesCollector, truePeriod, falsePeriod, times );
}

TEST_F( FunctionOperatorTests, pulseChildChangedStateTwiceOnFalsePeriod ) {  // 4.8
    // child state:
    //    ___________ ______________________________________________________
    // ___|         |_|
    // expected pulse output:
    //    _________   _________         _________         _________
    // ___|       |___|       |_________|       |_________|       |_________

    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    const int times = 3;
    auto pulseOperator =
        std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, times );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    const auto stateChangeTime = std::chrono::milliseconds( truePeriod ) / 5;

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    sleepStepped( taskLoop.m_taskLoop, truePeriod + stateChangeTime, PulseTestSleepStep );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    sleepStepped( taskLoop.m_taskLoop, stateChangeTime, PulseTestSleepStep );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const std::chrono::seconds noExtraTransitionsCheckTime( 3 );
    const auto remainingTestTime = ( truePeriod + falsePeriod ) * times + noExtraTransitionsCheckTime;
    sleepStepped( taskLoop.m_taskLoop, remainingTestTime, PulseTestSleepStep );

    const int transitionBeforeRestart = 2;
    const std::vector< DelayedState >& delayedStates = statesCollector.m_states;
    ASSERT_EQ( delayedStates.size(), times * 2 + transitionBeforeRestart );

    std::chrono::milliseconds checkTime( falsePeriod + stateChangeTime * 2 );
    for( size_t i = transitionBeforeRestart; i < delayedStates.size(); i += 2 ) {
        const DelayedState& transitionTrue = delayedStates[ i ];
        EXPECT_EQ( transitionTrue.first, AbstractOperator::OperatorState::True );
        EXPECT_EQ( transitionTrue.second, checkTime );
        checkTime += truePeriod;

        const DelayedState& transitionFalse = delayedStates[ i + 1 ];
        EXPECT_EQ( transitionFalse.first, AbstractOperator::OperatorState::False );
        EXPECT_EQ( transitionFalse.second, checkTime );
        checkTime += falsePeriod;
    }
}

TEST_F( FunctionOperatorTests, pulseChildChangedStateTwiceOnLastActivation ) {  // 4.9
    // child state:
    //    _________________________________________________________   ___________
    // ___|                                                       |___|
    // expected pulse output:
    //    _________         _________         _________         _________
    // ___|       |_________|       |_________|       |_________|       |________

    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    const int times = 3;
    auto pulseOperator =
        std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, times );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    const auto lastActivationTime = ( truePeriod + falsePeriod ) * ( times - 1 );
    sleepStepped( taskLoop.m_taskLoop, lastActivationTime, PulseTestSleepStep );

    const auto stateChangeTime = std::chrono::milliseconds( truePeriod ) / 5;

    sleepStepped( taskLoop.m_taskLoop, stateChangeTime, PulseTestSleepStep );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    sleepStepped( taskLoop.m_taskLoop, stateChangeTime, PulseTestSleepStep );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const std::chrono::seconds noExtraTransitionsCheckTime( 3 );
    const auto remainingTestTime = stateChangeTime + falsePeriod + noExtraTransitionsCheckTime;
    sleepStepped( taskLoop.m_taskLoop, remainingTestTime, PulseTestSleepStep );
    checkPulseTransitions( statesCollector, truePeriod, falsePeriod, times );
}

TEST_F( FunctionOperatorTests, pulseChildMultipleTransitionsToTrue ) {  // 4.10
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 2 );
    const std::chrono::seconds falsePeriod( 0 );
    auto pulseOperator = std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod, 1 );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    const auto nextTransitionDelay = truePeriod + std::chrono::milliseconds( 500 );

    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    sleepStepped( taskLoop.m_taskLoop, nextTransitionDelay, PulseTestSleepStep );
    EXPECT_FALSE( evaluateSubtree( *eventOperator, true ) );
    sleepStepped( taskLoop.m_taskLoop, truePeriod, PulseTestSleepStep );

    checkPulseTransitions( statesCollector, truePeriod, falsePeriod, 1 );
}

TEST_F( FunctionOperatorTests, pulseInfinitePulses ) {  // 4.11
    AutoStoppedTaskLoop taskLoop( *this );

    const std::chrono::seconds truePeriod( 1 );
    const std::chrono::seconds falsePeriod( 1 );
    auto pulseOperator = std::make_shared< PulseFunctionOperator >( taskLoop.m_taskLoop, truePeriod, falsePeriod );
    auto eventOperator = createEventOperator( pulseOperator );

    OperatorStatesCollector statesCollector( *pulseOperator, taskLoop.m_taskLoop );

    // test
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    const std::chrono::seconds testDuration( 6 );
    sleepStepped( taskLoop.m_taskLoop, testDuration, PulseTestSleepStep );

    const int expectedTransitions = 7;  // 6 - 3 time full truePeriod + falsePeriod, 7th - the next transition to "true"
    ASSERT_EQ( expectedTransitions, statesCollector.m_states.size() );

    std::chrono::milliseconds checkTime( 0 );
    bool expectedState = true;
    for( const DelayedState& state : statesCollector.m_states ) {
        ASSERT_EQ( state.first, AbstractOperator::toOperatorState( expectedState ) );
        ASSERT_EQ( state.second, checkTime );

        checkTime += expectedState == true ? truePeriod : falsePeriod;
        expectedState = !expectedState;
    }
}

TEST_F( FunctionOperatorTests, latch ) {  // 5.1
    auto latchOperator = createLatchOperator( nullptr );
    auto eventOperator = createEventOperator( latchOperator );

    // Latch the kid
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
    // Kid state change ignored
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    latchOperator->openLatch();
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
}

TEST_F( FunctionOperatorTests, uplatchFromParentAnd ) {  // 5.2
    auto andOperator = createAndOperator( nullptr );
    andOperator->setBlockID( "parent_and" );
    auto freeEvent = createEventOperator( andOperator );
    freeEvent->setBlockID( "child_free_event" );
    auto latchOperator = createLatchOperator( andOperator );
    latchOperator->setBlockID( "child_latch" );
    auto underLatchEvent = createEventOperator( latchOperator );
    underLatchEvent->setBlockID( "child_underlatch" );

    bool oldState = false;
    auto andOperatorNotifyEventState = [andOperator, oldState]( AbstractOperator::OperatorState state ) mutable {
        bool newState = AbstractOperator::toBool( state );

        if( oldState != newState ) {
            if( state == AbstractOperator::OperatorState::False ) {
                // iterate all children on the block, select latches, reset closed ones
                if( auto* multipleChildren = dynamic_cast< MultipleChildrenOperator* >( andOperator.get() ) ) {
                    multipleChildren->forEach( []( AbstractOperator& op ) {
                        const bool resetLatch = true;
                        LatchVisitor visitor( resetLatch );
                        op.accept( visitor );
                    } );
                }
            }
            oldState = newState;
        }
    };
    andOperator->setOperatorStateEventHandler( andOperatorNotifyEventState );

    // Latch the kid, (AND( latch(event1), event2 )) -> false
    EXPECT_FALSE( evaluateSubtree( *underLatchEvent, true ) );
    EXPECT_TRUE( latchOperator->isLatched() );
    // make event2 also true
    // Latch the kid, (AND( latch(event1), event2 )) -> true
    EXPECT_TRUE( evaluateSubtree( *freeEvent, true ) );
    bool sceneState;
    // Kid state change ignored
    //    EXPECT_FALSE( evaluateSubtree( *eventOperator,  false ) );
    sceneState = ( evaluateSubtree( *underLatchEvent, false ) );
    //    EXPECT_TRUE( evaluateSubtree( *eventOperator,  true ) );
    // should be FALSE logically, but AND operator allows a child to return true
    sceneState = ( evaluateSubtree( *underLatchEvent, true ) );

    // make AND operator False using second event -> and operator -> false -> should open latch
    EXPECT_TRUE( latchOperator->isLatched() );

    sceneState = ( evaluateSubtree( *freeEvent, false ) );
    EXPECT_FALSE( latchOperator->isLatched() );
}

TEST_F( FunctionOperatorTests, latchNotSensor ) {  // 5.3
    auto latchOperator = createLatchOperator( nullptr );
    auto notOperator = createNotOperator( latchOperator );
    auto eventOperator = createEventOperator( notOperator );

    // Latch the kid: not(false) == true
    ASSERT_TRUE( evaluateSubtree( *eventOperator, false ) );
    // Kid state change ignored
    ASSERT_TRUE( evaluateSubtree( *eventOperator, true ) );
    ASSERT_TRUE( evaluateSubtree( *eventOperator, false ) );

    latchOperator->openLatch();
    ASSERT_FALSE( evaluateSubtree( *eventOperator, true ) );
    ASSERT_TRUE( evaluateSubtree( *eventOperator, false ) );
}

TEST_F( FunctionOperatorTests, latchVisitor ) {  // 6.1
    auto latchOperator = createLatchOperator( nullptr );
    auto eventOperator = createEventOperator( latchOperator );

    LatchVisitor constVisitor( false );
    // check latch visitor init state
    EXPECT_FALSE( constVisitor.isCompleted() );
    EXPECT_FALSE( constVisitor.isLatch() );
    EXPECT_FALSE( constVisitor.isLatchClosed() );

    // check event operator with latch visitor
    eventOperator->accept( constVisitor );
    EXPECT_FALSE( constVisitor.isLatch() );
    constVisitor.clear();

    // check latch operator(latch opened) with latch visitor
    latchOperator->accept( constVisitor );
    EXPECT_TRUE( constVisitor.isLatch() );
    EXPECT_FALSE( constVisitor.isLatchClosed() );
    EXPECT_FALSE( constVisitor.isCompleted() );
    constVisitor.clear();

    // Latch the kid
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    // check latch operator(latch closed) with latch visitor
    latchOperator->accept( constVisitor );
    EXPECT_TRUE( constVisitor.isLatch() );
    EXPECT_TRUE( constVisitor.isLatchClosed() );
    EXPECT_FALSE( constVisitor.isCompleted() );
    constVisitor.clear();

    // Kid state change ignored, latch stays closed
    EXPECT_TRUE( evaluateSubtree( *eventOperator, false ) );
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );

    LatchVisitor resetLatchVisitor( true );
    // check latch operator(latch closed) with modifying latch visitor: latch operator becomes open
    latchOperator->accept( resetLatchVisitor );
    EXPECT_TRUE( resetLatchVisitor.isLatch() );
    EXPECT_FALSE( resetLatchVisitor.isLatchClosed() );
    EXPECT_TRUE( resetLatchVisitor.isCompleted() );
    resetLatchVisitor.clear();

    // event condition transits to False freely
    EXPECT_FALSE( evaluateSubtree( *eventOperator, false ) );
    // event condition transits to True, latched again
    EXPECT_TRUE( evaluateSubtree( *eventOperator, true ) );
}

TEST_F( FunctionOperatorTests, operatorNameVisitor ) {  // 6.2
    OperatorNameVisitor visitor;
    HubPlatform::Services::TaskLoop taskLoop;

    EventOperator event;
    NotOperator notOp;
    OrOperator orOp;
    XorOperator xorOp;
    AndOperator andOp;
    FunctionOperator functionOp;
    ForLessFunctionOperator functionForLessOp;
    ForAtLeastFunctionOperator functionForAtLeastOp( taskLoop );
    RepeatFunctionOperator functionRepeatOp;
    FollowFunctionOperator functionFollowOp( taskLoop, std::chrono::seconds( 5 ) );
    PulseFunctionOperator functionPulseOp( taskLoop, std::chrono::seconds( 5 ), std::chrono::seconds( 5 ) );
    LatchFunctionOperator functionLatchOp;

    using OpList = std::vector< AbstractOperator* >;
    OpList operators = { &event, &notOp, &orOp, &xorOp, &andOp };
    OpList functions = { &functionOp,       &functionForLessOp, &functionForAtLeastOp, &functionRepeatOp,
                         &functionFollowOp, &functionPulseOp,   &functionLatchOp };
    operators.insert( end( operators ), begin( functions ), end( functions ) );

    struct TestInfo {
        std::string name;
        bool isFunction;
        std::string functionName;
    };
    std::vector< TestInfo > names = {
        { "event", false },
        { "not", false },
        { "or", false },
        { "xor", false },
        { "and", false },

        { "function", true },
        { "function", true, "for/less" },
        { "function", true, "for/at_least" },
        { "function", true, "repeat" },
        { "function", true, "follow" },
        { "function", true, "pulse" },
        { "function", true, "latch" },
    };
    ASSERT_EQ( operators.size(), names.size() ) << "Sizes should match";

    for( size_t i = 0; i < operators.size(); i++ ) {
        auto op = operators[ i ];
        op->accept( visitor );
        TestInfo& testInfo = names[ i ];

        ASSERT_EQ( visitor.name(), testInfo.name );
        ASSERT_EQ( visitor.isFunction(), testInfo.isFunction );
        ASSERT_EQ( visitor.functionName(), testInfo.functionName );
    }
}

}  // namespace UT
}  // namespace SceneEngine
