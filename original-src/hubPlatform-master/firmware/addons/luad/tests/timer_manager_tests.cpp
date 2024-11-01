#include "luad/modules/timer/timer_manager.h"
#include "luad/modules/timer/timer_resources.h"
#include "luad/modules/timer/timer_type.h"
#include "luad/tests/fakes/fake_event_emitter.h"
#include "luad/tests/fakes/fake_plugin_status_events_subscription.h"

#include "service_interfaces/task_loop.h"
#include "service_interfaces/tests/task_loop_test_helper.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Set timer
DONE        1.1 Periodic
DONE        1.2 Single
DONE        1.3 Script params
DONE            1.3.1 Empty
DONE            1.3.2 Not Empty
DONE        1.4 Custom id
DONE    2. Stop timer
DONE        2.1 Periodic
DONE        2.2 Single
DONE        2.3 Invalid timer id
DONE        2.4 The same timer several times
DONE        2.5 Custom id
DONE    3. Restart timer
DONE        3.1 Periodic
DONE        3.2 Single
DONE        3.3 Invalid timer id
DONE        3.4 Already executed timer
DONE    4. Timer parameters
DONE        4.1 Set empty parameters
DONE        4.2 Set non-empty parameters
DONE        4.3 Set/get parameters for non-existing timer
DONE    5. Timers groups
DONE        5.1 Setup timers and add them to different timers groups
DONE        5.2 Destroy timers group
DONE        5.3 Stop a whole timers group
DONE    6. Plugin status events subscription
DONE        6.1 Check startup subscription
DONE        6.2 Check stopping a whole timers group in a response of a plugin status change
DONE            6.2.1 Check stopping a whole timers group in a response of a plugin restart
DONE            6.2.2 Check stopping a whole timers group in a response of a plugin shutdown
DONE            6.2.3 Check stopping a whole timers group in a response of a plugin uninstall
DONE    7. Check timers are grouped
*/

namespace HubPlatform {
namespace Luad {
namespace UT {

class TimeManagerTestSuite : public ::testing::Test, public Services::UT::TaskLoopTestHelper {
public:
    void SetUp() override {
        setTimeProviderForLoop( m_taskLoop );
        m_future = runInThread( m_taskLoop );
        m_timerManager =
            std::make_unique< TimerManager >( m_fakeEventEmitter, m_taskLoop, m_fakePluginStatusEventsSubscription );
    }

    void TearDown() override {
        m_taskLoop.stop();
        m_future.get();
    }

    TimerManager& getTimerManager() {
        return *m_timerManager;
    }

    FakeEventEmitter& getEventEmitter() {
        return m_fakeEventEmitter;
    }

    void sleep( TimerDuration delay ) {
        TaskLoopTestHelper::sleep( m_taskLoop, delay );
        TaskLoopTestHelper::waitForTasks( m_taskLoop );
    }

    bool hasTimer( const GroupName& groupName, const TimerId& timerId ) {
        Counter counter;
        bool result{ false };

        m_taskLoop.addTask( [&]() {
            result = getTimerManager().hasTimer( groupName, timerId );
            ++counter;
        } );

        counter.wait( 1 );
        return result;
    }

    FakePluginStatusEventsSubscription& getFakePluginStatusEventsSubscription() {
        return m_fakePluginStatusEventsSubscription;
    }

    const TimerManager::TimersGroups& getTimersGroups() const {
        return m_timerManager->m_timers;
    }

private:
    Services::TaskLoop m_taskLoop;
    FakeEventEmitter m_fakeEventEmitter;
    FakePluginStatusEventsSubscription m_fakePluginStatusEventsSubscription;
    std::unique_ptr< TimerManager > m_timerManager;
    std::future< void > m_future;
};

TEST_F( TimeManagerTestSuite, testSetTimerPeriodic /*1.1*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId = getTimerManager().setTimer( groupName,
                                                     TimerDuration{ 1000 },
                                                     "",
                                                     JSON( "key"
                                                           << "value" ),
                                                     TimerType::Periodic );

    sleep( TimerDuration{ 10000 } );

    counter.wait( 10 );
    EXPECT_TRUE( hasTimer( groupName, timerId ) );

    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ),
               JSON( "key"
                     << "value" ) );
}

TEST_F( TimeManagerTestSuite, testSetTimerSingle /* 1.2 */ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId = getTimerManager().setTimer( groupName,
                                                     TimerDuration{ 1000 },
                                                     "",
                                                     JSON( "key"
                                                           << "value" ),
                                                     TimerType::Single );

    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ),
               JSON( "key"
                     << "value" ) );

    sleep( TimerDuration{ 1000 } );

    counter.wait( 1 );

    EXPECT_FALSE( hasTimer( groupName, timerId ) );
    EXPECT_EQ( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
}

TEST_F( TimeManagerTestSuite, testSetTimerScriptParamsEmpty /*1.3.1*/ ) {
    Counter counter;
    TimerId timerId;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter, &timerId]( const std::string&, const Json& eventData ) {
        ++counter;
        EXPECT_EQ( eventData, JSON( "timerId" << timerId ) );
    } );

    timerId = getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ), JSON_EMPTY_OBJ );

    sleep( TimerDuration{ 1000 } );

    counter.wait( 1 );

    EXPECT_FALSE( hasTimer( groupName, timerId ) );
    EXPECT_EQ( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
}

TEST_F( TimeManagerTestSuite, testSetTimerScriptParamsNotEmpty /*1.3.2*/ ) {
    Counter counter;
    TimerId timerId;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter, &timerId]( const std::string&, const Json& eventData ) {
        ++counter;
        EXPECT_EQ( eventData,
                   JSON( "timerId" << timerId << "key"
                                   << "value" ) );
    } );

    timerId = getTimerManager().setTimer( groupName,
                                          TimerDuration{ 1000 },
                                          "",
                                          JSON( "key"
                                                << "value" ),
                                          TimerType::Single );

    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ),
               JSON( "key"
                     << "value" ) );

    sleep( TimerDuration{ 1000 } );

    counter.wait( 1 );

    EXPECT_FALSE( hasTimer( groupName, timerId ) );
    EXPECT_EQ( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
}

TEST_F( TimeManagerTestSuite, testSetTimerCustomId /* 1.4 */ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    TimerId timerId = "test_id";
    getTimerManager().setTimerWithId(
        groupName, timerId, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    sleep( TimerDuration{ 1000 } );

    counter.wait( 1 );

    EXPECT_FALSE( hasTimer( groupName, timerId ) );
}

TEST_F( TimeManagerTestSuite, testStopTimerPeriodic /*2.1*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Periodic );

    sleep( TimerDuration{ 10000 } );
    counter.wait( 10 );

    getTimerManager().stopTimer( groupName, timerId );

    sleep( TimerDuration{ 10000 } );

    EXPECT_EQ( counter.get(), 10 );
    EXPECT_FALSE( hasTimer( groupName, timerId ) );
}

TEST_F( TimeManagerTestSuite, testStopTimerSingle /*2.2*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    sleep( TimerDuration{ 999 } );
    EXPECT_EQ( counter.get(), 0 );

    getTimerManager().stopTimer( groupName, timerId );
    EXPECT_FALSE( hasTimer( groupName, timerId ) );

    sleep( TimerDuration{ 1 } );
    EXPECT_EQ( counter.get(), 0 );
}

TEST_F( TimeManagerTestSuite, testStopTimerInvalidTimerId /*2.3*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const TimerId invalidTimerId{ "invalidTimerId" };

    sleep( TimerDuration{ 500 } );

    EXPECT_NO_THROW( getTimerManager().stopTimer( groupName, invalidTimerId ) );
    EXPECT_TRUE( hasTimer( groupName, timerId ) );
}

TEST_F( TimeManagerTestSuite, testStopTimerTheSameTimerSeveralTimes /*2.4*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    sleep( TimerDuration{ 500 } );

    getTimerManager().stopTimer( groupName, timerId );
    EXPECT_FALSE( hasTimer( groupName, timerId ) );
    EXPECT_NO_THROW( getTimerManager().stopTimer( groupName, timerId ) );
}

TEST_F( TimeManagerTestSuite, testStopTimerCustomId /*2.5*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    TimerId timerId = "test_id";
    getTimerManager().setTimerWithId(
        groupName, timerId, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    sleep( TimerDuration{ 999 } );
    EXPECT_EQ( counter.get(), 0 );

    getTimerManager().stopTimer( groupName, timerId );
    EXPECT_FALSE( hasTimer( groupName, timerId ) );

    sleep( TimerDuration{ 1 } );
    EXPECT_EQ( counter.get(), 0 );
}

TEST_F( TimeManagerTestSuite, testRestartTimerPeriodic /*3.1*/ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Periodic );

    sleep( TimerDuration{ 1000 } );
    counter.wait( 1 );

    getTimerManager().restartTimer( groupName, timerId, TimerDuration{ 500 } );

    sleep( TimerDuration{ 1000 } );
    counter.wait( 3 );
}

TEST_F( TimeManagerTestSuite, testRestartTimerSingle /* 3.2 */ ) {
    Counter counter;
    const GroupName groupName = "group1";

    getEventEmitter().setEmitHandler( [&counter]() { ++counter; } );

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    sleep( TimerDuration{ 999 } );

    getTimerManager().restartTimer( groupName, timerId, TimerDuration{ 500 } );
    sleep( TimerDuration{ 500 } );
    counter.wait( 1 );
    EXPECT_FALSE( hasTimer( groupName, timerId ) );
}

TEST_F( TimeManagerTestSuite, testRestartTimerInvalidTimerId /* 3.3 */ ) {
    EXPECT_NO_THROW( getTimerManager().restartTimer( GroupName{}, TimerId{}, TimerDuration{ 500 } ) );
}

TEST_F( TimeManagerTestSuite, testRestartTimerAlereadyExecutedTimer /* 3.4 */ ) {
    const GroupName groupName = "group1";

    const auto timerId =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    sleep( TimerDuration{ 1000 } );

    EXPECT_FALSE( hasTimer( groupName, timerId ) );

    EXPECT_NO_THROW( getTimerManager().restartTimer( groupName, timerId, TimerDuration{ 500 } ) );
}

TEST_F( TimeManagerTestSuite, testSetTimerParamatersEmpty /*4.1*/ ) {
    const GroupName groupName = "group1";

    const auto timerId = getTimerManager().setTimer( groupName,
                                                     TimerDuration{ 1000 },
                                                     "",
                                                     JSON( "key"
                                                           << "value" ),
                                                     TimerType::Single );

    getTimerManager().setTimerParameters( groupName, timerId, Json() );
    ASSERT_TRUE( getTimerManager().getTimerParameters( groupName, timerId ) );

    EXPECT_TRUE( getTimerManager().hasTimer( groupName, timerId ) );
    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ), Json() );
}

TEST_F( TimeManagerTestSuite, testSetTimerParamatersNonEmpty /*4.2*/ ) {
    const GroupName groupName = "group1";

    const auto timerId = getTimerManager().setTimer( groupName,
                                                     TimerDuration{ 1000 },
                                                     "",
                                                     JSON( "key"
                                                           << "value" ),
                                                     TimerType::Single );

    getTimerManager().setTimerParameters( groupName,
                                          timerId,
                                          JSON( "a"
                                                << "b" ) );
    ASSERT_TRUE( getTimerManager().getTimerParameters( groupName, timerId ) );

    EXPECT_TRUE( getTimerManager().hasTimer( groupName, timerId ) );
    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ),
               JSON( "a"
                     << "b" ) );
}

TEST_F( TimeManagerTestSuite, testSetAndGetParamatersForNonExistingTimer /*4.3*/ ) {
    const GroupName groupName = "group1";

    const auto timerId = getTimerManager().setTimer( groupName,
                                                     TimerDuration{ 1000 },
                                                     "",
                                                     JSON( "key"
                                                           << "value" ),
                                                     TimerType::Single );

    const auto nonExistingTimerId = timerId + "_";
    EXPECT_FALSE( getTimerManager().getTimerParameters( groupName, nonExistingTimerId ) );
    getTimerManager().setTimerParameters( groupName, nonExistingTimerId, Json() );
    ASSERT_TRUE( getTimerManager().getTimerParameters( groupName, timerId ) );
    ASSERT_NE( getTimerManager().getTimerParameters( groupName, timerId ), nullptr );
    EXPECT_EQ( *getTimerManager().getTimerParameters( groupName, timerId ),
               JSON( "key"
                     << "value" ) );
}

TEST_F( TimeManagerTestSuite, testSetTimersToDifferentGroups /* 5.1 */ ) {
    const auto timerId1 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId3 =
        getTimerManager().setTimer( "group2", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    const auto& timersGroup = getTimersGroups();

    EXPECT_TRUE( hasTimer( "group1", timerId1 ) );
    EXPECT_TRUE( hasTimer( "group1", timerId2 ) );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );

    ASSERT_TRUE( timersGroup.find( "group1" ) != timersGroup.end() );
    ASSERT_TRUE( timersGroup.find( "group2" ) != timersGroup.end() );
    EXPECT_EQ( timersGroup.at( "group1" ).size(), 2 );
    EXPECT_EQ( timersGroup.at( "group2" ).size(), 1 );
}

TEST_F( TimeManagerTestSuite, testTimerGroupDestroying /* 5.2 */ ) {
    const GroupName groupName = "group1";

    const auto timerId1 =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( groupName, TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    const auto& timersGroup = getTimersGroups();

    ASSERT_TRUE( timersGroup.find( groupName ) != timersGroup.end() );
    EXPECT_EQ( timersGroup.at( groupName ).size(), 2 );

    getTimerManager().stopTimer( groupName, timerId1 );
    ASSERT_TRUE( timersGroup.find( groupName ) != timersGroup.end() );
    EXPECT_EQ( timersGroup.at( groupName ).size(), 1 );

    getTimerManager().stopTimer( groupName, timerId2 );
    ASSERT_TRUE( timersGroup.find( groupName ) == timersGroup.end() );
}

TEST_F( TimeManagerTestSuite, testStopTimersGroup /* 5.3 */ ) {
    const auto timerId1 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId3 =
        getTimerManager().setTimer( "group2", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    getTimerManager().stopTimersGroup( "group1" );
    EXPECT_FALSE( hasTimer( "group1", timerId1 ) );
    EXPECT_FALSE( hasTimer( "group1", timerId2 ) );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );
}

TEST_F( TimeManagerTestSuite, testCheckStartupSubscription /* 6.1 */ ) {
    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    EXPECT_EQ( pluginStatusEventsSubscription.m_pluginRestartedCallbacks.size(), 1 );
    EXPECT_EQ( pluginStatusEventsSubscription.m_pluginTeardownCallbacks.size(), 1 );
    EXPECT_EQ( pluginStatusEventsSubscription.m_pluginUninstallCallbacks.size(), 1 );
}

TEST_F( TimeManagerTestSuite, testStopTimerGroupAfterPluginRestarted /* 6.2.1 */ ) {
    const auto timerId1 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId3 =
        getTimerManager().setTimer( "group2", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    pluginStatusEventsSubscription.m_pluginRestartedCallbacks[ 0 ]( "group1" );
    EXPECT_FALSE( hasTimer( "group1", timerId1 ) );
    EXPECT_FALSE( hasTimer( "group1", timerId2 ) );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );
}

TEST_F( TimeManagerTestSuite, testStopTimerGroupAfterPluginShutdown /* 6.2.2 */ ) {
    const auto timerId1 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId3 =
        getTimerManager().setTimer( "group2", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    pluginStatusEventsSubscription.m_pluginTeardownCallbacks[ 0 ]( "group1" );
    EXPECT_FALSE( hasTimer( "group1", timerId1 ) );
    EXPECT_FALSE( hasTimer( "group1", timerId2 ) );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );
}

TEST_F( TimeManagerTestSuite, testStopTimerGroupAfterPluginUninstalled /* 6.2.3 */ ) {
    const auto timerId1 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId3 =
        getTimerManager().setTimer( "group2", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    const auto pluginStatusEventsSubscription = getFakePluginStatusEventsSubscription();

    pluginStatusEventsSubscription.m_pluginUninstallCallbacks[ 0 ]( "group1" );
    EXPECT_FALSE( hasTimer( "group1", timerId1 ) );
    EXPECT_FALSE( hasTimer( "group1", timerId2 ) );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );
}

TEST_F( TimeManagerTestSuite, testTimersInGroupsAreIndependent /* 7 */ ) {
    const auto timerId1 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId2 =
        getTimerManager().setTimer( "group1", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId3 =
        getTimerManager().setTimer( "group2", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );
    const auto timerId4 =
        getTimerManager().setTimer( "group3", TimerDuration{ 1000 }, "", JSON_EMPTY_OBJ, TimerType::Single );

    getTimerManager().stopTimersGroup( "group1" );
    EXPECT_FALSE( hasTimer( "group1", timerId1 ) );
    EXPECT_FALSE( hasTimer( "group1", timerId2 ) );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );
    EXPECT_TRUE( hasTimer( "group3", timerId4 ) );

    EXPECT_FALSE( hasTimer( "group1", timerId4 ) );
    EXPECT_FALSE( hasTimer( "group2", timerId4 ) );

    getTimerManager().stopTimer( "group1", timerId3 );
    EXPECT_TRUE( hasTimer( "group2", timerId3 ) );

    getTimerManager().stopTimer( "group2", timerId3 );
    EXPECT_FALSE( hasTimer( "group2", timerId3 ) );
    EXPECT_TRUE( hasTimer( "group3", timerId4 ) );
}

}  // namespace UT
}  // namespace Luad
}  // namespace HubPlatform
