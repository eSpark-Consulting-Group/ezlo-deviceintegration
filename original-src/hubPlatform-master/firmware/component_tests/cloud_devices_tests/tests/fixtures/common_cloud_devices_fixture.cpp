#include "cloud_devices_tests/tests/fixtures/common_cloud_devices_fixture.h"
#include "test_utils/component_tests/component_test_application.h"

#include "config_reader/config_reader.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Tests {

void CommonCloudDevicesTests::SetUp() {
    nma().setRequestsProcessFunction( [this]( rpc::Message& message ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        m_requestMessages.push_back( message.getMessage() );
        lock.unlock();
        m_condVar.notify_all();
    } );
    CommonNmaTests::SetUp();
    m_cloudRequestTimeout = std::chrono::seconds( ConfigReader().getInt( "cloud_devices/cloud/requestTimeout" ) + 1 );
    m_currentVersion = -1;
}

void CommonCloudDevicesTests::TearDown() {
    // TEST_WAIT_NO_REQUESTS();
    nma().resetRequestsProcessFunction();
    CommonNmaTests::TearDown();
}

void CommonCloudDevicesTests::sendCloudRegistered() {
    nma().getDBClient().update( HaTempCacheDBName,
                                JSON_EMPTY_OBJ,
                                JSON( "_id"
                                      << "cache-cloud-status"
                                      << "registered" << true ),
                                true,
                                false );

    nma().sendBroadcast( CLOUD_REGISTERED, {} );
}

void CommonCloudDevicesTests::sendCloudUnregistered() {
    nma().getDBClient().update( HaTempCacheDBName,
                                JSON_EMPTY_OBJ,
                                JSON( "_id"
                                      << "cache-cloud-status"
                                      << "registered" << false ),
                                true,
                                false );

    nma().sendBroadcast( CLOUD_UNREGISTERED, {} );
}

void CommonCloudDevicesTests::timeoutResponse() {
    std::this_thread::sleep_for( m_cloudRequestTimeout );
}

void CommonCloudDevicesTests::setCurrentVersion( int version ) {
    m_currentVersion = version;
}

int CommonCloudDevicesTests::getCurrentVersion() const {
    return m_currentVersion;
}

void CommonCloudDevicesTests::increaseCurrentVersion() {
    m_currentVersion = ( m_currentVersion == -1 ) ? 1 : m_currentVersion + 1;
}

JsonObject CommonCloudDevicesTests::waitForRequest( const char* file, int line, std::chrono::seconds timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_requestMessages.empty() ) {
        if( !m_condVar.wait_for( lock, timeout, [this] { return !m_requestMessages.empty(); } ) ) {
            ADD_FAILURE_AT( file, line ) << "No requests received";
            throw std::exception();
        }
    }
    auto tmp = m_requestMessages.front();
    m_requestMessages.pop_front();
    return tmp;
}

JsonObject CommonCloudDevicesTests::waitForDbEntry( const std::string& dbName,
                                                    const Json& query,
                                                    const char* file,
                                                    int line,
                                                    std::chrono::milliseconds timeout ) {
    const auto waitTime = std::chrono::milliseconds( 10 );
    Json entry;
    for( int attempt = 0; attempt < timeout / waitTime; ++attempt ) {
        entry = nma().getDBClient().findOne( dbName, query, nullptr );
        if( !entry.empty() ) {
            break;
        }
        std::this_thread::sleep_for( waitTime );
    }
    if( entry.empty() ) {
        ADD_FAILURE_AT( file, line ) << "Expected database entry in " + dbName + " with " + query.dump( 4 );
        throw std::exception();
    }
    return entry;
}

void CommonCloudDevicesTests::checkNoRequestsReceived( const char* file, int line, std::chrono::milliseconds timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( !m_requestMessages.empty() ||
        m_condVar.wait_for( lock, timeout, [this] { return !m_requestMessages.empty(); } ) ) {
        for( auto request : m_requestMessages ) {
            ADD_FAILURE_AT( file, line ) << "Unexpected request received: " << cc::j( m_requestMessages.front() );
        }
        throw std::exception();
    }
}

void CommonCloudDevicesTests::sendResponse( const JsonObject& request, Json result, Json error, Json sender ) {
    Json response;
    response[ "id" ] = request[ "id" ];
    response[ "method" ] = request[ "method" ];
    if( request.count( "api" ) != 0 ) {
        response[ "api" ] = request[ "api" ];
    }
    response[ "result" ] = std::move( result );
    if( !error.empty() ) {
        response[ "error" ] = std::move( error );
    }
    if( !sender.empty() ) {
        response[ "sender" ] = std::move( sender );
    }
    nma().sendtoHub( response, ComponentTestApplication::ConnectionType::NMA );
}

void CommonCloudDevicesTests::sendBroadcast( const std::string& msgSubclass, JsonObject result, JsonObject sender ) {
    Json broadcast;
    broadcast[ "id" ] = "ui_broadcast";
    broadcast[ "msg_subclass" ] = msgSubclass;
    broadcast[ "result" ] = std::move( result );
    if( !sender.empty() ) {
        broadcast[ "sender" ] = std::move( sender );
    }
    nma().sendtoHub( broadcast, ComponentTestApplication::ConnectionType::NMA );
}

}  // namespace Tests
}  // namespace HubPlatform
