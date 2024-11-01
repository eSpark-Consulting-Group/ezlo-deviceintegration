#include "http/http_options.h"
#include "http/http_events.h"
#include "http/http.h"

#include "json_utils/json_utils.h"

#include "test_utils/component_tests/http_server/http_server.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"

#include "base_utils/md5.h"

#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"

#include <gtest/gtest.h>
#include <curl/curl.h>
#include <thread>
#include <atomic>
#include <fstream>
#include <memory>

/*

TESTPLAN

DONE    1. to_json/from_json serialization
DONE        1.1 HttpOptions to json
DONE        1.2 HttpOptions from json
DONE        1.3 Certificates to json
DONE        1.4 Certificates from json
DONE        1.5 HttpOptions with multiple headers to json
DONE        1.6 HttpOptions with multiple headers from json

DONE    2. libcurl related tests, request
DONE        2.1 GET with portion 10k bytes
DONE        2.2 GET maximum number of connections per plugin, check error
DONE        2.3 GET request to invalid server
DONE        2.4 POST requests
DONE            2.4.1 POST request with short body, no expect: 100-continue header
DONE            2.4.2 POST request with long body, with expect: 100-continue header
TODO            2.4.3 POST with empty body and no content-length header
TODO            2.4.4 POST with empty body and content-length=0
DONE            2.4.5 PUT request with short body, no expect: 100-continue header
DONE            2.4.6 PATCH request with short body, no expect: 100-continue header
DONE        2.5 POST and raise http_data_send event
TODO            2.5.1 POST request with last = false, data = "" should raise http_data_send event
TODO            2.5.2 POST send_data() with binary data, check data integrity
DONE        2.6 OPTIONS request
DONE        2.7 HEAD request, response has no body
DONE        2.8 GET, basic authentication request
DONE        2.9 GET, http status 404, with error description in http_connection_closed
DONE        2.10 GET, http status 403, then read all response body, and then exit.
DONE        2.11 GET request, with Multiple Headers with the same name

DONE    3. Connections
DONE        3.1 Create connection and close it immediately
DONE        3.2 Create connection with keepAlive=true and close it:
DONE            3.2.1 Create connection with keepAlive=true and close it immediately
DONE            3.2.2 Create connection with keepAlive=true and close it after some time
DONE        2.3 Get connection information, user_data: object

DONE    4. repeat_allowed
DONE        4.1 Try http.repeat_request method, repeat_allowed = true, user_data: string
DONE        4.2 DISABLED, MANUAL: multiple responses, repeat_allowed = false
DONE        4.3 DISABLED, MANUAL: http_bridge tester: waits for requests for 5 minutes

DONE    5. Download binary file
DONE        5.1 Download a binary file in concurrent way, check HttpImpl::clearQueue for cross-locks.

DONE    6. Events serialization/deserialization
            6.1 DataReceivedEvent
            6.2 DataSendEvent
            6.3 ConnectionClosedEvent

DONE    7. Close all connections owned by a plugin

*/

namespace HubPlatform {
namespace UT {

using namespace CommonHttp;

class HttpTests : public ::testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static void runRequest( Http& http, const std::string& pluginName, const HttpOptions& opts, std::string& id );
    static void waitTillFinish( Http& http );
    void saveToStorage( const std::string& eventHandler, const Json& eventData );
    std::unique_ptr< Http > configureHttp();

    static std::string md5( const std::string& );

    static Json makeUserData();
    static std::string makeLongString( int loops, const std::string& message = "" );
    static const std::string errorAt( const char* file, int codeLine );

    void withBodyTest( const std::string& testName,
                       const std::string& method,
                       size_t bodySize,
                       bool withExpect,
                       const std::string& message,
                       int codeLine );

protected:
    using ConnectionId = std::string;
    using EventPair = std::pair< std::string, Json >;
    using EventList = std::vector< EventPair >;
    using EventStorage = std::map< ConnectionId, EventList >;

    EventStorage m_storage;
    Json m_eventDataSend, m_eventDataReceived, m_eventConnectionClosed;
    std::vector< Json > m_eventsCaught;

    std::string m_pluginName = "http_tests";
    const char* const m_defaultHandler = "HUB:http_tests/scripts/not_exists";
    static int m_timeoutMs;

    using HttpServer = Tests::HttpServer;

    class EventHitsCounter {
    public:
        std::atomic< size_t > m_dataSend{ 0 };
        std::atomic< size_t > m_dataReceived{ 0 };
        std::atomic< size_t > m_connectionClosed{ 0 };
        std::atomic< size_t > m_dataReceivedBodySize{ 0 };
    };
    EventHitsCounter m_counter;

protected:
    static unsigned& getPort() {
        static unsigned port = 8000;
        return port;
    }
    static unsigned& getSecurePort() {
        static unsigned port = 8001;
        return port;
    }
    static std::string m_hostName;

    static std::string makeHostName( unsigned port = 0 ) {
        return m_hostName + ":" + std::to_string( port > 0 ? port : getPort() );
    }

    static std::string makeUrl( const char* path, const char* hostName = "", const char* proto = "" ) {
        if( !proto || !*proto )
            proto = "http";
        if( !path || !*path )
            path = "/";
        unsigned port = getPort();
        if( 0 == strcmp( proto, "https" ) )
            port = getSecurePort();
        std::stringstream os;
        os << proto << "://" << ( hostName && *hostName ? hostName : makeHostName( port ) ) << path;
        return os.str();
    }
    static std::string makeUrl( const std::string& path, const char* hostName = "", const char* proto = "" ) {
        return makeUrl( path.c_str(), hostName, proto );
    }

    static HttpServer& getHttpServer() {
        static HttpServer lws;
        return lws;
    }
};

int HttpTests::m_timeoutMs = 10000;
std::string HttpTests::m_hostName = "localhost";

void HttpTests::SetUpTestCase() {
    curl_global_init( CURL_GLOBAL_ALL );
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );

    LOGGER_DEBUG( "getUniqueFreeLocalPort HttpTests pending..." );

    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::lock_guard< Process::SharedMutex > lock( processMutex );

    LOGGER_DEBUG( "getUniqueFreeLocalPort HttpTests" );

#ifdef _DEBUG
    m_timeoutMs = 90000;
#endif
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

    server.start();
}

void HttpTests::TearDownTestCase() {
    curl_global_cleanup();

    auto& server = getHttpServer();
    server.stop();
}

void HttpTests::runRequest( Http& http, const std::string& pluginName, const HttpOptions& opts, std::string& id ) {
    try {
        id = http.request( pluginName, opts );
        SUCCEED();
    }
    catch( const HttpError& err ) {
        FAIL() << err.what();
    }
}

void HttpTests::waitTillFinish( Http& http ) {
    if( http.joinable() ) {
        http.join();
        Log::info( "Exit from join" );
    }
}

void HttpTests::saveToStorage( const std::string& eventHandler, const Json& eventData ) {
    auto dump = eventData.dump();
    Log::trace( cc::trace( "Outer event handler reached!" ) );
    Log::debug( cc::debug( "# Event: http_data_reached: " ) + std::to_string( dump.size() ) );

    auto connId = json_get_str( eventData, "id" );
    auto foundMap = m_storage.find( connId );
    if( m_storage.end() == foundMap ) {
        foundMap = m_storage.emplace( std::move( connId ), EventList() ).first;
    }
    foundMap->second.emplace_back( std::make_pair( eventHandler, eventData ) );
}

std::unique_ptr< Http > HttpTests::configureHttp() {
    auto onDataSend = [this]( const std::string& eventHandler, const EventPtr& event ) -> void {
        if( !event.get() )
            return;
        auto eventData = event->toJson();
        this->m_eventsCaught.push_back( eventData );
        this->m_eventDataSend = std::move( eventData );
        this->m_counter.m_dataSend++;
    };
    auto onDataReceived = [this]( const std::string& eventHandler, const EventPtr& event ) -> void {
        if( !event.get() )
            return;
        auto eventData = event->toJson();
        this->m_eventsCaught.push_back( eventData );
        this->m_eventDataReceived = std::move( eventData );
        auto dump = this->m_eventDataReceived.dump();

        Json eventDataField, postData;
        std::string postDataBody;
        if( json_get( m_eventDataReceived, "data", postData ) ) {
            postDataBody = json_get_str( postData );
        }
        else {
            LOGGER_DEBUG_RAW( "Failed to get post_body" );
        }

        this->m_counter.m_dataReceived++;
        this->m_counter.m_dataReceivedBodySize += postDataBody.size();

        const std::string& details = dump.size() < 1024 ? cc::j( this->m_eventDataReceived ) : "";
        LOGGER_DEBUG_RAW( cc::debug( "Data received: size: " ) + cc::num10( dump.size() ) +
                          ", body size: " + cc::num10( postDataBody.size() ) + ", " + details );
    };
    auto onConnectionClosed = [this]( const std::string& eventHandler, const EventPtr& event ) -> void {
        if( !event.get() )
            return;
        auto eventData = event->toJson();
        this->m_eventsCaught.push_back( eventData );
        this->m_eventConnectionClosed = std::move( eventData );
        this->m_counter.m_connectionClosed++;
    };

    std::unique_ptr< Http > httpPtr( new Http );
    httpPtr->on( "http_data_send", onDataSend );
    httpPtr->on( "http_data_received", onDataReceived );
    httpPtr->on( "http_connection_closed", onConnectionClosed );

    httpPtr->exitDelayInterval( 0 );

    return httpPtr;
}

std::string HttpTests::md5( const std::string& messageData ) {
    Md5 hasher;
    hasher.append( messageData );
    return hasher.getHex();
}

Json HttpTests::makeUserData() {
    Json userData = { { "some_id", "connection user_data as object !!!!!!!!!!!! <---------" },
                      { "another_field", "...and its data" } };
    return userData;
}

TEST_F( HttpTests, toJsonHttpOptions ) /* 1.1 */ {
    HttpOptions opts;
    opts.url = "http://localhost:80/some/path/script?query=string&and=more";
    opts.type = "GET";
    opts.userData = { { "a", "b" }, { "c", "d" } };
    opts.failOnError = true;
    opts.verbose = false;

    Json j = JSON_EMPTY_OBJ;
    to_json( j, opts );

    Json expected = R"({
        "url": "http://localhost:80/some/path/script?query=string&and=more",
        "handler": "",
        "user_data": {
            "a": "b",
            "c": "d"
        }
    })"_json;
    EXPECT_EQ( j, expected );

    j.clear();
    opts.type = "POST";
    opts.userData = "simple string";
    opts.verbose = true;
    opts.failOnError = false;
    expected = R"({
        "url": "http://localhost:80/some/path/script?query=string&and=more",
        "type": "POST",
        "handler": "",
        "user_data": "simple string",
        "verbose": true,
        "fail_on_error": false
    })"_json;
    to_json( j, opts );
    EXPECT_EQ( j, expected );
}

TEST_F( HttpTests, fromJsonHttpOptions ) /* 1.2 */ {
    HttpOptions opts;
    Json j = JSON( "url"
                   << "url"
                   << "content_type"
                   << "content-type" );
    j[ "headers" ] = Headers{ { "a", "b" }, { "c", "d" } };

    from_json( j, opts );

    EXPECT_EQ( opts.url, "url" );
    EXPECT_EQ( opts.contentType, "content-type" );
    EXPECT_EQ( opts.headers.size(), 2 );
}

TEST_F( HttpTests, toJsonCertificates ) /* 1.3 */ {
    {
        Json j;
        Certificates cert;
        to_json( j, cert );
        EXPECT_EQ( j, Json() );
    }
    {
        Json j;
        Certificates cert = { "ca" };
        to_json( j, cert );
        EXPECT_EQ( j,
                   JSON( "ca"
                         << "ca" ) );
    }
    {
        Json j;
        Certificates cert = { "ca", "private key", "public key" };
        to_json( j, cert );
        EXPECT_EQ( j,
                   JSON( "ca"
                         << "ca"
                         << "key"
                         << "private key"
                         << "public"
                         << "public key" ) );
    }
}

TEST_F( HttpTests, fromJsonCertificates ) /* 1.4 */ {
    {
        Certificates cert;
        Json j;
        ASSERT_NO_THROW( from_json( j, cert ) );
        EXPECT_EQ( cert.ca, "" );
    }
    {
        Json j = { { "ca", "value" } };
        Certificates cert;
        ASSERT_NO_THROW( from_json( j, cert ) );
        EXPECT_EQ( cert.ca, "value" );
        EXPECT_TRUE( cert.privateKey.empty() );
        EXPECT_TRUE( cert.publicKey.empty() );
    }
    {
        Json j = { { "ca", "111" }, { "key", "222" }, { "public", "333" } };
        Certificates cert;
        ASSERT_NO_THROW( from_json( j, cert ) );
        EXPECT_EQ( cert.ca, "111" );
        EXPECT_EQ( cert.privateKey, "222" );
        EXPECT_EQ( cert.publicKey, "333" );
    }
}

TEST_F( HttpTests, toJsonHttpOptionsHeaders ) /* 1.5 */ {
    HttpOptions opts;
    opts.url = "http://localhost:80/some/path/script?query=string&and=more";
    opts.type = "GET";
    JsonArray headersArray = { { { "key", "a" }, { "value", "valueForA" } },
                               { { "key", "a" }, { "value", "secondValueForA" } },
                               { { "key", "b" }, { "value", "valueForB" } } };

    opts.headers = json_get_multimap< std::string >( headersArray, "key", "value" );
    opts.multipleHeaders = true;

    Json j = JSON_EMPTY_OBJ;
    to_json( j, opts );

    Json expected = R"({
        "url": "http://localhost:80/some/path/script?query=string&and=more",
        "handler": "",
        "headers": [
            { "key": "a", "value": "valueForA" },
            { "key": "a", "value": "secondValueForA" },
            { "key": "b", "value": "valueForB" }
        ],
        "multiple_headers": true
    })"_json;
    EXPECT_EQ( j, expected );
}

TEST_F( HttpTests, fromJsonHttpOptionsHeaders ) /* 1.6 */ {
    HttpOptions opts;
    Json j = JSON( "url"
                   << "url"
                   << "content_type"
                   << "application/json"
                   << "multiple_headers" << true );
    j[ "headers" ] = {
        { { "key", "a" }, { "value", "b" } }, { { "key", "a" }, { "value", "c" } }, { { "key", "c" }, { "value", "d" } }
    };

    from_json( j, opts );

    EXPECT_EQ( opts.url, "url" );
    EXPECT_EQ( opts.contentType, "application/json" );
    EXPECT_EQ( opts.headers.size(), 3 );

    auto range = opts.headers.equal_range( "a" );
    ASSERT_EQ( std::distance( range.first, range.second ), 2 ) << "For key 'a' 2 records";
    EXPECT_EQ( opts.multipleHeaders, true );
}

TEST_F( HttpTests, curlGETPortions10k ) /* 2.1 */ {
    HttpOptions opts;
    std::string baseUrl = makeUrl( "/reflect/nma/getting_started/?suite=HttpTests&method=curlRun" );
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.type = "GET";
    opts.keepAlive = true;
    opts.newDataBuffer = 10000;  // limit size of notification for event http_data_received
    opts.verbose = true;

    Http http;
    std::string id;
    runRequest( http, m_pluginName, opts, id );

    ASSERT_FALSE( id.empty() ) << id;
    waitTillFinish( http );
}

TEST_F( HttpTests, curlAddMaxPluginConnectionsAndGET ) /* 2.2 */ {
    HttpOptions opts;
    // std::string baseUrl = "https://postman-echo.com/get?suite=DISABLED_HttpTests&method=curlAddMaxPluginConnections";
    std::string baseUrl = makeUrl( "/reflect/get?from=curlAddMaxPluginConnections" );
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.type = "GET";
    opts.keepAlive = true;
    opts.newDataBuffer = 10000;  // limit size of notification for event http_data_received
    opts.verbose = true;

    const unsigned numberOfConnections = 10;
    Log::debug( "Http tests started " );

    std::unique_ptr< Http > httpPtr = configureHttp();
    Http& http = *httpPtr;
    http.setMaxPluginConnections( numberOfConnections );
    auto eventHandler = [this]( const std::string& eventHandler, const EventPtr& event ) -> void {
        saveToStorage( eventHandler, event->toJson() );
    };

    http.on( "http_data_received", eventHandler );

    std::string id;
    for( size_t i = 0; i < http.getMaxPluginConnections(); i++ ) {
        opts.url = baseUrl + std::string{ "&try=" } + std::to_string( i );
        id.clear();
        runRequest( http, m_pluginName, opts, id );
        ASSERT_FALSE( id.empty() ) << id;
    }

    id.clear();
    ASSERT_ANY_THROW( id = http.request( m_pluginName, opts ) ) << "Should throw on max connections added";

    waitTillFinish( http );

    // check all events here
    ASSERT_EQ( m_storage.size(), numberOfConnections )
        << "Number of event lists should match number of requested connections";
    ASSERT_EQ( m_counter.m_connectionClosed, numberOfConnections )
        << "Number of connection_closed events should match to number of opened connections";
}

TEST_F( HttpTests, curlGETErrorCannotConnectToServer ) /* 2.3 */ {
    HttpOptions opts;
    // localhost with unused port
    std::string baseUrl = "http://localhost:7999/?suite=HttpTests&method=curlGetErrorCannotconnectoToServer";
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.type = "GET";
    opts.verbose = true;
    Json eventConnectionClosed;

    auto eventHandler = [&eventConnectionClosed]( const std::string& eventHandler, const EventPtr& event ) -> void {
        eventConnectionClosed = event->toJson();
    };

    Http http;
    http.on( "http_connection_closed", eventHandler );

    std::string id;
    runRequest( http, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( http );

    ASSERT_FALSE( eventConnectionClosed.empty() ) << "Event http_connection_closed should not be empty";
}

std::string HttpTests::makeLongString( int loops, const std::string& message ) {
    std::string postingTemplate = !message.empty() ? message : "This is a string to make a POST request with\n";
    std::string postingData;
    for( int i = 1; i <= loops; i++ )
        postingData += std::to_string( i ) + " : " + postingTemplate;
    return postingData;
}

const std::string HttpTests::errorAt( const char* file, int codeLine ) {
    std::string res = file;
    res += ":" + std::to_string( codeLine );

    return res;
}

TEST_F( HttpTests, curlPOSTNoExpect ) /* 2.4.1 */ {
    const auto testName = test_info_->name();
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + testName );

    std::string postingData = std::string{ testName }.append( ": Short body string." );

    HttpOptions opts;
    // std::string baseUrl = "https://postman-echo.com/post?suite=HttpTests&method=curlPostBody";
    std::string baseUrl = makeUrl( std::string{ "/reflect/post?suite=HttpTests&method=" }.append( testName ).c_str() );
    opts.handler = m_defaultHandler;

    opts.url = baseUrl;
    opts.type = "POST";
    opts.data = postingData;
    opts.contentLength = postingData.size();
    opts.last = true;
    opts.contentType = "application/octet-stream";
    opts.newDataBuffer = 1000;
    opts.timeoutMs = m_timeoutMs;
    opts.verbose = true;
    opts.headers = { { "expect", "" } };

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    EXPECT_FALSE( m_eventDataReceived.empty() ) << "Event http_data_received should not be empty";

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";

    LOGGER_TRACE( "data_received: " + cc::j( m_eventDataReceived ) );
    EXPECT_EQ( json_get_int( m_eventConnectionClosed, "code" ), 200 )
        << "Http status should be 200 Ok, not 100 continue";
}

void HttpTests::withBodyTest( const std::string& testName,
                              const std::string& method,
                              size_t bodySize,
                              bool withExpect,
                              const std::string& message,
                              int codeLine ) {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + testName );

    std::string postingData = makeLongString( bodySize, "X" );
    LOGGER_DEBUG( "postingData.size: " + cc::num10( postingData.size() ) );

    postingData.resize( bodySize );
    LOGGER_DEBUG( "postingData.size resized: " + cc::num10( postingData.size() ) );

    HttpOptions opts;
    // uncomment next define to check with POSTMAN, only for test web-server development!
    //#define USE_POSTMAN_CHECK
    //#define USE_LOCAL_REFLECTOR_PY

#ifdef USE_POSTMAN_CHECK
    LOGGER_WARN( "DEV: Use postman-echo.com/post endpoint to test" );
    std::string baseUrl = std::string{ "https://postman-echo.com/post?suite=HttpTests&method=" }.append( testName );
    const int dataBufferSize = 1000;
    const int expectedPortions = postingData.size() / dataBufferSize + 2;
#elif defined( USE_LOCAL_REFLECTOR_PY )
    LOGGER_WARN( "DEV: Use local reflector script endpoint to test" );
    std::string baseUrl = std::string{ "http://127.0.0.1:8080/post?suite=HttpTests&method=" }.append( testName );
    const int dataBufferSize = 1000;
    const int expectedPortions = 1;  // output around 200 bytes
#else
    std::string baseUrl = makeUrl( std::string{ "/reflect/post?suite=HttpTests&method=" } + testName );
    const int dataBufferSize = 1000;
    // FIXME: can't understand why +2 not +1
    const int expectedPortions = postingData.size() / dataBufferSize + 2;
#endif
    opts.handler = m_defaultHandler;

    opts.url = baseUrl;
    opts.type = method;
    opts.data = postingData;
    opts.contentLength = postingData.size();
    opts.last = true;
    opts.contentType = "application/octet-stream";
    opts.newDataBuffer = dataBufferSize;
    opts.timeoutMs = m_timeoutMs;
    opts.verbose = true;
    opts.timeoutMs = 5000;
    if( withExpect ) {
        opts.headers = { { "expect", "100-continue" } };
    }

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id << ", " << errorAt( __FILE__, codeLine );
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    EXPECT_FALSE( m_eventDataReceived.empty() )
        << "Event http_data_received should not be empty, " << errorAt( __FILE__, codeLine );
    EXPECT_EQ( m_counter.m_dataReceived, expectedPortions )
        << expectedPortions << " portions * " << dataBufferSize << " bytes expected; " << errorAt( __FILE__, codeLine );

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections; "
        << errorAt( __FILE__, codeLine );
    EXPECT_EQ( json_get_int( m_eventConnectionClosed, "code" ), 200 )
        << "Http status should be 200 Ok, not 100 continue; " << errorAt( __FILE__, codeLine );
}

TEST_F( HttpTests, curlWithBodyExpect100Continue_POST ) /* 2.4.2 */ {
    withBodyTest( test_info_->name(), "POST", 6000 - 1, true, "POST with body", __LINE__ );
}

// FIXME: Tests behave bad wih PUT  and  PATCH requests while behave good on POST. bUt, functionality work well on other
// web-servers other that reflect.
TEST_F( HttpTests, DISABLED_curlWithBodyExpect100Continue_PUT ) /* 2.4.5 */ {
    withBodyTest( test_info_->name(), "PUT", 6000 - 1, true, "PUT with body", __LINE__ );
}

// FIXME: the same
TEST_F( HttpTests, DISABLED_curlWithBodyExpect100Continue_PATCH ) /* 2.4.6 */ {
    withBodyTest( test_info_->name(), "PATCH", 6000 - 1, true, "PATCH with body", __LINE__ );
}

TEST_F( HttpTests, curlPOSTBodyAndRaiseDataSendEvent ) /* 2.5 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    std::string postingData = makeLongString( 100 );

    const int repeats = 2, portionSize = 1000;

    HttpOptions opts;
    // std::string baseUrl = "https://postman-echo.com/post?suite=HttpTests&method=curlPOSTBodyAndRaiseDataSendEvent";
    std::string baseUrl = makeUrl( "/reflect/post?suite=HttpTests&method=curlPOSTBodyAndRaiseDataSendEvent" );
    opts.handler = m_defaultHandler;

    opts.url = baseUrl;
    opts.type = "POST";
    opts.data = "";
    opts.contentLength = repeats * portionSize;
    opts.headers = Headers{ { "expect", "" } };
    opts.repeatAllowed = false;
    opts.last = false;
    opts.contentType = "application/octet-stream";
    opts.newDataBuffer = 10000;
    opts.timeoutMs = m_timeoutMs;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    int counter = 0;
    auto dataSendHandler = [&counter, this, &httpPtr]( const std::string& eventHandler,
                                                       const EventPtr& event ) -> void {
        if( !event.get() )
            return;
        bool last = ++counter == repeats;
        LOGGER_TRACE_RAW( "dataSendHandler: counter=" + cc::num10( counter ) + ", last=" + cc::flag( last ) );
        std::string postingData = makeLongString( portionSize, "+" ).substr( 1, 1000 );

        auto eventData = event->toJson();
        auto dump = eventData.dump();
        if( eventData.is_object() ) {
            std::string id = json_get_str( eventData, "id" );
            Log::trace( "httpPtr->sendData called" );
            httpPtr->sendData( m_pluginName, id, postingData, last );
        }
        else {
            throw HttpError( "Malformed event object passed to http_data_send handler" );
        }
    };

    httpPtr->on( "http_data_send", dataSendHandler );

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    const int expectedDataReceived = 1;
    EXPECT_EQ( m_counter.m_dataSend, repeats );
    EXPECT_EQ( m_counter.m_dataReceived, expectedDataReceived );
    EXPECT_EQ( m_counter.m_connectionClosed, 1 );
    EXPECT_FALSE( m_eventDataSend.empty() ) << "Event http_data_send should not be empty";
}

TEST_F( HttpTests, curlOPTIONS ) /* 2.6 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string testSuffix = "suite=HttpTests&method=curlOptions";
    HttpOptions opts;
    // std::string baseUrl = "https://postman-echo.com/get?" + testSuffix;
    std::string path = std::string( "/reflect/options?" ) + testSuffix;
    std::string baseUrl = makeUrl( path.c_str() );
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    opts.type = "OPTIONS";
    //    opts.type = "TRACE"; // postman-echo - 405 Not Allowed
    //    opts.type = "CONNECT"; // 502 Bad Gateway
    //    opts.type = "DELETE"; // 404 Not Found
    //    opts.type = "PUT"; // 404 Not Found

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    waitTillFinish( *httpPtr );
    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    EXPECT_FALSE( m_eventDataReceived.empty() ) << "Event http_data_received should not be empty";
    auto eventDump = m_eventDataReceived.dump();
    Log::trace( eventDump );
    EXPECT_EQ( json_get_int( m_eventDataReceived, "code", 0 ), 200 ) << eventDump;

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";
}

TEST_F( HttpTests, curlHEAD ) /* 2.7 */ {
    const std::string testSuffix = "suite=HttpTests&method=curlHead";
    HttpOptions opts;
    std::string baseUrl = "/reflect/get?" + testSuffix;
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.type = "HEAD";
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    waitTillFinish( *httpPtr );
    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    EXPECT_FALSE( m_eventDataReceived.empty() ) << "Event http_data_received should not be empty";
    auto eventDump = m_eventDataReceived.dump();
    Log::trace( eventDump );
    EXPECT_EQ( json_get_int( m_eventDataReceived, "code", 0 ), 200 ) << eventDump;
    EXPECT_TRUE( json_get_str( m_eventDataReceived, "data" ).empty() ) << eventDump;

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";
}

TEST_F( HttpTests, curlGETWithBasicAuth ) /* 2.8 */ {
    const std::string testSuffix = "suite=HttpTests&method=curlGETWithBasicAuth";
    HttpOptions opts;
    std::string baseUrl = "/auth/?" + testSuffix;
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.user = "user";
    opts.password = "password";
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    opts.type = "GET";

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    waitTillFinish( *httpPtr );
    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    EXPECT_FALSE( m_eventDataReceived.empty() ) << "Event http_data_received should not be empty";
    auto eventDump = m_eventDataReceived.dump();

    Log::trace( eventDump );
    EXPECT_EQ( json_get_int( m_eventDataReceived, "code", 0 ), 200 ) << eventDump;
    EXPECT_FALSE( json_get_str( m_eventDataReceived, "data" ).empty() ) << eventDump;

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";

    ASSERT_EQ( json_get_int( m_eventConnectionClosed, "code", 0 ), 200 ) << eventDump;
    ASSERT_EQ( json_get_str( m_eventConnectionClosed, "code_status" ), "OK" ) << eventDump;
}

TEST_F( HttpTests, curlGETWith404Error ) /* 2.9 */ {
    const std::string testSuffix = "suite=HttpTests&method=curlGETWith404Error";
    HttpOptions opts;
    std::string baseUrl = "/this/path/does/not/exist/?" + testSuffix;
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.type = "GET";
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    waitTillFinish( *httpPtr );
    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";
    auto eventDump = m_eventConnectionClosed.dump();
    Log::trace( cc::j( eventDump ) );

    ASSERT_EQ( json_get_int( m_eventConnectionClosed, "code", 0 ), 404 ) << eventDump;
    ASSERT_EQ( json_get_str( m_eventConnectionClosed, "code_status" ), "Not Found" ) << eventDump;
    Json reason;
    EXPECT_TRUE( json_get( m_eventConnectionClosed, "reason", reason ) );
    EXPECT_EQ( json_get_int( reason, "code" ), toInt( Reason::CLOSED_BY_SERVER ) );
}

TEST_F( HttpTests, curlGETWith403ErrorAndReadResponseBody ) /* 2.10 */ {
    const std::string testSuffix = "suite=HttpTests&method=curlGETWith403ErrorAndReadResponseBody&httpStatus=403";
    HttpOptions opts;
    std::string baseUrl = "/dyn/get?" + testSuffix;
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.type = "GET";
    opts.failOnError = false;  // ignore error
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    waitTillFinish( *httpPtr );
    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";
    auto eventDump = m_eventConnectionClosed.dump();
    Log::trace( cc::j( eventDump ) );

    ASSERT_EQ( json_get_int( m_eventConnectionClosed, "code", 0 ), 403 ) << eventDump;
    ASSERT_EQ( json_get_str( m_eventConnectionClosed, "code_status" ), "Forbidden" ) << eventDump;

    Json reason;
    EXPECT_TRUE( json_get( m_eventConnectionClosed, "reason", reason ) );
    EXPECT_EQ( json_get_int( reason, "code" ), toInt( Reason::CLOSED_BY_SERVER ) );
}

TEST_F( HttpTests, curlGETWithMultiHeaders ) /* 2.11 */ {
    LOGGER_TRACE_RAW( cc::trace( "# test name: " ) + test_info_->name() );
    const std::string testSuffix = std::string{ "suite=HttpTests&method=" } + test_info_->name();

    const int expectedRepeats = 10;
    const std::string headerToRepeat = "Set-Cookie";
    const std::string multiHeadersParams =
        std::string{ "multiply_header=" + headerToRepeat + "&repeat_count=" } + std::to_string( expectedRepeats );

    HttpOptions opts;
    std::string path = std::string( "/reflect/get?" ) + testSuffix + "&" + multiHeadersParams;
    std::string baseUrl = makeUrl( path.c_str() );
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.type = "GET";
    opts.verbose = true;
    opts.multipleHeaders = true;  // To activate multiple headers with the same name, turn on this option!

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    waitTillFinish( *httpPtr );
    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    EXPECT_FALSE( m_eventDataReceived.empty() ) << "Event http_data_received should not be empty";
    auto eventDump = m_eventDataReceived.dump();
    Log::trace( eventDump );
    EXPECT_EQ( json_get_int( m_eventDataReceived, "code", 0 ), 200 ) << eventDump;

    // {"code":200,"data":"{\"headers\":{\"accept:\":\"*/*\",\"get\":\"/reflect/get\",\"host:\":\"localhost:9018\",\"http/1.1\":\"HTTP/1.1\",\"uri-args\":\"suite=HttpTests,method=curlGETWithMultiHeaders,multiply_header=Set-Cookie,repeat_count=3\"},\"method\":\"GET\",\"now\":\"12:16:48\"}","headers":[{"key":"Set-Cookie","value":"619a1c900000002b6c4f19ec"},{"key":"Set-Cookie","value":"619a1c900000002b6c4f19ed"},{"key":"Set-Cookie","value":"619a1c900000002b6c4f19ee"},{"key":"connection","value":"close"},{"key":"content-type","value":"application/json"}],"id":"619a1c900000002b6c4f19ea","last":true,"multiple_headers":true,"url":"http://localhost:9018/reflect/get?suite=HttpTests&method=curlGETWithMultiHeaders&multiply_header=Set-Cookie&repeat_count=3"}

    auto headersMap = json_get_multimap< std::string >( m_eventDataReceived[ "headers" ], "key", "value" );
    auto range = headersMap.equal_range( headerToRepeat );
    ASSERT_TRUE( range.first != range.second ) << "found range for key '" << headerToRepeat << "' should not be empty";
    ASSERT_EQ( std::distance( range.first, range.second ), expectedRepeats )
        << expectedRepeats << " elements for a key '" << headerToRepeat << "' expected";

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";
}

TEST_F( HttpTests, httpCloseConnectionTypeClose ) /* 3.1 */ {
    const std::string testSuffix = "suite=HttpTests&method=httpCloseConnectionTypeClose";
    HttpOptions opts;

    std::string path = "/reflect/get?" + testSuffix;
    std::string baseUrl = makeUrl( path.c_str() );  // unsupported method here
    //    std::string baseUrl = "https://postman-echo.com/get?" + testSuffix;
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.type = "GET";
    opts.keepAlive = false;
    opts.timeoutMs = 10000;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    try {
        ASSERT_TRUE( httpPtr->closeConnection( m_pluginName, id ) );
        SUCCEED() << "Connection successfully closed";
    }
    catch( HttpError const& e ) {
        FAIL() << " Connection doesn't exist error: " << e.what();
    }

    waitTillFinish( *httpPtr );

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";
}

/** Close connection immediately, it can not even reach curl multi handle */
TEST_F( HttpTests, httpConnectionTypeKeepAliveCloseImmediately ) /* 3.2.1 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    const auto testSuffix = std::string{ "suite=HttpTests&method=" }.append( test_info_->name() );
    HttpOptions opts;
    std::string baseUrl = "/reflect/get?" + testSuffix;  // unsupported method here
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.type = "GET";
    opts.keepAlive = true;
    opts.timeoutMs = 10000;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    unsigned repeats = 10;
    const auto delayInterval = std::chrono::milliseconds( 0 );
    Log::info( "Number of repeats: " + cc::num10( repeats ) +
               ", delay interval: " + cc::num10( delayInterval.count() ) );

    for( auto i = 0; i < repeats; i++ ) {
        Log::info( "Run iteration #" + cc::num10( i ) + "/" + cc::num10( repeats ) );
        std::string id;
        runRequest( *httpPtr, m_pluginName, opts, id );
        EXPECT_FALSE( id.empty() ) << id;
        Log::info( "Connection ID: " + id );

        try {
            std::this_thread::sleep_for( delayInterval );
            ASSERT_TRUE( httpPtr->closeConnection( m_pluginName, id ) );
            Log::info( "Connection successfully closed, id: " + id );
        }
        catch( HttpError const& e ) {
            FAIL() << " Connection doesn't exist error: " << e.what();
        }
    }
    waitTillFinish( *httpPtr );
    ASSERT_EQ( m_counter.m_connectionClosed, repeats )
        << "Number of connection_closed events should match to number of opened connections";
}

/** Close connection after some time, it be placed to a plugin's queue or even in the curl multi handle */
TEST_F( HttpTests, httpConnectionTypeKeepAliveCloseAfterSomeTime ) /* 3.2.2 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    const auto testSuffix = std::string{ "suite=HttpTests&method=" }.append( test_info_->name() );

    HttpOptions opts;
    std::string baseUrl = "/reflect/get?" + testSuffix;
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.type = "GET";
    opts.keepAlive = true;
    opts.timeoutMs = 10000;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    const unsigned repeats = 10;
    const auto delayInterval = std::chrono::milliseconds( 50 );
    Log::info( "Number of repeats: " + cc::num10( repeats ) +
               ", delay interval: " + cc::num10( delayInterval.count() ) );

    for( auto i = 0; i < repeats; i++ ) {
        Log::info( "Run iteration #" + cc::num10( i ) + "/" + cc::num10( repeats ) );
        std::string id;
        runRequest( *httpPtr, m_pluginName, opts, id );
        EXPECT_FALSE( id.empty() ) << id;
        Log::info( "Connection ID: " + id );

        try {
            std::this_thread::sleep_for( delayInterval );
            ASSERT_TRUE( httpPtr->closeConnection( m_pluginName, id ) );
            Log::info( "Connection successfully closed, id: " + id );
        }
        catch( HttpError const& e ) {
            FAIL() << " Connection doesn't exist error: " << e.what();
        }
    }
    waitTillFinish( *httpPtr );
    ASSERT_EQ( m_counter.m_connectionClosed, repeats )
        << "Number of connection_closed events should match to number of opened connections";
}

TEST_F( HttpTests, connectionInfo ) /* 3.3 */ {
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
    const std::string testSuffix = "suite=HttpTests&method=connectionInfo";
    HttpOptions opts;
    std::string baseUrl = "/reflect/get?" + testSuffix;
    //    std::string baseUrl = "https://postman-echo.com/get?" + testSuffix;
    opts.type = "HEAD";
    opts.handler = m_defaultHandler;
    opts.url = makeUrl( baseUrl.c_str() );
    opts.keepAlive = true;
    opts.userData = makeUserData();
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );
    EXPECT_FALSE( id.empty() ) << id;

    ConnectionInfo info;
    ASSERT_NO_THROW( info = httpPtr->getConnectionInfo( m_pluginName, id ) ) << "Connection should exist";
    ASSERT_EQ( info.toJson().count( "user_data" ), 1 ) << info.toJson();
    ASSERT_EQ( info.toJson()[ "user_data" ], opts.userData )
        << "connectionInfo.user_data field should be equal to the data passed with request options";

    try {
        std::string nonExistentId = id + "1";
        httpPtr->getConnectionInfo( m_pluginName, nonExistentId );
        FAIL() << "Connection should not be found, non-existent id is used";
    }
    catch( HttpError const& e ) {
        SUCCEED() << "Connection exception should be thrown on non-existing id";
    }

    waitTillFinish( *httpPtr );

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";

    {
        Json userData;
        ASSERT_NE( this->m_eventConnectionClosed, nullptr );
        ASSERT_TRUE( json_get( m_eventConnectionClosed, "user_data", userData ) ) << m_eventConnectionClosed;
        ASSERT_EQ( userData, opts.userData );
    }
}

TEST_F( HttpTests, curlRepeatRequestGet ) /* 4.1 */ {
    std::string postingData = makeLongString( 10 );

    HttpOptions opts;
    // std::string baseUrl = "https://postman-echo.com/get?suite=HttpTests&method=curlRepeatRequest";
    std::string baseUrl = makeUrl( "/reflect/get?suite=HttpTests&method=curlRepeatRequest" );

    opts.handler = m_defaultHandler;

    opts.url = baseUrl;
    opts.type = "GET";
    opts.keepAlive = true;
    opts.timeoutMs = 20000;
    std::string userDataCallCounter = "0";
    opts.userData = userDataCallCounter;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    const int expectedDataReceivedEvents = 4;  // 1 - 0 repeats, 2 - 1 repeat, ...
    std::atomic< int > repeatCounter{ 0 };
    auto dataReceivedHandler = [this, &repeatCounter, &httpPtr, &userDataCallCounter, opts](
                                   const std::string& eventHandler, const EventPtr& eventPtr ) -> void {
        if( !eventPtr.get() )
            return;
        const auto& event = *eventPtr;

        if( repeatCounter++ >= expectedDataReceivedEvents - 1 )
            return;
        HttpOptions updatedOpts = opts;
        userDataCallCounter = std::to_string( repeatCounter );
        updatedOpts.userData = userDataCallCounter;
        updatedOpts.timeoutMs = 2000 * ( 1 + repeatCounter );

        const std::string& id = event.id();
        Log::trace( cc::trace( "httpPtr->repeatRequest called: " ) + id );

        try {
            httpPtr->repeatRequest( m_pluginName, id, updatedOpts );
        }
        catch( const HttpConnectionTransferPending& e ) {
            Log::error( cc::error( "Transfer ending, skip repeat_request call: " ) + e.what() );
        }
        catch( const HttpError& e ) {
            Log::error( cc::error( "Repeated request failed" ) + e.what() );
        }
    };

    httpPtr->on( "http_data_received", dataReceivedHandler );

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
    ASSERT_EQ( m_counter.m_dataReceived, expectedDataReceivedEvents )
        << "Number of events expected corresponds to number of requests";

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";

    {
        Json userData = "1";
        ASSERT_NE( this->m_eventConnectionClosed, nullptr );
        ASSERT_TRUE( json_get( m_eventConnectionClosed, "user_data", userData ) ) << cc::j( m_eventConnectionClosed );
        ASSERT_TRUE( userData.is_string() ) << "Type of user_data should be 'string': " << cc::j( userData );

        ASSERT_EQ( userData, std::to_string( repeatCounter - 1 ) );
    }
}

template < class Container >
void readFile( const std::string& name, Container& container ) {
    std::ifstream ifs( name, std::ios_base::binary );
    std::copy(
        std::istreambuf_iterator< char >( ifs ), std::istreambuf_iterator< char >(), std::back_inserter( container ) );
}

std::string readFile( const std::string& name ) {
    std::ifstream ifs( name );
    std::string content( ( std::istreambuf_iterator< char >( ifs ) ), std::istreambuf_iterator< char >() );
    return content;
}

/** To run this test, use https://github.com/azproduction/event-source-chat on port 8181. */
TEST_F( HttpTests, DISABLED_curlGETMultiResult_ManualTesting ) /* 4.2 */ {
    HttpOptions opts;
    std::string baseUrl = "http://127.0.0.1:8181/event?suite=HttpTests&method=curlGETMultiResult";
    opts.handler = m_defaultHandler;

    opts.url = baseUrl;
    opts.type = "GET";
    opts.keepAlive = true;
    opts.timeoutMs = 20000;  // HUGE timeout + keepAlive
    opts.newDataBuffer = 32;

    std::unique_ptr< Http > httpPtr = configureHttp();

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );
}

/** Enable this test to use only http-server for 10 minutes */
TEST_F( HttpTests, DISABLED_infiniteLoopManualTesting ) /* 4.3 */ {
    int duration = 600;
    LOGGER_TRACE_RAW( cc::note( "infiniteLoopManualTesting: Sleep for " ) + cc::num10( duration ) +
                      cc::note( " seconds" ) );
    std::this_thread::sleep_for( std::chrono::seconds( duration ) );
}

TEST_F( HttpTests, curlBinaryExchangeGet ) /* 5 */ {
    std::string postingData = makeLongString( 10 );

    HttpOptions opts;
    std::string baseUrl = makeUrl( "/img/ezlologo.png" );
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    EventPtr received;
    auto onDataReceived = [&received]( const std::string& eventHandler, const EventPtr& event ) -> void {
        if( !event.get() )
            return;
        received = event;
    };
    httpPtr->off( "http_data_received", nullptr );
    httpPtr->on( "http_data_received", onDataReceived );

    std::string id;
    runRequest( *httpPtr, m_pluginName, opts, id );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );

    ASSERT_EQ( m_counter.m_connectionClosed, 1 )
        << "Number of connection_closed events should match to number of opened connections";

    ASSERT_FALSE( received.get() == nullptr ) << "http_data_received should be present";

    auto& dataReceived = *dynamic_cast< CommonHttp::DataReceivedEvent* >( received.get() );
    auto& messageData = dataReceived.data();
    LOGGER_TRACE( cc::trace( "Data Received: size: " ) + cc::num10( messageData.size() ) );

    const int expectLength = 1304;
    const char expectMd5Hash[] = "ab8cee2e025f066ea9dc0a3d0c975881";

    ASSERT_EQ( messageData.size(), expectLength ) << "Size of a file downloaded should be is 1304";

    // calculate md5 sum of data received
    auto hash = md5( messageData );
    ASSERT_TRUE( strcasecmp( hash.c_str(), expectMd5Hash ) == 0 );
}

TEST_F( HttpTests, simultaneousRequests ) /* 5.1 */ {
    std::string postingData = makeLongString( 10 );

    HttpOptions opts;
    std::string baseUrl = makeUrl( "/img/ezlologo.png" );
    opts.handler = m_defaultHandler;
    opts.url = baseUrl;
    opts.keepAlive = true;
    opts.timeoutMs = 500;

    opts.userData = Json( { { "name", "first_request" }, { "counter", 0 }, { "keep_alive", opts.keepAlive } } );
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    unsigned counterLimit = 20;
    unsigned counter = 0;
    EventPtr received;
    auto onDataReceivedRaiseAnotherQuery = [&]( const std::string& eventHandler, const EventPtr& event ) -> void {
        if( !event.get() )
            return;
        received = event;

        std::string id;
        if( counter < counterLimit ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

            counter++;
            HttpOptions newOpts = opts;
            auto userData = event->userData();
            userData[ "counter" ] = counter;

            newOpts.userData = userData;
            newOpts.keepAlive = userData[ "keep_alive" ];
            runRequest( *httpPtr, m_pluginName, newOpts, id );
        }
    };
    httpPtr->on( "http_data_received", onDataReceivedRaiseAnotherQuery );

    std::string id, id1;
    runRequest( *httpPtr, m_pluginName, opts, id );

    HttpOptions opts2 = opts;
    opts2.keepAlive = false;
    opts2.userData = Json( { { "name", "second_request" }, { "counter", 0 }, { "keep_alive", opts.keepAlive } } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 30 * 3 ) );
    runRequest( *httpPtr, m_pluginName, opts2, id1 );

    EXPECT_FALSE( id.empty() ) << id;
    waitTillFinish( *httpPtr );

    Log::debug( cc::debug( "Number of events caught: " ) + std::to_string( m_eventsCaught.size() ) );

    ASSERT_EQ( m_counter.m_connectionClosed, counterLimit + 2 )
        << "Number of connection_closed events should match to number of opened connections";

    ASSERT_FALSE( received.get() == nullptr ) << "http_data_received should be present";

    auto& dataReceived = *dynamic_cast< CommonHttp::DataReceivedEvent* >( received.get() );
    auto& messageData = dataReceived.data();
    LOGGER_TRACE( cc::trace( "Data Received: size: " ) + cc::num10( messageData.size() ) );

    const int expectLength = 1304;
    const char expectMd5Hash[] = "ab8cee2e025f066ea9dc0a3d0c975881";

    ASSERT_EQ( messageData.size(), expectLength ) << "Size of a file downloaded should be is 1304";

    // calculate md5 sum of data received
    auto hash = md5( messageData );
    ASSERT_TRUE( strcasecmp( hash.c_str(), expectMd5Hash ) == 0 );
}

static Json eventDataReceivedPattern() {
    static Json eventDataReceivedPattern = R"({
        "id": "__ID__",
        "url": "__URL__",
        "user_data": {
            "__USER_DATA_KEY__": "__USER_DATA_VALUE__"
        },
        "code": 0,
        "data": "",
        "last": true
    })"_json;
    return eventDataReceivedPattern;
}

#define ASSERT_EXTENDED_NO_THROW( expr )         \
    do {                                         \
        try {                                    \
            { ( expr ); };                       \
            SUCCEED();                           \
        }                                        \
        catch( std::exception const& e ) {       \
            FAIL() << "It throws: " << e.what(); \
        }                                        \
    } while( false )

TEST_F( HttpTests, serialize_dataReceivedEvent ) /* 6.1 */ {
    std::string id = "111";
    std::string url = "http://example.com/";

    {
        // no event data field
        Json userData = nullptr;

        CommonHttp::DataReceivedEvent event( id, url, userData );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        auto expected = Tests::CommonNmaTests::replaceTokens( eventDataReceivedPattern(), list );

        expected.erase( "user_data" );

        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = "eventData is a string";

        CommonHttp::DataReceivedEvent event( id, url, userData );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        auto expected = Tests::CommonNmaTests::replaceTokens( eventDataReceivedPattern(), list );
        expected.erase( "user_data" );
        expected[ "user_data" ] = userData;

        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = { { "payload", "some data" } };

        CommonHttp::DataReceivedEvent event( id, url, userData );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id },
                               { "__URL__", url },
                               { "__USER_DATA_KEY__", "payload" },
                               { "__USER_DATA_VALUE__", json_get_str( userData, "payload" ) } };
        auto expected = Tests::CommonNmaTests::replaceTokens( eventDataReceivedPattern(), list );

        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = { { "payload", "some data" } };
        bool multiHeaders = true;
        CommonHttp::DataReceivedEvent event( id, url, userData, multiHeaders );
        Headers headers = { { "a", "v1" }, { "a", "v2" } };
        event.headers( headers );

        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id },
                               { "__URL__", url },
                               { "__USER_DATA_KEY__", "payload" },
                               { "__USER_DATA_VALUE__", json_get_str( userData, "payload" ) } };
        auto expected = Tests::CommonNmaTests::replaceTokens( eventDataReceivedPattern(), list );
        expected[ "multiple_headers" ] = true;
        expected[ "headers" ] = R"([{"key":"a","value":"v1"},{"key":"a","value":"v2"}])"_json;

        ASSERT_EQ( serializedEvent, expected );
    }
}

TEST_F( HttpTests, serialize_dataSendEvent ) /* 6.2 */ {
    std::string id = "111";
    std::string url = "http://example.com/";

    Json pattern;
    ASSERT_EXTENDED_NO_THROW( pattern = R"({"id":"__ID__","url":"__URL__", "max_data_length": 500000})"_json );
    {
        // no event data field
        Json userData = nullptr;

        CommonHttp::DataSendEvent event( id, url, userData );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        Json expected;
        ASSERT_EXTENDED_NO_THROW( expected = Tests::CommonNmaTests::replaceTokens( pattern, list ) );

        expected.erase( "user_data" );
        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = "userData is a string";

        CommonHttp::DataSendEvent event( id, url, userData );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        auto expected = Tests::CommonNmaTests::replaceTokens( pattern, list );
        expected.erase( "user_data" );
        expected[ "user_data" ] = userData;

        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = { { "payload", "some data" } };

        CommonHttp::DataSendEvent event( id, url, userData );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        auto expected = Tests::CommonNmaTests::replaceTokens( pattern, list );
        expected[ "user_data" ] = userData;

        ASSERT_EQ( serializedEvent, expected );
    }
}

TEST_F( HttpTests, serialize_connectionClosedEvent ) /* 6.3 */ {
    std::string id = "111";
    std::string url = "http://example.com/";

    Json pattern;
    ASSERT_EXTENDED_NO_THROW(
        pattern =
            R"({"id":"__ID__","url":"__URL__","reason":{"code":0,"text":""}, "code": 100, "code_status": "Continue"})"_json );
    {
        // no event data field
        Json userData = nullptr;

        CommonHttp::ConnectionClosedEvent event( id, url, userData );
        event.httpStatus( 100, "Continue" );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        Json expected;
        ASSERT_EXTENDED_NO_THROW( expected = Tests::CommonNmaTests::replaceTokens( pattern, list ) );

        expected.erase( "user_data" );
        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = "userData is a string";

        CommonHttp::ConnectionClosedEvent event( id, url, userData );
        event.httpStatus( 100, "Continue" );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        auto expected = Tests::CommonNmaTests::replaceTokens( pattern, list );
        expected.erase( "user_data" );
        expected[ "user_data" ] = userData;

        ASSERT_EQ( serializedEvent, expected );
    }
    {
        Json userData = { { "payload", "some data" } };

        CommonHttp::ConnectionClosedEvent event( id, url, userData );
        event.httpStatus( 100, "Continue" );
        auto serializedEvent = event.toJson();

        using KeyStringList = Tests::CommonNmaTests::KeyStringList;
        KeyStringList list = { { "__ID__", id }, { "__URL__", url } };
        auto expected = Tests::CommonNmaTests::replaceTokens( pattern, list );
        expected[ "user_data" ] = userData;

        ASSERT_EQ( serializedEvent, expected );
    }
}

TEST_F( HttpTests, httpClosePluginConnections ) /* 7 */ {
    int connectionsAmount = 3;
    HttpOptions opts;

    opts.handler = m_defaultHandler;
    opts.url = makeUrl( "/reflect/get?suite=HttpTests&method=httpCloseConnectionTypeKeepAlive" );
    opts.type = "GET";
    opts.keepAlive = true;
    opts.timeoutMs = 10000;
    opts.verbose = true;

    std::unique_ptr< Http > httpPtr = configureHttp();

    for( auto i = 0; i < connectionsAmount; i++ ) {
        std::string id;
        runRequest( *httpPtr, m_pluginName, opts, id );
        runRequest( *httpPtr, m_pluginName + "1", opts, id );
        EXPECT_FALSE( id.empty() ) << id;
    }

    ASSERT_EQ( m_counter.m_connectionClosed, 0 )
        << "Number of connection_closed events should be zero as connections were not closed yet";

    httpPtr->closePluginConnections( m_pluginName );
    ASSERT_EQ( m_counter.m_connectionClosed, connectionsAmount )
        << "Number of connection_closed events should match to number of opened connections for plugin";

    waitTillFinish( *httpPtr );
    ASSERT_EQ( m_counter.m_connectionClosed, connectionsAmount * 2 )
        << "Number of connection_closed events should match to number of opened connections";
}

}  // namespace UT
}  // namespace HubPlatform
