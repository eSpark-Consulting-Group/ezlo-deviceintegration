#include "retry_command_manager/retry_command_manager.h"
#include "retry_command_manager/cloud_command.h"

#include "service_interfaces/tests/fixtures/fake_service_fixture.h"
#include "service_interfaces/tests/fakes/fake_service.h"

#include <queue>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Send commands without retries
DONE    2. Send commands with retries
DONE    2. Send commands with retries and finish with fail
DONE    3. Send command with fail and storing to DB
DONE    4. Send commands which were stored in DB
DONE    5. Calculate interval

*/

using namespace std;

namespace HubPlatform {
namespace UT {
const int MinInterval = 0;

const char* const CommandRetryStorage = "db.firmware.commands.send.retry.storage";
const int minIntervalSeconds = 5;
const int maxIntervalSeconds = 15;
const int persistentStorageIntervalSeconds = 0;

class FakeRetryHandlerService : public Services::UT::ServiceFixture, public Services::UT::FakeService {
public:
    void SetUp() override {
        createDatabase();
        Services::UT::ServiceFixture::SetUp();
    }

    FakeService& getHostService() final {
        return *this;
    }

    void initLogOptions() override{};
};

class RetryCommandManagerTests : public FakeRetryHandlerService {
public:
    static void SetUpTestCase() {
        FakeRetryHandlerService::SetUpTestCase();
        Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    }

    void TearDown() override {
        Services::UT::ServiceFixture::TearDown();
        EXPECT_TRUE( readStorage().empty() );
        thereNoTestEvents();
    }

    void testEvent( const Json& jsonData ) {
        {
            std::lock_guard< std::mutex > lock( m_lock );
            m_eventQueue.push( jsonData );
        }
        m_cond.notify_one();
    }

    Json waitForTestEvent( std::chrono::seconds timeout = std::chrono::seconds( 5 ) ) {
        std::unique_lock< std::mutex > lock( m_lock );
        if( !m_cond.wait_for( lock, timeout, [this] { return !m_eventQueue.empty(); } ) ) {
            return JSON_NULL_VAL;
        }

        Json receivedData = m_eventQueue.front();
        m_eventQueue.pop();
        return receivedData;
    }

    bool thereNoTestEvents() {
        Json event = waitForTestEvent();
        if( event == JSON_NULL_VAL )
            return true;
        else {
            LOGGER_INFO( "event: " + cc::j( event ) );
            return false;
        }
    }

    void emulateSendCommand( const Json& data ) {
        testEvent( JSON( "type"
                         << "send"
                         << "data" << data ) );
    }

    bool isSendCommand() {
        auto data = waitForTestEvent();
        return data[ "type" ] == "send";
    }

    void emulateReceiveCommand( const Json& data ) {
        testEvent( JSON( "type"
                         << "receive"
                         << "data" << data ) );
    }

    void emulateSuccessEvent( const Json& data ) {
        testEvent( JSON( "type"
                         << "success"
                         << "data" << data ) );
    }

    void emulateFailEvent( const Json& data ) {
        testEvent( JSON( "type"
                         << "fail"
                         << "data" << data ) );
    }

    bool isReceiveCommand() {
        auto data = waitForTestEvent();
        return data[ "type" ] == "receive";
    }

    static Json testCommand( const std::string& name = "name" ) {
        return JSON( "command" << name );
    }

    bool isSuccess( const std::string& name = "name" ) {
        auto data = waitForTestEvent();
        return data[ "type" ] == "success" && data[ "data" ][ "command" ] == name;
    }

    bool isFailNotification() {
        auto data = waitForTestEvent();
        return data[ "type" ] == "fail";
    }

    JsonArray readStorage() {
        return getHostService().getDBClient().findAll( CommandRetryStorage, JSON_EMPTY_OBJ, nullptr );
    }

private:
    std::mutex m_lock;
    std::condition_variable m_cond;
    std::queue< Json > m_eventQueue;
};

TEST_F( RetryCommandManagerTests, sendCommandsWithoutRetries ) {
    const int maxRetryNumber = 10;
    const int persistentStorageIntervalSeconds = 10;

    std::shared_ptr< RetryCommandManager > commandManager =
        std::make_shared< RetryCommandManager >( "ClassId",
                                                 std::chrono::seconds( minIntervalSeconds ),
                                                 std::chrono::seconds( maxIntervalSeconds ),
                                                 std::chrono::seconds( persistentStorageIntervalSeconds ),
                                                 getHostService(),
                                                 true );

    shared_ptr< CloudCommand > command1 =
        make_shared< CloudCommand >( maxRetryNumber,
                                     testCommand( "command1" ),
                                     [&]( const Json& sendData ) {
                                         emulateSendCommand( sendData );
                                         command1->receive( true );
                                         commandManager->checkResult();
                                     },
                                     [&]( const Json& sendData ) { emulateSuccessEvent( sendData ); },
                                     []( const Json& sendData ) { FAIL() << "Error call notify handler"; } );

    commandManager->sendCommand( command1 );

    shared_ptr< CloudCommand > command2 =
        make_shared< CloudCommand >( maxRetryNumber,
                                     testCommand( "command2" ),
                                     [&]( const Json& sendData ) {
                                         emulateSendCommand( sendData );
                                         ;
                                         command2->receive( true );
                                         commandManager->checkResult();
                                     },
                                     [&]( const Json& sendData ) { emulateSuccessEvent( sendData ); },
                                     []( const Json& sendData ) { FAIL() << "Error call notify handler"; } );
    commandManager->sendCommand( command2 );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSuccess( "command1" ) );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSuccess( "command2" ) );
}

TEST_F( RetryCommandManagerTests, sendCommandsWithRetries ) {
    const int maxRetryNumber = 10;
    const int persistentStorageIntervalSeconds = 10;

    std::shared_ptr< RetryCommandManager > commandManager =
        std::make_shared< RetryCommandManager >( "ClassId",
                                                 std::chrono::seconds( minIntervalSeconds ),
                                                 std::chrono::seconds( maxIntervalSeconds ),
                                                 std::chrono::seconds( persistentStorageIntervalSeconds ),
                                                 getHostService(),
                                                 true );

    shared_ptr< CloudCommand > command1 =
        make_shared< CloudCommand >( maxRetryNumber,
                                     testCommand( "command1" ),
                                     [&]( const Json& sendData ) {
                                         emulateSendCommand( sendData );
                                         std::async( std::launch::async, [this, command1, sendData, &commandManager] {
                                             auto data = JSON_EMPTY_OBJ;
                                             static int count = 0;
                                             if( count++ < 2 ) {
                                                 command1->fail();
                                                 commandManager->checkResult();
                                             }
                                             else {
                                                 emulateReceiveCommand( data );
                                                 command1->receive( true );
                                                 commandManager->checkResult();
                                             }
                                         } );
                                     },
                                     [&]( const Json& sendData ) { emulateSuccessEvent( sendData ); },
                                     []( const Json& sendData ) { FAIL() << "Error call notify handler"; } );

    commandManager->sendCommand( command1 );
    commandManager->sendCommand( command1 );

    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isReceiveCommand() );
    EXPECT_TRUE( isSuccess( "command1" ) );

    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isReceiveCommand() );
    EXPECT_TRUE( isSuccess( "command1" ) );
}

TEST_F( RetryCommandManagerTests, sendCommandsWithRetriesFailed ) {
    const int maxRetryNumber = 3;

    std::shared_ptr< RetryCommandManager > commandManager =
        std::make_shared< RetryCommandManager >( "ClassId",
                                                 std::chrono::seconds( minIntervalSeconds ),
                                                 std::chrono::seconds( maxIntervalSeconds ),
                                                 std::chrono::seconds( persistentStorageIntervalSeconds ),
                                                 getHostService(),
                                                 true );

    shared_ptr< CloudCommand > command1 =
        make_shared< CloudCommand >( maxRetryNumber,
                                     testCommand( "command1" ),
                                     [&]( const Json& sendData ) {
                                         emulateSendCommand( sendData );
                                         auto data = JSON_EMPTY_OBJ;
                                         static int count = 0;
                                         if( count++ < 6 ) {
                                             command1->fail();
                                             commandManager->checkResult();
                                         }
                                         else {
                                             emulateReceiveCommand( data );
                                             commandManager->checkResult();
                                         }
                                     },
                                     [&]( const Json& sendData ) { emulateSuccessEvent( sendData ); },
                                     [&]( const Json& sendData ) { emulateFailEvent( sendData ); } );

    commandManager->sendCommand( command1 );

    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isFailNotification() );
}

TEST_F( RetryCommandManagerTests, sendCommandWithFailAndStoringToDb ) {
    const int maxRetryNumber = 10;
    const int persistentStorageIntervalSeconds = 0;

    std::shared_ptr< RetryCommandManager > commandManager =
        std::make_shared< RetryCommandManager >( "ClassId",
                                                 std::chrono::seconds( minIntervalSeconds ),
                                                 std::chrono::seconds( maxIntervalSeconds ),
                                                 std::chrono::seconds( persistentStorageIntervalSeconds ),
                                                 getHostService(),
                                                 true );

    shared_ptr< CloudCommand > command1 = make_shared< CloudCommand >(
        maxRetryNumber,
        testCommand( "command1" ),
        [&]( const Json& sendData ) {
            emulateSendCommand( sendData );
            auto data = JSON_EMPTY_OBJ;
            static int count = 0;
            if( count == 3 ) {
                auto expected = R"({"command":{"command":"command1"},"commandClassId":"ClassId","retriesNumber":7})";
                auto record = readStorage().front();
                EXPECT_TRUE( !record[ "_id" ].empty() );
                record.erase( record.find( "_id" ) );
                EXPECT_EQ( record.dump(), expected );
            }

            if( count++ < 4 ) {
                command1->fail();
                commandManager->checkResult();
            }
            else {
                emulateReceiveCommand( data );
                command1->receive( true );
                commandManager->checkResult();
            }
        },
        [&]( const Json& sendData ) { emulateSuccessEvent( sendData ); },
        [&]( const Json& sendData ) { emulateFailEvent( sendData ); } );

    commandManager->sendCommand( command1 );

    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isReceiveCommand() );
    EXPECT_TRUE( isSuccess( "command1" ) );
    EXPECT_TRUE( readStorage().empty() );

    auto record = readStorage();
}

TEST_F( RetryCommandManagerTests, loadFromStorageAndSend ) {
    const auto record1 = R"({"_id":"1","command":{"command":"command1"},"commandClassId":"ClassId","retriesNumber":3})";
    const auto record2 = R"({"_id":"2","command":{"command":"command2"},"commandClassId":"ClassId","retriesNumber":2})";

    getHostService().getDBClient().insert( CommandRetryStorage, Json::parse( record1 ) );
    getHostService().getDBClient().insert( CommandRetryStorage, Json::parse( record2 ) );

    auto record = readStorage();

    std::shared_ptr< RetryCommandManager > commandManager =
        std::make_shared< RetryCommandManager >( "ClassId",
                                                 std::chrono::seconds( minIntervalSeconds ),
                                                 std::chrono::seconds( maxIntervalSeconds ),
                                                 std::chrono::seconds( persistentStorageIntervalSeconds ),
                                                 getHostService(),
                                                 true );

    std::vector< Json > commandsData = commandManager->loadCommandsDataFromStorage();

    for( auto& commandData : commandsData ) {
        shared_ptr< CloudCommand > command =
            make_shared< CloudCommand >( commandData,
                                         [&]( const Json& sendData ) {
                                             emulateSendCommand( sendData );
                                             auto data = JSON_EMPTY_OBJ;
                                             static int count = 0;
                                             if( count++ < 1 ) {
                                                 command->fail();
                                                 commandManager->checkResult();
                                             }
                                             else {
                                                 emulateReceiveCommand( data );
                                                 command->receive( true );
                                                 commandManager->checkResult();
                                             }
                                         },
                                         [&]( const Json& sendData ) { emulateSuccessEvent( sendData ); },
                                         [&]( const Json& sendData ) { emulateFailEvent( sendData ); } );

        commandManager->sendCommand( command );
    }
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isReceiveCommand() );
    EXPECT_TRUE( isSuccess( "command1" ) );

    EXPECT_TRUE( isSendCommand() );
    EXPECT_TRUE( isReceiveCommand() );
    EXPECT_TRUE( isSuccess( "command2" ) );
}

TEST_F( RetryCommandManagerTests, calculateInterval ) {
    auto interval = RetryCommandManager::calcRepeatInterval( 1 );
    EXPECT_EQ( interval.count(), 15 );
    interval = RetryCommandManager::calcRepeatInterval( 2 );
    EXPECT_EQ( interval.count(), 45 );
    interval = RetryCommandManager::calcRepeatInterval( 3 );
    EXPECT_EQ( interval.count(), 90 );
    interval = RetryCommandManager::calcRepeatInterval( 4 );
    EXPECT_EQ( interval.count(), 150 );
    interval = RetryCommandManager::calcRepeatInterval( 5 );
    EXPECT_EQ( interval.count(), 210 );
    interval = RetryCommandManager::calcRepeatInterval( 6 );
    EXPECT_EQ( interval.count(), 300 );

    for( int i = 7; i < 100; i++ ) {
        interval = RetryCommandManager::calcRepeatInterval( i );
        EXPECT_EQ( interval.count(), 300 + ( i - 6 ) * 90 );
    }
}

}  // namespace UT
}  // namespace HubPlatform
