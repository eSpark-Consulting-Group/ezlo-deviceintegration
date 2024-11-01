#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/cored_resources.h"
#include "test_utils/component_tests/http_server/http_server.h"

#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"

#include "jsonrpc/message.h"
#include "json_utils/json_fwd.h"
#include <deque>
#include <string>
#include <mutex>
#include <atomic>

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. lua http module
DONE        1.1 create GET request, run http.request(), last=true
DONE            1.1.1 create GET request, run http.request(), last=true, user_data=null
DONE            1.1,2 create GET request, run http.request(), last=true, user_data=<string>
DONE            1.1.3 create GET request, run http.request(), last=true, user_data_<object>
DONE        1.2 create POST request, run http.request(), last=true
DONE            1.2.1  post with no Content-Length header, expect timeout
DONE            1.2.2  post with Content-Length: 0; should be ok
DONE        1.3 create POST request, run http.request(), last=false, catch http_data_send event, send data with
http.data_send() from handler.
DONE        1.4 create POST request, run http.request(), last=true, catch http_data_received event, send another
    request with http.repeat_request() from handler.

DONE        1.5 Create GET request with certificates

DONE    2. Use lua http module from multiple plugins
DONE        2.1 Call http module from http_test, then from network_dhcp plugin

DONE    3. Download binary file

DONE    4. Multiple headers
DONE        4.1 Multiple headers: false
DONE        4.2 Multiple headers: true
DONE        4.3 Multiple headers from redirect chain

*/

namespace HubPlatform {
namespace Tests {

class HttpLuadTests : public CoreNmaTest {
public:
    void waitForDataBroadcasts( unsigned dataBroadcasts, unsigned totalBroadcasts );
    void assertEventBroadcast( const Json& dataBroadcast,
                               const std::string& requestId,
                               const std::string& eventName,
                               std::string pluginName = "",
                               const Json& userData = nullptr,
                               int codeLine = -1 );
    void assertUserData( const Json& eventUserdata, const Json& expected, int codeLine );

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;

protected:
    static unsigned& getPort() {
        static unsigned port = 8000;
        return port;
    }
    static unsigned& getSecurePort() {
        static unsigned port = 8443;
        return port;
    }
    static std::string hostName;

    static std::string makeHostName() {
        return hostName + ":" + std::to_string( getPort() );
    }

    static std::string makeUrl( const char* path, const char* hostName = "", const char* proto = "" ) {
        if( !proto || !*proto )
            proto = "http";
        if( !path || !*path )
            path = "/";
        std::stringstream os;
        os << proto << "://" << ( hostName && *hostName ? hostName : makeHostName() ) << path;
        return os.str();
    }

    static HttpServer& getHttpServer() {
        static HttpServer lws;
        return lws;
    }

protected:
    static std::string m_pluginName;

    std::string m_requestId;
    std::chrono::seconds m_shortWait{ 5 };

    std::vector< Json > m_broadcasts;
    std::vector< Json > m_progressBroadcasts;
};

std::string HttpLuadTests::m_pluginName = "http_test";
std::string HttpLuadTests::hostName = "localhost";

// resources
static char httpRequestScript[] = "HUB:http_test/scripts/http_request";
static char httpEventScript[] = "HUB:http_test/scripts/http_events";
static char httpRepeatRequestScript[] = "HUB:http_test/scripts/http_repeat_request";
static char httpDataSendScript[] = "HUB:http_test/scripts/http_data_send";
static char httpBinaryScript[] = "HUB:http_test/scripts/http_binary_data";

void HttpLuadTests::assertEventBroadcast( const Json& dataBroadcast,
                                          const std::string& requestId,
                                          const std::string& eventName,
                                          std::string pluginName,
                                          const Json& userData,
                                          int codeLine ) {
    std::string message = getCodeLineString( codeLine );
    Json result;
    auto brDump = json_write( dataBroadcast );
    ASSERT_TRUE( json_get_obj( dataBroadcast, "result", result ) ) << brDump << ": " << message;
    if( pluginName.empty() )
        pluginName = m_pluginName;
    EXPECT_EQ( json_get_str( result, "plugin" ), pluginName ) << brDump << ": " << message;
    EXPECT_EQ( json_get_str( result, "event" ), eventName ) << brDump << ": " << message;

    Json data;
    if( "http_data_received" == eventName ) {
        ASSERT_TRUE( json_get_obj( result, "data", data ) ) << brDump << ": " << message;
        EXPECT_EQ( json_get_int( data, "code" ), 200 ) << brDump << ": " << message;
        EXPECT_EQ( json_get_str( data, "id" ), requestId ) << brDump << ": " << message;
    }
    else if( "http_connection_closed" == eventName ) {
        Json reason;
        ASSERT_TRUE( json_get_obj( result, "data", data ) ) << brDump << ": " << message;
        ASSERT_TRUE( json_get_obj( data, "reason", reason ) ) << brDump << ": " << message;
        ASSERT_TRUE( reason.is_object() ) << brDump << ": " << message;
        ASSERT_EQ( json_get_int( reason, "code" ), 0 ) << brDump << ": " << message;

        int httpStatusCode = json_get_int( data, "code", 0 );
        ASSERT_NE( httpStatusCode, 0 ) << "httpStatusCode should not be zero " << message;
        auto codeStatus = json_get_str( data, "code_status" );
        ASSERT_FALSE( codeStatus.empty() ) << "code_status should not be empty" << message;
    }

    if( !data.empty() && !userData.empty() ) {
        Json eventUserData;
        ASSERT_TRUE( json_get( data, "user_data", eventUserData ) ) << "Expected to have user_data field. " << message;
        assertUserData( eventUserData, userData, codeLine );
    }
}

void HttpLuadTests::assertUserData( const Json& eventUserData, const Json& expected, int codeLine ) {
    if( expected == nullptr )
        return;
    ASSERT_EQ( eventUserData.type(), expected.type() )
        << "Types of 'user_data' fields should match! Event.user_data: " << eventUserData.dump()
        << ", expected user_data: " << expected << ", at " << getCodeLineString( codeLine );
    ASSERT_EQ( eventUserData, expected ) << "Contents of 'user_data' fields should match, "
                                         << ", at " << getCodeLineString( codeLine );
}

void HttpLuadTests::waitForDataBroadcasts( unsigned dataBroadcasts, unsigned totalBroadcasts ) {
    // wait for broadcasts: ui_broadcast/value, progress/started, progress/finished
    const char* progressStatuses[] = { "started", "finished" };
    const unsigned progressStatusesSize = sizeof( progressStatuses ) / sizeof( progressStatuses[ 0 ] );
    unsigned currentProgress = 0;

    for( unsigned i = 0; i < totalBroadcasts && m_broadcasts.size() < dataBroadcasts; i++ ) {
        // read three broadcasts
        auto broadcast =
            waitForAnyOfBroadcasts( { "hub.extensions.plugin.run.progress", "hub.extensions.plugin.ui_broadcast" } );
        auto dump = json_write( broadcast );

        ASSERT_EQ( broadcast.type(), Json::value_t::object );

        const auto& msgClass = json_get_str( broadcast, "msg_subclass" );
        if( msgClass == "hub.extensions.plugin.run.progress" ) {
            // started/finished check
            const char* statusToCheck = progressStatuses[ currentProgress ];
            checkExtensionsPluginRunProgressBroadcast( broadcast, statusToCheck, __LINE__ );

            currentProgress = ( currentProgress + 1 ) % progressStatusesSize;
        }
        else if( msgClass == "hub.extensions.plugin.ui_broadcast" ) {
            // value broadcast check here
            ASSERT_TRUE( broadcast.count( "result" ) );
            ASSERT_TRUE( broadcast[ "result" ].is_object() );

            std::string eventName;
            if( m_requestId.empty() ) {
                m_requestId = json_get_str( broadcast[ "result" ], "value" );
                ASSERT_FALSE( m_requestId.empty() ) << "ConnectionID cannot be empty: " << json_write( broadcast );
            }
            else if( broadcast.count( "result" ) > 0 && broadcast[ "result" ].count( "source" ) > 0 ) {
                ASSERT_EQ( json_get_str( broadcast[ "result" ], "source" ), "http_repeat_request event handler" );
                i--;
                continue;  // not an event
            }
            else {
                ASSERT_NO_THROW( eventName = broadcast.at( "result" ).at( "event" ).get< std::string >() )
                    << json_write( broadcast );
            }
            m_broadcasts.emplace_back( std::move( broadcast ) );
            if( eventName == "http_connection_closed" ) {
                break;
            }
        }
    }
}

void HttpLuadTests::SetUpTestCase() {
    LOGGER_DEBUG( "getUniqueFreeLocalPort HttpLuadTests pending..." );

    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::lock_guard< Process::SharedMutex > lock( processMutex );

    LOGGER_DEBUG( "getUniqueFreeLocalPort HttpLuadTests" );

    // Check and start static test HTTP server on port 8000 if needed
#ifndef TESTS_SKIP_HTTP_RANDOM_PORT
    {
        getPort() = system_utils::getUniqueFreeLocalPort();

        unsigned securePort = 0;
        try {
            securePort = system_utils::getUniqueFreeLocalPort();
        }
        catch( const std::exception& e ) {
            securePort = system_utils::getUniqueFreeLocalPort();
        }

        getSecurePort() = securePort;
    }
#endif

    auto& server = getHttpServer();
    server.setPort( getPort() );
    server.setSecurePort( getSecurePort() );

    // server.configure();
    server.start();
}

void HttpLuadTests::TearDownTestCase() {
    auto& server = getHttpServer();
    server.stop();
}

void HttpLuadTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

TEST_F( HttpLuadTests, httpRequestGET_userDataNull ) /* 1.1.1 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;
    Json paramsUserData = nullptr;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/get?suite=HttpLuadTests&method=httpRequestGET" );
    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "timeoutMs" ] = 3000;
    scriptParams[ "user_data" ] = paramsUserData;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    // check type and contents of a 'user_data' field
    Json jUserData;
    ASSERT_FALSE( json_get( broadcastConnectionClosed, { "result", "data", "user_data" }, jUserData ) );
}

TEST_F( HttpLuadTests, httpRequestGET_userDataString ) /* 1.1.2 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;
    Json paramsUserData = "user_data is a simple string <-------- !!!!!!!!!!!!!!!!!!! -------->";

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] =
        makeUrl( std::string{ "/reflect/get?suite=HttpLuadTests&method=" }.append( test_info_->name() ).c_str() );
    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "timeoutMs" ] = 3000;
    scriptParams[ "user_data" ] = paramsUserData;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    // check type and contents of a 'user_data' field
    Json jUserData;
    ASSERT_TRUE( json_get( broadcastConnectionClosed, { "result", "data", "user_data" }, jUserData ) );
    ASSERT_EQ( jUserData.type(), paramsUserData.type() )
        << "'user_data' field should have the same type as a passed to a script one!";
    ASSERT_EQ( jUserData, paramsUserData );
}

TEST_F( HttpLuadTests, httpRequestGET_userDataObject ) /* 1.1.3 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;
    Json paramsUserData = { { "payload", "user_data is a simple string <-------- !!!!!!!!!!!!!!!!!!! -------->" } };

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/get?suite=HttpLuadTests&method=httpRequestGET" );
    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "timeoutMs" ] = 3000;
    scriptParams[ "user_data" ] = paramsUserData;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    // check type and contents of a 'user_data' field
    Json jUserData;
    ASSERT_TRUE( json_get( broadcastConnectionClosed, { "result", "data", "user_data" }, jUserData ) );
    ASSERT_EQ( jUserData.type(), paramsUserData.type() )
        << "'user_data' field should have the same type as a passed to a script one!";
    ASSERT_EQ( jUserData, paramsUserData );
}

TEST_F( HttpLuadTests, postNoContentLength ) /* 1.2.1 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/post?suite=HttpLuadTests&method=postNoContentLength" );
    scriptParams[ "type" ] = "POST";
    scriptParams[ "headers" ] = Json( {
        { "expect", "" },
    } );
    scriptParams[ "timeout" ] = 2000;
    scriptParams[ "verbose" ] = true;

    scriptParams[ "handler" ] = httpEventScript;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 2, 4 );

    ASSERT_EQ( m_broadcasts.size(), 2 );
    {
        Json result, data, reason;
        const Json& broadcast = m_broadcasts[ 1 ];
        ASSERT_TRUE( json_get_obj( broadcast, "result", result ) );
        ASSERT_TRUE( json_get_obj( result, "data", data ) );
        ASSERT_TRUE( json_get_obj( data, "reason", reason ) );
        ASSERT_TRUE( reason.is_object() );
        ASSERT_EQ( json_get_int( reason, "code" ), 6 ) << "Timeout expected, code = 6";
    }
}

TEST_F( HttpLuadTests, postContentLengthZero ) /* 1.2.2 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/post?suite=HttpLuadTests&method=postContentLengthZero" );
    scriptParams[ "type" ] = "POST";
    scriptParams[ "headers" ] = Json( { { "expect", "" }, { "content-length", 0 } } );

    scriptParams[ "handler" ] = httpEventScript;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );

    ASSERT_EQ( m_broadcasts.size(), 3 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    assertEventBroadcast( m_broadcasts[ 2 ], m_requestId, "http_connection_closed", "", nullptr, __LINE__ );
}

// TODO: enable it after POST will be implemented in http-server.
TEST_F( HttpLuadTests, DISABLED_httpRequestPOSTWithDataSend ) /* 1.3 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/post?suite=HttpLuadTests&method=httpRequestPOSTWithDataSend" );
    scriptParams[ "type" ] = "POST";
    scriptParams[ "headers" ] = Json( { { "expect", "" } } );

    scriptParams[ "handler" ] = httpDataSendScript;
    scriptParams[ "last" ] = false;
    scriptParams[ "data" ] = "Main POST request data, sent from test body\n";
    scriptParams[ "content_type" ] = "text/plain";
    // data_send() tests with lws test server works only with fixed content-length header. with postman works and
    // without it.
    scriptParams[ "content_length" ] = 80;

    scriptParams[ "keep_alive" ] = false;
    scriptParams[ "repeat_allowed" ] = false;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    std::string expectedValue;
    waitForDataBroadcasts( 4, 4 + 2 * 3 );

    ASSERT_EQ( m_broadcasts.size(), 4 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_send", "", nullptr, __LINE__ );
    assertEventBroadcast( m_broadcasts[ 2 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    assertEventBroadcast( m_broadcasts[ 3 ], m_requestId, "http_connection_closed", "", nullptr, __LINE__ );
}

TEST_F( HttpLuadTests, postRepeatRequest ) /* 1.4 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/post?suite=HttpLuadTests&method=postRepeatRequest" );
    //    scriptParams[ "url" ] =
    //    "https://postman-echo.com/post?suite=HttpLuadTests&method=postRepeatRequest";
    scriptParams[ "type" ] = "POST";
    scriptParams[ "headers" ] = Json( { { "expect", "" }, { "transfer-encoding", "chunked" } } );
    scriptParams[ "handler" ] = httpRepeatRequestScript;
    std::string payload = "Main POST request data, sent from test body\n";
    scriptParams[ "data" ] = payload;
    scriptParams[ "content_length" ] = payload.size();
    scriptParams[ "user_data" ] = "1";
    scriptParams[ "keep_alive" ] = true;
    scriptParams[ "repeat_allowed" ] = true;  // default value: true; just to highlight

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    const unsigned expectedBroadcasts = 6;  // value + 5 data_received
    waitForDataBroadcasts( expectedBroadcasts, expectedBroadcasts + 2 * expectedBroadcasts );

    ASSERT_EQ( m_broadcasts.size(), expectedBroadcasts );
    for( auto i = 1; i < expectedBroadcasts; ++i ) {
        auto userData = std::to_string( i );
        assertEventBroadcast(
            m_broadcasts[ i ], m_requestId, "http_data_received", m_pluginName, Json( userData ), __LINE__ );
    }
    // no connection_closed event, keep-alive
    // assertEventBroadcast( m_broadcasts[ 6 ], m_requestId, "http_connection_closed", "", nullptr, __LINE__ );
}

TEST_F( HttpLuadTests, httpRequestGET_withCertsCa ) /* 1.5 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;
    Json paramsUserData = nullptr;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/reflect/get?suite=HttpLuadTests&method=httpRequestGET_withCertsCa" );
    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "timeoutMs" ] = 3000;
    scriptParams[ "user_data" ] = paramsUserData;
    scriptParams[ "certs" ] = { { "public", "HUB:http_test/certs/server.ca" } };

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    // check type and contents of a 'user_data' field
    Json jUserData;
    ASSERT_FALSE( json_get( broadcastConnectionClosed, { "result", "data", "user_data" }, jUserData ) );
}

TEST_F( HttpLuadTests, useHttpModuleFromTwoPlugins ) /* 2.1 */ {
    // call the first plugin
    {
        auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );
        Json userData = { { "from_plugin", m_pluginName } };

        Json scriptParams = JSON_EMPTY_OBJ;

        scriptParams[ "command" ] = "request";
        scriptParams[ "url" ] = makeUrl( "/reflect/get?suite=HttpLuadTests&method=firstPluginGET" );
        scriptParams[ "handler" ] = httpEventScript;
        scriptParams[ "timeoutMs" ] = 3000;
        scriptParams[ "user_data" ] = userData;

        request[ "params" ][ "scriptParams" ] = scriptParams;

        auto dump = request.dump();
        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );

        auto prevBroadcasts = m_broadcasts.size();
        auto prevProgressBroadcasts = m_progressBroadcasts.size();
        waitForDataBroadcasts(
            prevBroadcasts + 3,
            prevProgressBroadcasts + 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

        Log::trace( "first assert results: broadcasts: " + cc::num10( m_broadcasts.size() ) );
        ASSERT_EQ( m_broadcasts.size(), prevBroadcasts + 3 );
        // m_broadcasts[0] - broadcast with "value", no "event" field
        assertEventBroadcast(
            m_broadcasts[ prevBroadcasts + 1 ], m_requestId, "http_data_received", m_pluginName, userData, __LINE__ );
        assertEventBroadcast( m_broadcasts[ prevBroadcasts + 2 ],
                              m_requestId,
                              "http_connection_closed",
                              m_pluginName,
                              userData,
                              __LINE__ );

        Log::trace( "first end" );
        m_requestId.clear();
    }

    // call from second plugin
    {
        char requestScript[] = "HUB:network_dhcp/scripts/http_request";
        char eventScript[] = "HUB:network_dhcp/scripts/dhcp_events";
        auto pluginName = "network_dhcp";
        Json userData = { { "from_plugin", pluginName } };

        auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", requestScript } } );

        Json scriptParams = JSON_EMPTY_OBJ;

        scriptParams[ "command" ] = "request";
        scriptParams[ "url" ] = makeUrl( "/reflect/get?suite=HttpLuadTests&method=secondPluginGet" );
        scriptParams[ "handler" ] = eventScript;
        scriptParams[ "timeoutMs" ] = 3000;
        scriptParams[ "user_data" ] = userData;

        request[ "params" ][ "scriptParams" ] = scriptParams;

        auto dump = request.dump();
        Log::trace( "second make request: " + cc::j( request ) );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        auto error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error.dump( 4 );

        auto prevBroadcasts = m_broadcasts.size();
        auto prevProgressBroadcasts = m_progressBroadcasts.size();

        Log::trace( cc::trace( "second wait for broadcasts " ) + cc::num10( prevBroadcasts ) );
        waitForDataBroadcasts(
            prevBroadcasts + 3,
            prevProgressBroadcasts + 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)
        Log::trace( cc::trace( "second assert results: broadcasts: " ) + cc::num10( m_broadcasts.size() ) );

        ASSERT_EQ( m_broadcasts.size(), prevBroadcasts + 3 );
        // m_broadcasts[3] - broadcast with "value", no "event" field
        assertEventBroadcast(
            m_broadcasts[ prevBroadcasts + 1 ], m_requestId, "http_data_received", pluginName, userData, __LINE__ );
        assertEventBroadcast(
            m_broadcasts[ prevBroadcasts + 2 ], m_requestId, "http_connection_closed", pluginName, userData, __LINE__ );

        Log::trace( "second end" );
        m_requestId.clear();
    }
}

TEST_F( HttpLuadTests, downloadBinaryFile ) /* 3 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( "/img/ezlologo.png?suite=HttpLuadTests&method=downloadBinaryFile" );
    scriptParams[ "handler" ] = httpBinaryScript;
    scriptParams[ "timeoutMs" ] = 3000;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    assertEventBroadcast( m_broadcasts[ 1 ], m_requestId, "http_data_received", "", nullptr, __LINE__ );
    assertEventBroadcast( m_broadcasts[ 2 ], m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    const auto& received = m_broadcasts[ 1 ];
    ASSERT_FALSE( received.empty() ) << "Received event should not be empty";

    Json result;
    ASSERT_TRUE( json_get_obj( received, "result", result ) ) << "'result' key is missing";
    const unsigned expectedSize = 1304;
    const std::string expectedHash = "ab8cee2e025f066ea9dc0a3d0c975881";

    ASSERT_EQ( json_get_str( result, "data_hash" ), expectedHash ) << "hash of a picture should match";
    ASSERT_EQ( json_get_int( result, "data_size" ), expectedSize ) << "size of a picture should match";
}

TEST_F( HttpLuadTests, multipleHeadersFalse ) /* 4.1 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;
    Json paramsUserData = "user_data is a simple string <-------- !!!!!!!!!!!!!!!!!!! -------->";

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] =
        makeUrl( std::string{ "/reflect/get?suite=HttpLuadTests&method=" }.append( test_info_->name() ).c_str() );
    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "multiple_headers" ] = false;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    const auto broadcastDataReceived = m_broadcasts[ 1 ];
    assertEventBroadcast( broadcastDataReceived, m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    // check type and contents of a 'user_data' field
    Json headers;
    ASSERT_TRUE( json_get( broadcastDataReceived, { "result", "data", "headers" }, headers ) );
    ASSERT_TRUE( headers.is_object() ) << "Should be an object for multiple_headers=false request";
}

TEST_F( HttpLuadTests, multipleHeadersTrue ) /* 4.2 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    const int expectedRepeats = 10;
    const std::string headerToRepeat = "Set-Cookie";
    const std::string multiHeadersParams =
        std::string{ "multiply_header=" + headerToRepeat + "&repeat_count=" } + std::to_string( expectedRepeats );

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( std::string{ "/reflect/get?suite=HttpLuadTests&method=" }
                                         .append( test_info_->name() )
                                         .append( "&" )
                                         .append( multiHeadersParams )
                                         .c_str() );
    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "timeoutMs" ] = 3000;
    scriptParams[ "multiple_headers" ] = true;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    const auto broadcastDataReceived = m_broadcasts[ 1 ];
    assertEventBroadcast( broadcastDataReceived, m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    Json headers;
    ASSERT_TRUE( json_get( broadcastDataReceived, { "result", "data", "headers" }, headers ) );

    EXPECT_TRUE( headers.is_array() ) << "Should be an array for multiple_headers=true request";

    int countRepeatedHeader = 0;
    for( const auto& kv : headers ) {
        EXPECT_TRUE( kv.is_object() );
        EXPECT_TRUE( kv.count( "key" ) && kv.count( "value" ) );

        auto key = json_get_str( kv, "key" );
        if( key == headerToRepeat )
            countRepeatedHeader++;
    }

    EXPECT_EQ( countRepeatedHeader, expectedRepeats )
        << "Number of headers in response should match the number in a request";
}

TEST_F( HttpLuadTests, multipleHeadersFromRedirectsTrue ) /* 4.3 */ {
    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", httpRequestScript } } );

    Json scriptParams = JSON_EMPTY_OBJ;

    const std::string headerToRepeat = "Set-Cookie";
    const int expectedRepeats = 7;    // number of times the  headerToRepeat should be repeated in a final response
    const int expectedRedirects = 4;  // number of redirects to perform
    const std::string multiHeadersParams =
        std::string{ "multiply_header=" + headerToRepeat + "&repeat_count=" } +
        std::to_string( expectedRepeats ).append( "&make_redirect=" + std::to_string( expectedRedirects ) );

    scriptParams[ "command" ] = "request";
    scriptParams[ "url" ] = makeUrl( std::string{ "/reflect/get?suite=HttpLuadTests&method=" }
                                         .append( test_info_->name() )
                                         .append( "&" )
                                         .append( multiHeadersParams )
                                         .c_str() );

    scriptParams[ "handler" ] = httpEventScript;
    scriptParams[ "timeoutMs" ] = 3000;
    scriptParams[ "redirection" ] = 10;
    scriptParams[ "multiple_headers" ] = true;

    request[ "params" ][ "scriptParams" ] = scriptParams;

    auto dump = request.dump();
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    ASSERT_TRUE( error.empty() ) << error.dump( 4 );

    waitForDataBroadcasts( 3, 7 );  // 3 data events + 4 progress events (2 for 2 lua script calls)

    ASSERT_EQ( m_broadcasts.size(), 3 );
    const auto broadcastDataReceived = m_broadcasts[ 1 ];
    assertEventBroadcast( broadcastDataReceived, m_requestId, "http_data_received", "", nullptr, __LINE__ );
    const auto& broadcastConnectionClosed = m_broadcasts[ 2 ];
    assertEventBroadcast( broadcastConnectionClosed, m_requestId, "http_connection_closed", "", nullptr, __LINE__ );

    Json headers;
    ASSERT_TRUE( json_get( broadcastDataReceived, { "result", "data", "headers" }, headers ) );

    EXPECT_TRUE( headers.is_array() ) << "Should be an array for multiple_headers=true request";

    int countRepeatedHeader = 0;
    for( const auto& kv : headers ) {
        EXPECT_TRUE( kv.is_object() );
        EXPECT_TRUE( kv.count( "key" ) && kv.count( "value" ) );

        auto key = json_get_str( kv, "key" );
        if( key == headerToRepeat )
            countRepeatedHeader++;
    }

    EXPECT_EQ( countRepeatedHeader, expectedRepeats )
        << "Number of headers in response should match the number in a request";
}

}  // namespace Tests
}  // namespace HubPlatform
