#include "service_interfaces/tests/fixtures/fake_service_fixture.h"
#include "service_interfaces/tests/fixtures/service_fixture.h"

#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"

#include "database_client/fake_db/fake_db_client.h"

#include "bus/messagesclasses.h"

#include "common_utils/cpp14.h"
#include "bus/spread_bus.h"

#include "test_utils/test_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <unordered_map>

namespace HubPlatform {
namespace Services {
namespace UT {

using namespace ::testing;

namespace {

class TestBus final : public BaseBus {
public:
    TestBus( const std::string& privateName, const RxCallBack& rxCallBack, bool silent, int connectionAttempts )
    : BaseBus( privateName, rxCallBack ) {}

    void start() override {}

    void stop() override {}

    void join( const char* /*group*/ ) override {}

    bool multicast( const char* group, int msgClass, const std::string& msg ) override {
        bus_rx_data_t data;
        memset( &data, 0, sizeof( bus_rx_data_t ) );
        strcpy( data.sender, getPrivateName().c_str() );
        strcpy( data.msg, msg.c_str() );
        data.msg_size = msg.size();
        data.num_groups = 1;
        strcpy( data.target_groups[ 0 ], group );
        data.msg_type = msgClass;
        data.service_type = REGULAR_MESS;
        getRxCallback()( &data );
        return true;
    }

    bool isRegularMessage( int msgType ) const override {
        return true;
    }
};

class TestService : public Service {
public:
    using PluginId = Service::PluginId;
    using SubscriberAddedCallback = std::function< void( const PluginId& ) >;

    template < typename BusType = TestBus >
    TestService( const std::string& name,
                 BusWrapper::BusTypeTag< BusType > dummy = BusWrapper::BusTypeTag< BusType >() )
    : Service( name,
               std::make_unique< BusWrapper >( BusWrapper::BusTypeTag< BusType >{} ),
               std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

    void setSubscriberAddedCallback( SubscriberAddedCallback callback ) {
        m_subscriberAddedCallback = std::move( callback );
    }

private:
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override {
        return request;
    }

    void handleMessageFromBus( const Message& m, const std::string& ) override {
        dataFromBus = m;
    }

    void onSubscriberAdded( const PluginEventsSubscriber::PluginId& subscriber ) override {
        if( m_subscriberAddedCallback )
            m_subscriberAddedCallback( subscriber );
    }

public:
    Message dataFromBus;
    SubscriberAddedCallback m_subscriberAddedCallback;
};

class MockService : public TestService {
public:
    MockService()
    : TestService( "mock_service" ) {}

    MOCK_METHOD0( setUp, void() );
    MOCK_METHOD0( tearDown, void() );
};

}  // namespace

class ServiceTestSuite : public testing::Test {
public:
    ServiceTestSuite()
    : m_service( "test_service",
                 std::make_unique< Services::BusWrapper >(
                     Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
                 std::make_unique< HubPlatform::UT::FakeDbClient >() ) {}

    void SetUp() override {
        m_service.getDBClient().clean();
    }

    Service& getService() {
        return m_service;
    }

    const Service& getService() const {
        return m_service;
    }

    IDbClient& getDBClient() {
        return m_service.getDBClient();
    }

    const IDbClient& getDBClient() const {
        return m_service.getDBClient();
    }

    const Services::UT::FakeResponseBus& getTestBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( m_service.getBus() );
    }

    Service::SubscribersByPluginId& getPluginEventsSubscribers( const std::string& moduleName ) {
        return m_service.m_subscribers.process(
            [&moduleName]( Service::SubscribersByModuleName& subscribers ) -> Service::SubscribersByPluginId& {
                return subscribers[ moduleName ];
            } );
    }

private:
    Service m_service;
};

class FakeScriptEvent final : public Services::ScriptEvent {
public:
    FakeScriptEvent( const std::string& eventName, const Json& body )
    : Services::ScriptEvent( eventName )
    , m_body( body ) {}

    Json toJson() const override {
        auto js = Services::ScriptEvent::toJson();
        for( auto it = m_body.begin(); it != m_body.end(); it++ ) {
            js[ it.key() ] = it.value();
        }

        return js;
    }

private:
    const Json m_body;
};

TEST_F( ServiceTestSuite, send ) {
    TestService service( "test_service" );

    Message message;
    message.group = "logd";
    message.msgClass = LOG_EVENT;
    message.msgSubClass = HA_LOG_INFO;
    message.data = JSON( "data"
                         << "SomeTestData" );

    service.addTask( [&service, &message]() {
        service.send( message );
        service.addTask( [&service]() { service.stop(); } );
    } );

    std::thread worker( &TestService::run, &service );
    worker.join();

    EXPECT_EQ( message.data, service.dataFromBus.data );
    EXPECT_EQ( message.group, service.dataFromBus.group );
    EXPECT_EQ( message.msgClass, service.dataFromBus.msgClass );
}

TEST_F( ServiceTestSuite, setUp_tearDown ) {
    MockService service;

    {
        service.addTask( [&]() { service.stop(); } );

        EXPECT_CALL( service, setUp() );
        EXPECT_CALL( service, tearDown() );

        service.run();
    }

    {
        service.addTask( [&]() { throw "exception"; } );

        EXPECT_CALL( service, setUp() );
        EXPECT_CALL( service, tearDown() );

        EXPECT_THROW( service.run(), TaskLoop::TaskException );
    }
}

// Note: It is manual test. Can be runed only in dev environment.
TEST_F( ServiceTestSuite, DISABLED_sendRequest ) {
    TestService request_service( "requestd", BusWrapper::BusTypeTag< SpreadBus >() );
    TestService response_service( "responsed", BusWrapper::BusTypeTag< SpreadBus >() );

    JsonObject response = JSON_EMPTY_OBJ;
    JsonObject request = JSON( "data"
                               << "SomeTestData" );

    std::thread worker_1( &TestService::run, &request_service );
    std::thread worker_2( &TestService::run, &response_service );

    response_service.addTask( [&]() {
        request_service.addTask( [&]() {
            response = request_service.sendRequest( "responsed", request, CREATE_ROOM );
            request_service.addTask( [&request_service]() { request_service.stop(); } );
            response_service.addTask( [&response_service]() { response_service.stop(); } );
        } );
    } );

    worker_1.join();
    worker_2.join();

    EXPECT_EQ( response, request );
}

TEST_F( ServiceTestSuite, testSubscription ) {
    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );
    getService().subscribe( subscriber );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    ASSERT_EQ( 1U, subscribers.size() );
    ASSERT_EQ( 1U, subscribers[ pluginName ].size() );
    EXPECT_EQ( subscriber.toJson(), subscribers[ pluginName ][ scriptName ].toJson() );
}

TEST_F( ServiceTestSuite, testSubscriptionTheSameSubscriber ) {
    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );

    getService().subscribe( subscriber );
    getService().subscribe( subscriber );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    ASSERT_EQ( 1U, subscribers[ pluginName ].size() );
}

TEST_F( ServiceTestSuite, onSubscriberAddedHandlerTriggered ) {
    std::vector< TestService::PluginId > addedSubscribers;

    TestService testService( "testService" );
    testService.setSubscriberAddedCallback(
        [&addedSubscribers]( const TestService::PluginId& pluginId ) { addedSubscribers.push_back( pluginId ); } );

    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );
    testService.subscribe( subscriber );

    ASSERT_EQ( 1U, addedSubscribers.size() );
    const TestService::PluginId& addedPluginName = addedSubscribers[ 0 ];
    EXPECT_EQ( addedPluginName, pluginName );
}

TEST_F( ServiceTestSuite, onSubscriberAddedHandlerNotTriggeredForSameSubscriber ) {
    std::vector< TestService::PluginId > addedSubscribers;

    TestService testService( "testService" );
    testService.setSubscriberAddedCallback(
        [&addedSubscribers]( const TestService::PluginId& pluginId ) { addedSubscribers.push_back( pluginId ); } );

    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );
    testService.subscribe( subscriber );
    testService.subscribe( subscriber );

    ASSERT_EQ( 1U, addedSubscribers.size() );
}

TEST_F( ServiceTestSuite, testMultipleSubscription ) {
    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto scriptName2 = "riky2";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber2( "interpreterName", pluginName, moduleName, scriptName2, JSON_EMPTY_OBJ );

    getService().subscribe( subscriber );
    getService().subscribe( subscriber2 );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    ASSERT_EQ( 2U, subscribers[ pluginName ].size() );
}

TEST_F( ServiceTestSuite, testMultipleSubscriptionForDifferentPlugins ) {
    const auto pluginName = "kyky";
    const auto pluginName2 = "kyky2";
    const auto scriptName = "riky";
    const auto scriptName2 = "riky2";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber2( "interpreterName", pluginName2, moduleName, scriptName2, JSON_EMPTY_OBJ );

    getService().subscribe( subscriber );
    getService().subscribe( subscriber2 );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    ASSERT_EQ( 2U, subscribers.size() );
    ASSERT_EQ( 1U, subscribers[ pluginName ].size() );
    ASSERT_EQ( 1U, subscribers[ pluginName2 ].size() );
}

TEST_F( ServiceTestSuite, testMultipleSubscriptionForDifferentModules ) {
    const auto pluginName = "a";
    const auto scriptName1 = "b";
    const auto scriptName2 = "c";
    const auto moduleName1 = "d";
    const auto moduleName2 = "e";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName1, scriptName1, JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber2( "interpreterName", pluginName, moduleName2, scriptName2, JSON_EMPTY_OBJ );

    getService().subscribe( subscriber );
    getService().subscribe( subscriber2 );

    auto& subscribers1 = getPluginEventsSubscribers( moduleName1 );
    ASSERT_EQ( 1U, subscribers1.size() );
    ASSERT_EQ( 1U, subscribers1[ pluginName ].size() );
    ASSERT_NE( subscribers1[ pluginName ].end(), subscribers1[ pluginName ].find( scriptName1 ) );

    auto& subscribers2 = getPluginEventsSubscribers( moduleName2 );
    ASSERT_EQ( 1U, subscribers2.size() );
    ASSERT_EQ( 1U, subscribers2[ pluginName ].size() );
    ASSERT_NE( subscribers2[ pluginName ].end(), subscribers2[ pluginName ].find( scriptName2 ) );
}

TEST_F( ServiceTestSuite, testUnsubscription ) {
    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );

    getService().subscribe( subscriber );
    getService().unsubscribe( subscriber );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    ASSERT_EQ( 1U, subscribers.size() );
    ASSERT_EQ( 0U, subscribers[ pluginName ].size() );
    EXPECT_EQ( PluginEventsSubscriber{}.getId(), subscribers[ pluginName ][ scriptName ].getId() );
}

TEST_F( ServiceTestSuite, testChangeSubscriberFilterRules ) {
    const auto pluginName = "kyky";
    const auto scriptName = "riky";
    const auto moduleName = "asd";
    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, scriptName, JSON_EMPTY_OBJ );

    const auto subscriberId = json_get_str( getService().subscribe( subscriber ), "subscriberId" );

    const auto filterRules = PluginEventsSubscriber::createFilterRulesFromJson( { JSON( "event"
                                                                                        << "kyky" ),
                                                                                  JSON( "event"
                                                                                        << "riky" ) } );
    getService().changeSubscriberFilterRules( moduleName, subscriberId, filterRules );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    auto& eventFilterRules = subscribers[ pluginName ][ subscriberId ].getEventsFilterRules();

    EXPECT_EQ( 2U, eventFilterRules.size() );
}

TEST_F( ServiceTestSuite, testChangeExistentSubscriberFilterRule ) {
    const auto pluginName = "kyky";
    const auto ruleId = "rule1";
    const auto moduleName = "asd";
    JsonArray rules = { JSON( "id" << ruleId << "event"
                                   << "kyky" ) };

    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, "riky", JSON( "rules" << rules ) );

    const auto subscriberId = json_get_str( getService().subscribe( subscriber ), "subscriberId" );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    auto& eventFilterRules = subscribers[ pluginName ][ subscriberId ].getEventsFilterRules();

    EXPECT_EQ( "kyky", eventFilterRules[ ruleId ].m_eventName );

    const auto filterRules = PluginEventsSubscriber::createFilterRulesFromJson( { JSON( "id" << ruleId << "event"
                                                                                             << "riky" ) } );
    getService().changeSubscriberFilterRules( moduleName, subscriberId, filterRules );

    EXPECT_EQ( "riky", eventFilterRules[ ruleId ].m_eventName );
}

TEST_F( ServiceTestSuite, testChangeDeleteSubscriberFilterRule ) {
    const auto pluginName = "kyky";
    const auto ruleId = "rule1";
    const auto moduleName = "asd";
    JsonArray rules = { JSON( "id" << ruleId << "event"
                                   << "kyky" ) };

    PluginEventsSubscriber subscriber( "interpreterName", pluginName, moduleName, "riky", JSON( "rules" << rules ) );

    const auto subscriberId = json_get_str( getService().subscribe( subscriber ), "subscriberId" );

    auto& subscribers = getPluginEventsSubscribers( moduleName );
    auto& eventFilterRules = subscribers[ pluginName ][ subscriberId ].getEventsFilterRules();

    EXPECT_EQ( 1, eventFilterRules.size() );

    const auto filterRules = PluginEventsSubscriber::createFilterRulesFromJson( { JSON( "id" << ruleId << "event"
                                                                                             << "" ) } );
    getService().changeSubscriberFilterRules( moduleName, subscriberId, filterRules );

    EXPECT_EQ( 0, eventFilterRules.size() );
}

TEST( PluginEventsSubscriberTestSuite, testPassEveryEvent ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber( "interpreterName", "pluginName", "moduleName", "scriptName", JSON_EMPTY_OBJ );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassEveryEventsExplicit ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber( "interpreterName",
                                       "pluginName",
                                       "moduleName",
                                       "scriptName",
                                       JSON( "exclude" << true << "rules" << JSON_EMPTY_OBJ ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testBlockEveryEvents ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << JSON_EMPTY_OBJ ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testBlockEveryEventsExplicit ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    PluginEventsSubscriber subscriber( "interpreterName",
                                       "pluginName",
                                       "moduleName",
                                       "scriptName",
                                       JSON( "exclude" << false << "rules" << JSON_EMPTY_OBJ ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEvent ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    JsonArray filterRules{ JSON( "event"
                                 << "event" ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEventWithSeveralRules ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    JsonArray filterRules{ JSON( "event"
                                 << "kyky" ),
                           JSON( "event"
                                 << "event" ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEventWithArgs ) {
    FakeScriptEvent event( "event",
                           JSON( "arg"
                                 << "value" ) );
    JsonArray accepts{ JSON( "arg"
                             << "value" ) };
    JsonArray filterRules{ JSON( "event"
                                 << "event"
                                 << "accepts" << accepts ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEventWithSeveralArgs ) {
    FakeScriptEvent event( "event",
                           JSON( "arg"
                                 << "value"
                                 << "id"
                                 << "any_id" ) );
    JsonArray accepts{ JSON( "arg"
                             << "value"
                             << "id" << JSON_EMPTY_OBJ ) };
    JsonArray filterRules{ JSON( "event"
                                 << "event"
                                 << "accepts" << accepts ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEventWithSeveralAccepts ) {
    FakeScriptEvent event( "event",
                           JSON( "id"
                                 << "any_id" ) );
    JsonArray accepts{ JSON( "arg"
                             << "value" ),
                       JSON( "id" << JSON_EMPTY_OBJ ) };
    JsonArray filterRules{ JSON( "event"
                                 << "event"
                                 << "accepts" << accepts ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEventWithNoRuleArgs ) {
    FakeScriptEvent event( "event",
                           JSON( "arg"
                                 << "value" ) );
    JsonArray filterRules{ JSON( "event"
                                 << "event" ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testNotPassUnMatchedEvent ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    JsonArray filterRules{ JSON( "event"
                                 << "kyky" ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testNotPassMatchedEventWithNoEventArgs ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    JsonArray accepts{ JSON( "arg"
                             << "value" ) };
    JsonArray filterRules{ JSON( "event"
                                 << "kyky"
                                 << "accepts" << accepts ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testNotPassMatchedEventWithInvalidEventArgs ) {
    FakeScriptEvent event( "event",
                           JSON( "arg"
                                 << "riky" ) );
    JsonArray accepts{ JSON( "arg"
                             << "value" ) };
    JsonArray filterRules{ JSON( "event"
                                 << "kyky"
                                 << "accepts" << accepts ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassMatchedEventWithInvalidEventArgs1 ) {
    FakeScriptEvent event( "event",
                           JSON( "arg"
                                 << "kalue"
                                 << "id"
                                 << "any_id" ) );
    JsonArray accepts{ JSON( "arg"
                             << "value"
                             << "id" << JSON_EMPTY_OBJ ) };
    JsonArray filterRules{ JSON( "event"
                                 << "event"
                                 << "accepts" << accepts ) };
    PluginEventsSubscriber subscriber(
        "interpreterName", "pluginName", "moduleName", "scriptName", JSON( "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testPassUnMatchedEventExcludeMode ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    JsonArray filterRules{ JSON( "event"
                                 << "kyky" ) };
    PluginEventsSubscriber subscriber( "interpreterName",
                                       "pluginName",
                                       "moduleName",
                                       "scriptName",
                                       JSON( "exclude" << true << "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_TRUE( eventPassed );
}

TEST( PluginEventsSubscriberTestSuite, testNotPassMatchedEventExcludeMode ) {
    FakeScriptEvent event( "event", JSON_EMPTY_OBJ );
    JsonArray filterRules{ JSON( "event"
                                 << "event" ) };
    PluginEventsSubscriber subscriber( "interpreterName",
                                       "pluginName",
                                       "moduleName",
                                       "scriptName",
                                       JSON( "exclude" << true << "rules" << filterRules ) );

    bool eventPassed = false;
    auto notifyCb = [&eventPassed]( const Services::Message& ) { eventPassed = true; };

    subscriber.notify( event, notifyCb );
    EXPECT_FALSE( eventPassed );
}

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
