#include <gtest/gtest.h>
#include "test_utils/component_tests/mqtt_broker/mqtt_broker.h"
#include "test_utils/test_environment.h"
#include "system_utils/find_free_local_port.h"
#include "system_utils/process/shared_mutex.h"

#include "mqtt_client/mqtt_client.h"
#include "mqtt_client/mqtt_utils.h"

#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>

/*

TESTPLAN

DONE    1. Send/Receive tests
DONE        1.1 Send and receive string message
DONE        1.2 Send and receive bytearray message
DONE        1.3 Send and receive multiple messages

DONE    2. Subscribe/Unsubscribe tests
DONE        2.1 Subscribe and unsubscribe
DONE        2.2 Subscribe to multiple topics
DONE        2.3 Subscribe and publish to same topic

DONE    3. Connection tests
DONE        3.1 Reconnect client

*/

namespace HubPlatform {
namespace UT {
namespace {

class MessageEvent {
public:
    bool waitFor( std::chrono::milliseconds timeout );
    void received();
    void reset();

private:
    std::mutex m_mutex;
    std::atomic_bool m_isReceived{ false };
    std::condition_variable m_condition;
};

bool MessageEvent::waitFor( std::chrono::milliseconds timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_condition.wait_for( lock, timeout, [&] { return m_isReceived.load(); } );
    bool status = m_isReceived;
    m_isReceived = false;
    return status;
}

void MessageEvent::received() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_isReceived = true;
    lock.unlock();
    m_condition.notify_one();
}

void MessageEvent::reset() {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_isReceived = false;
}

void dummyCallback( HubPlatform::Common::MQTTMessage&& message ){};

}  // namespace

class MQTTClientTests : public ::testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Tests::MQTTBroker& getBroker();
    static bool& isBrokerRunning();
    static size_t getRandomBrokerPort();

    std::string getUniqueID();
    std::string getUniqueTopic();
};

Tests::MQTTBroker& MQTTClientTests::getBroker() {
    static Tests::MQTTBroker broker;
    return broker;
}

bool& MQTTClientTests::isBrokerRunning() {
    static bool isRunning = false;
    return isRunning;
}

size_t MQTTClientTests::getRandomBrokerPort() {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    auto port = system_utils::getUniqueFreeLocalPort();
    lock.unlock();
    return port;
}

void MQTTClientTests::SetUpTestCase() {
    using Status = Process::Forker::Status;
    Process::Forker::OnProcessCompletion onBrokerExit =
        []( pid_t, Status status, int exitCode, const std::string& error ) {
            if( MQTTClientTests::isBrokerRunning() ) {
                ADD_FAILURE() << "Broker is unexpectedly terminated: " << error << " : exit code = " << exitCode;
            }
        };

    try {
        getBroker().start( MQTTClientTests::getRandomBrokerPort(), std::move( onBrokerExit ) );
        isBrokerRunning() = true;
        return;
    }
    catch( const std::exception& e ) {
        ADD_FAILURE() << "Unable to setup broker: " << e.what();
    }
    catch( ... ) {
        ADD_FAILURE() << "Unable to setup broker: unknown exception";
    }
    exit( EXIT_FAILURE );
}

void MQTTClientTests::TearDownTestCase() {
    isBrokerRunning() = false;
    getBroker().stop();
}

std::string MQTTClientTests::getUniqueID() {
    static std::atomic_int pid( getpid() );
    static std::atomic_uint id{ 0 };
    return "id_" + std::to_string( pid ) + "_" + std::to_string( ++id );
}

std::string MQTTClientTests::getUniqueTopic() {
    return "topic_" + getUniqueID();
}

TEST_F( MQTTClientTests, sendAndReceiveStringMessage ) /* 1.1 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic = getUniqueTopic();
    const std::string payload = "payload";

    MQTTMessage message;
    MessageEvent event;

    auto messageCallback = [&message, &event]( MQTTMessage&& msg ) {
        message = std::move( msg );
        event.received();
    };

    MQTTClient clientTx;
    clientTx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", std::move( messageCallback ) );
    clientRx.subscribe( topic );

    clientTx.publish( topic, payload );

    EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );

    EXPECT_EQ( topic, message.topic );
    EXPECT_EQ( payload, MQTTUtils::getStrPayload( message ) );
}

TEST_F( MQTTClientTests, sendAndReceiveByteArrayMessage ) /* 1.2 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic = getUniqueTopic();

    MQTTMessage srcMessage{ topic, { 0, 1, 0, 2, 0 } };

    MQTTMessage message;
    MessageEvent event;

    auto messageCallback = [&message, &event]( MQTTMessage&& msg ) {
        message = std::move( msg );
        event.received();
    };

    MQTTClient clientTx;
    clientTx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", std::move( messageCallback ) );
    clientRx.subscribe( topic );

    clientTx.publish( srcMessage );
    EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );

    EXPECT_EQ( message.topic, srcMessage.topic );
    EXPECT_EQ( message.payload, srcMessage.payload );
}

TEST_F( MQTTClientTests, sendAndReceiveMultipleMessages ) /* 1.3 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic = getUniqueTopic();
    const size_t queueSize = 20;

    MessageEvent event;
    std::vector< MQTTMessage > messagePoll;
    messagePoll.reserve( queueSize );

    for( size_t i = 1; i <= queueSize; ++i )
        messagePoll.emplace_back( MQTTUtils::createMessage( topic, "payload_" + std::to_string( i ) ) );

    std::vector< MQTTMessage > messageQueue;
    messageQueue.reserve( messagePoll.size() );

    auto messageCallback = [&messageQueue, &event]( MQTTMessage&& msg ) {
        messageQueue.emplace_back( std::move( msg ) );
        event.received();
    };

    MQTTClient clientTx;
    clientTx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", std::move( messageCallback ) );
    clientRx.subscribe( topic );

    for( auto& msg : messagePoll ) {
        clientTx.publish( msg );
        EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );
    }

    ASSERT_EQ( messageQueue.size(), messagePoll.size() );
    for( size_t i = 0; i < messagePoll.size(); ++i ) {
        EXPECT_EQ( messageQueue[ i ].topic, messagePoll[ i ].topic );
        EXPECT_EQ( messageQueue[ i ].payload, messagePoll[ i ].payload );
    }
}

TEST_F( MQTTClientTests, subscribeAndUnsubscribe ) /* 2.1 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic = getUniqueTopic();

    MessageEvent event;
    std::vector< MQTTMessage > messageQueue;
    messageQueue.reserve( 2 );

    auto messageCallback = [&messageQueue, &event]( MQTTMessage&& msg ) {
        messageQueue.emplace_back( std::move( msg ) );
        event.received();
    };

    MQTTClient clientTx;
    clientTx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", std::move( messageCallback ) );

    clientTx.publish( topic, "payload1" );
    EXPECT_FALSE( event.waitFor( std::chrono::milliseconds( 500 ) ) );

    clientRx.subscribe( topic );
    clientTx.publish( topic, "payload2" );
    EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );

    clientRx.unsubscribe( topic );
    clientTx.publish( topic, "payload3" );
    EXPECT_FALSE( event.waitFor( std::chrono::milliseconds( 500 ) ) );

    ASSERT_EQ( messageQueue.size(), 1 );

    auto& msg = messageQueue.front();

    EXPECT_EQ( topic, msg.topic );
    EXPECT_EQ( "payload2", MQTTUtils::getStrPayload( msg ) );
}

TEST_F( MQTTClientTests, subscribeToMultipleTopics ) /* 2.2 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic1 = getUniqueTopic();
    const auto topic2 = getUniqueTopic();

    MessageEvent event;
    std::vector< MQTTMessage > messageQueue;
    messageQueue.reserve( 2 );

    auto messageCallback = [&messageQueue, &event]( MQTTMessage&& msg ) {
        messageQueue.emplace_back( std::move( msg ) );
        event.received();
    };

    MQTTClient clientTx;
    clientTx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", std::move( messageCallback ) );

    clientRx.subscribe( topic1 );
    clientRx.subscribe( topic2 );

    clientTx.publish( topic1, "payload1" );
    EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );

    clientTx.publish( topic2, "payload2" );
    EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );

    ASSERT_EQ( messageQueue.size(), 2 );

    auto& msg1 = messageQueue.front();
    auto& msg2 = messageQueue.back();

    EXPECT_EQ( topic1, msg1.topic );
    EXPECT_EQ( "payload1", MQTTUtils::getStrPayload( msg1 ) );

    EXPECT_EQ( topic2, msg2.topic );
    EXPECT_EQ( "payload2", MQTTUtils::getStrPayload( msg2 ) );
}

TEST_F( MQTTClientTests, subscribeAndPublishToSameTopic ) /* 2.3 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic = getUniqueTopic();

    MessageEvent event;
    std::vector< MQTTMessage > messageQueue;
    messageQueue.reserve( 2 );

    auto messageCallback = [&messageQueue, &event]( MQTTMessage&& msg ) {
        messageQueue.emplace_back( std::move( msg ) );
        if( messageQueue.size() > 1 )
            event.received();
    };

    MQTTClient clientTx;
    clientTx.connect( getUniqueID(), brokerURI, "", "", messageCallback );

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", messageCallback );

    clientRx.subscribe( topic );
    clientTx.subscribe( topic );

    clientTx.publish( topic, "payload" );
    EXPECT_TRUE( event.waitFor( std::chrono::seconds( 5 ) ) );

    ASSERT_EQ( messageQueue.size(), 2 );

    auto& msg1 = messageQueue.front();
    auto& msg2 = messageQueue.back();

    EXPECT_EQ( topic, msg1.topic );
    EXPECT_EQ( "payload", MQTTUtils::getStrPayload( msg1 ) );

    EXPECT_EQ( topic, msg2.topic );
    EXPECT_EQ( "payload", MQTTUtils::getStrPayload( msg2 ) );
}

TEST_F( MQTTClientTests, reconnectClient ) /* 3.1 */ {
    using namespace HubPlatform::Common;
    const auto brokerURI = getBroker().getURI();
    const auto topic = getUniqueTopic();

    MQTTClient clientRx;
    clientRx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );
    clientRx.subscribe( topic );

    clientRx.disconnect();

    clientRx.connect( getUniqueID(), brokerURI, "", "", dummyCallback );
    clientRx.subscribe( topic );
}

}  // namespace UT
}  // namespace HubPlatform
