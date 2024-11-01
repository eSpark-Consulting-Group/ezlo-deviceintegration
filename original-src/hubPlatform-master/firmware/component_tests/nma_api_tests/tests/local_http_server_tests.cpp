#include "common_test_resources/fixtures/core_test_fixture.h"
#include "fixtures/local_http_server_fixture.h"
#include "http_bridge/ws_api_bridge.h"
#include "common_utils/curl_wrapper.h"
#include "json_utils/json_fwd.h"
#include "ssl_utils/sha256.h"
#include "tiny/tiny_utils.h"
#include "http/http.h"
#include "fixtures/http_event_receiver.h"
#include "http/resources.h"
#include <deque>
#include <string>
#include <mutex>
#include <atomic>
#include <utility>

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Local https-server tests, /v1/request
DONE        1.1 Make valid hub.info.get request
DONE        1.1.1 Make valid hub.item.value.set request (async request)
DONE        1.2 Make invalid request without json (400)
DONE        1.3 Make invalid request with json, but without method field (400)
DONE        1.4 User/Token is missing (401)
DONE        1.5 User/Token is present, but have invalid values (401)
DONE        1.6 Request lasts more then allowed (408)
DONE        1.7 Limit of connections reached (429)
DONE        1.8 Any websocket formatted error on request (500)

DONE    2. Local https-server tests, /v1/method
DONE        2.1 Make valid hub.info.get request
DONE        2.1.1 Make valid hub.item.value.set request (async request)
DONE        2.2 Make invalid request without json (400)
DONE            2.2.1 Errors in typed parameters: bool
DONE            2.2.2 Errors in typed parameters: int
DONE            2.2.3 Errors in typed parameters: float
DONE        2.3 Make invalid request with json, but without method field (400)
DONE        2.4 User/Token is missing (401)
DONE        2.5 User/Token is present, but have invalid values (401)
DONE        2.6 Request lasts more then allowed (408)
DONE        2.7 Limit of connections reached (429)
DONE        2.8 Any websocket formatted error on request (500)

DONE    3. Local https-server tests, /v1/broadcasts
DONE        3.1 Local https-server tests, /v1/broadcasts (200)
DONE        3.2 /v1/broadcasts: Unauthorized (401)
DONE        3.3 /v1/broadcasts: too many connections (429)

*/

namespace HubPlatform {
namespace Tests {

class LocalHttpServerTests : public LocalHttpServerFixture {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    void TearDown() override;
};

void LocalHttpServerTests::SetUpTestCase() {
    LocalHttpServerFixture::SetUpTestCase();

    getHostname() = "127.0.0.1";
    getPort() = nma().getNmaPort();

    setCredentials( 1, getUUID(), { SslUtils::sha256( getPassword() ) } );
}

void LocalHttpServerTests::TearDownTestCase() {
    LocalHttpServerFixture::TearDownTestCase();
}

void LocalHttpServerTests::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 1 ) );

    LocalHttpServerFixture::TearDown();
}

std::string base64( const std::string& str );

// DONE        1.1 Make valid hub.info.get request
TEST_F( LocalHttpServerTests, requestValidHubInfoPostRequest ) /* 1.1 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;

    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=validHubInfoPostRequest" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );
    ASSERT_EQ( http.code, 200 ) << url << ", error: " << http.text;
    Log::trace( cc::trace( "Result: " ) + cc::j( result ) );
}

// DONE        1.1.1 Make valid hub.item.value.set request (async request)
TEST_F( LocalHttpServerTests, requestValidHubItemValueSet ) /* 1.1.1 */ {
    std::string deviceId = addDevice( makeRemote(),
                                      "testDevice",
                                      "smoke",
                                      "134_3_24",
                                      "security_sensor",
                                      "smoke",
                                      false /*batteryPowered*/,
                                      __LINE__ );

    using namespace CoreD::Model;
    std::string itemName = "item_int";
    int value = 11;
    auto valueType = ValueType::Enum::Int;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::Int,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );

    auto itemId = addItem( makeRemote(), item, __LINE__, nullptr );

    Json request = R"({ "method": "hub.item.value.set", "params": { "_id": "", "value": 22 } })"_json;
    request[ "params" ][ "_id" ] = itemId;
    request[ "params" ][ "value" ] = value * 2;

    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=validHubItemValueSet" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );
    ASSERT_EQ( http.code, 200 ) << url << ", error: " << http.text;

    Log::trace( cc::trace( "Http response: " ) + cc::j( result ) );
    Json response, error;
    // if curlopt_verbose is set, this may contain http headers before json itself:
    if( !result.empty() && result.find( "HTTP/" ) == 0 ) {
        char pattern[] = "\r\n\r\n";
        size_t pos = result.find( pattern );
        if( pos < std::string::npos ) {
            result = result.substr( pos + sizeof( pattern ) - 1 );
        }
    }
    ASSERT_NO_THROW( response = Json::parse( result ) ) << __LINE__;

    ASSERT_EQ( json_get_str( response, "method" ), json_get_str( request, "method" ) );
    ASSERT_TRUE( response.count( "result" ) > 0 );

    checkBroadcast( itemId, "hub.item.updated", {}, __LINE__ );
}

// DONE        1.2 Make invalid request without json (400)
TEST_F( LocalHttpServerTests, requestWithoutJsonError400 ) /* 1.2 */ {
    std::string request = "111";
    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=requestWithoutJsonError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request, timeout, result );

    ASSERT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

// DONE        1.3 Make invalid request with json, but without method field (400)
TEST_F( LocalHttpServerTests, requestNoMethodFieldError400 ) /* 1.3 */ {
    Json request = R"({ "params": {} })"_json;

    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=requestNoMethodFieldError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );

    ASSERT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

// DONE        1.4 User/Token is missed (401)
TEST_F( LocalHttpServerTests, requestAuthIsMissed ) /* 1.4 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;

    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=authIsMissed" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json" };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );

    ASSERT_EQ( http.code, 401 ) << url << ", error: " << http.text;
}

// DONE        1.5 User/Token is present, but have invalid values (401)
TEST_F( LocalHttpServerTests, requestAuthWrongCreds ) /* 1.5 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;

    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=authWrongCreds" );
    int timeout = m_defaultTimeoutSec;
    // echo -n 'bad-user:no-password' | base64 -
    CurlWrapper::Headers headers = { "content-type: application/json",
                                     "authorization: Basic YmFkLXVzZXI6bm8tcGFzc3dvcmQ=" };
    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );
    ASSERT_EQ( http.code, 401 ) << url << ", error: " << http.text;
}

// DONE        1.6 Request lasts more then allowed (408)
TEST_F( LocalHttpServerTests, requestLongRequestError408 ) /* 1.6 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;

    int prevDurationLimit = HttpBridge::getApiBridge().requestDurationLimit();
    const int newDurationLimit = 800;
    const int delayRequestFor = 2000;
    const int timeout = 10000;

    // Reset default duration limit from 60 sec to 4 sec for tests only.
    HttpBridge::getApiBridge().requestDurationLimit() = newDurationLimit;

    std::string url = std::string( "/v1/request?suite=LocalHttpServerTests&test=longRequestError408&__delayed=" ) +
                      std::to_string( delayRequestFor );
    url = makeUrl( url.c_str() );
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );

    HttpBridge::getApiBridge().requestDurationLimit() = prevDurationLimit;
    ASSERT_EQ( http.code, 408 ) << url << ", error: " << http.text;
}

// DONE        1.7 Limit of connections reached (429)
TEST_F( LocalHttpServerTests, requestLimitOfConnectionsError429 ) /* 1.7 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;

    std::string urlDelayed =
        makeUrl( "/v1/request?suite=LocalHttpServerTests&test=limitOfConnectionsError429&__delayed=1000" );
    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=limitOfConnectionsError429" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto& apiBridge = HttpBridge::getApiBridge();
    auto prevLimit = apiBridge.parallelSessionsLimit();
    apiBridge.parallelSessionsLimit() = 1;

    std::thread helperThread = std::thread( [urlDelayed, headers, request, timeout]() {
        std::string result;
        auto http = postUrl( urlDelayed, headers, request.dump(), timeout, result );
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    auto http = postUrl( url, headers, request.dump(), timeout, result );

    apiBridge.parallelSessionsLimit() = prevLimit;

    if( helperThread.joinable() )
        helperThread.join();
    ASSERT_EQ( http.code, 429 ) << url << ", error: " << http.text;
}

// DONE        1.8 Any websocket formatted error on request (500)
TEST_F( LocalHttpServerTests, requestUndefinedMethodError500 ) /* 1.8 */ {
    Json request = R"({ "method": "hub.undefined.method.error", "params": {} })"_json;

    std::string url = makeUrl( "/v1/request?suite=LocalHttpServerTests&test=undefinedMethodError500" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { "content-type: application/json", basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );

    ASSERT_EQ( http.code, 500 ) << url << ", error: " << http.text;
}

// DONE        2.1 Make valid hub.info.get request
TEST_F( LocalHttpServerTests, methodValidHubInfoPostRequest ) /* 2.1 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;

    std::string url =
        makeUrl( "/v1/method/hub/info/get?suite=LocalHttpServerTests&test=methodValidHubInfoPostRequest" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );
    ASSERT_EQ( http.code, 200 ) << url << ", error: " << http.text;

    Log::trace( cc::trace( "Result: " ) + cc::j( result ) );
}

// DONE        2.1.1 Make valid hub.item.value.set request (async request)
TEST_F( LocalHttpServerTests, methodValidHubItemValueSet ) /* 2.1.1 */ {
    std::string deviceId = addDevice( makeRemote(),
                                      "testDevice",
                                      "smoke",
                                      "134_3_24",
                                      "security_sensor",
                                      "smoke",
                                      false /*batteryPowered*/,
                                      __LINE__ );

    using namespace CoreD::Model;
    std::string itemName = "item_int";
    int value = 11;
    auto valueType = ValueType::Enum::Int;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::Int,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );

    auto itemId = addItem( makeRemote(), item, __LINE__, nullptr );

    std::string baseUrl = std::string( "/v1/method/hub/item/value/set?_id=" ) + itemId + std::string( "&value_int=" ) +
                          std::to_string( value * 2 );
    std::string url = makeUrl( baseUrl.c_str() );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );

    ASSERT_EQ( http.code, 200 ) << url << ", error: " << http.text;

    Log::trace( cc::trace( "Http response: " ) + cc::j( result ) );
    Json response, error;
    // if curlopt_verbose is set, this may contain http headers before json itself:
    if( !result.empty() && result.find( "HTTP/" ) == 0 ) {
        char pattern[] = "\r\n\r\n";
        size_t pos = result.find( pattern );
        if( pos < std::string::npos ) {
            result = result.substr( pos + sizeof( pattern ) - 1 );
        }
    }
    ASSERT_NO_THROW( response = Json::parse( result ) ) << __LINE__;

    ASSERT_EQ( json_get_str( response, "method" ), "hub.item.value.set" );
    ASSERT_TRUE( response.count( "result" ) > 0 );

    checkBroadcast( itemId, "hub.item.updated", {}, __LINE__ );
}

// DONE        2.2 Make invalid request without json (400)
TEST_F( LocalHttpServerTests, methodWithoutJsonError400 ) /* 2.2 */ {
    std::string request = "111";
    std::string url = makeUrl( "/v1/method/?suite=LocalHttpServerTests&test=methodWithoutJsonError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );

    ASSERT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

TEST_F( LocalHttpServerTests, methodBadBoolParamError400 ) /* 2.2.1 */ {
    std::string request = "111";
    std::string url = makeUrl(
        "/v1/method/"
        "hub.info.get?boolValue_bool=false_and_garbage&suite=LocalHttpServerTests&test=methodWithoutJsonError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );
    EXPECT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

TEST_F( LocalHttpServerTests, methodBadIntParamError400 ) /* 2.2.2 */ {
    std::string request = "111";
    std::string url = makeUrl(
        "/v1/method/hub.info.get?intValue_int=111zzz&suite=LocalHttpServerTests&test=methodWithoutJsonError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );
    EXPECT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

TEST_F( LocalHttpServerTests, methodBadFloatParamError400 ) /* 2.2.3 */ {
    std::string request = "111";
    std::string url = makeUrl(
        "/v1/method/hub.info.get?floatValue_float=fff&suite=LocalHttpServerTests&test=methodWithoutJsonError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );
    EXPECT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

// DONE        2.3 Make invalid request with json, but without method field (400)
TEST_F( LocalHttpServerTests, methodNoMethodError400 ) /* 2.3 */ {
    Json request = R"({ "params": {} })"_json;

    std::string url = makeUrl( "/v1/method?suite=LocalHttpServerTests&test=methodNoMethodFieldError400" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = postUrl( url, headers, request.dump(), timeout, result );

    ASSERT_EQ( http.code, 400 ) << url << ", error: " << http.text;
}

// DONE        2.4 User/Token is missed (401)
TEST_F( LocalHttpServerTests, methodAuthIsMissed ) /* 2.4 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;
    std::string url = makeUrl( "/v1/method/hub/info/get?suite=LocalHttpServerTests&test=authIsMissed" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers;

    std::string result;
    auto http = getUrl( url, headers, timeout, result );

    ASSERT_EQ( http.code, 401 ) << url << ", error: " << http.text;
}

// DONE        2.5 User/Token is present, but have invalid values (401)
TEST_F( LocalHttpServerTests, methodAuthWrongCreds ) /* 2.5 */ {
    std::string url = makeUrl( "/v1/method/hub.info.get?suite=LocalHttpServerTests&test=methodAuthWrongCreds" );
    int timeout = m_defaultTimeoutSec;
    // echo -n 'bad-user:no-password' | base64 -
    CurlWrapper::Headers headers = { "authorization: Basic YmFkLXVzZXI6bm8tcGFzc3dvcmQ=" };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );
    ASSERT_EQ( http.code, 401 ) << url << ", error: " << http.text;
}

// DONE        2.6 Request lasts more then allowed (408)
TEST_F( LocalHttpServerTests, methodLongRequestError408 ) /* 2.6 */ {
    int prevDurationLimit = HttpBridge::getApiBridge().requestDurationLimit();
    const int newDurationLimit = 800;
    const int delayRequestFor = 2000;
    const int timeout = 10000;

    // Reset default duration limit from 60 sec to 4 sec for tests only.
    HttpBridge::getApiBridge().requestDurationLimit() = newDurationLimit;

    std::string url =
        std::string( "/v1/method/hub/info/get?suite=LocalHttpServerTests&test=longRequestError408&__delayed=" ) +
        std::to_string( delayRequestFor );
    url = makeUrl( url.c_str() );
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );

    HttpBridge::getApiBridge().requestDurationLimit() = prevDurationLimit;
    ASSERT_EQ( http.code, 408 ) << url << ", error: " << http.text;
}

// DONE        2.7 Limit of connections reached (429)
TEST_F( LocalHttpServerTests, methodLimitOfConnectionsError429 ) /* 2.7 */ {
    Json request = R"({ "method": "hub.info.get", "params": {} })"_json;
    std::string urlDelayed =
        makeUrl( "/v1/request?suite=LocalHttpServerTests&test=limitOfConnectionsError429&__delayed=1000" );
    std::string url = makeUrl( "/v1/method?suite=LocalHttpServerTests&test=limitOfConnectionsError429" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto& apiBridge = HttpBridge::getApiBridge();
    auto prevLimit = apiBridge.parallelSessionsLimit();
    apiBridge.parallelSessionsLimit() = 1;

    std::thread helperThread = std::thread( [urlDelayed, headers, request, timeout]() {
        std::string result;
        auto http = postUrl( urlDelayed, headers, request.dump(), timeout, result );
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

    auto http = getUrl( url, headers, timeout, result );

    apiBridge.parallelSessionsLimit() = prevLimit;

    if( helperThread.joinable() )
        helperThread.join();
    ASSERT_EQ( http.code, 429 ) << url << ", error: " << http.text;
}

// DONE        2.8 Any websocket formatted error on request (500)
TEST_F( LocalHttpServerTests, methodUndefinedMethodError500 ) /* 2.8 */ {
    std::string url =
        makeUrl( "/v1/method/hub/undefined/method/error?suite=LocalHttpServerTests&test=undefinedMethodError500" );
    int timeout = m_defaultTimeoutSec;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto http = getUrl( url, headers, timeout, result );

    ASSERT_EQ( http.code, 500 ) << url << ", error: " << http.text;
}

TEST_F( LocalHttpServerTests, broadcasts ) /* 3.1 */ {
    Json request = R"({
    "method": "hub.extensions.plugin.run",
    "params": {
        "script": "HUB:http_test/scripts/http_events",
        "scriptParams": { "command": "request" }
    }})"_json;

    std::string urlDelayedEmitBroadcast = makeInsecureUrl( "/v1/request?emit_broadcasts" );

    std::string urlBroadcasts = makeInsecureUrl( "/v1/broadcasts" );
    int timeoutSec = 10;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    auto& apiBridge = HttpBridge::getApiBridge();

    std::promise< HttpResult > delayedRequest;
    std::future< HttpResult > future = delayedRequest.get_future();

    // Use Http and events to call to the /v1/broadcasts endpoint
    {
        std::string pluginName = test_info_->name();
        CommonHttp::Http http;
        Tests::EventReceiver eventReceiver( http, pluginName );

        namespace HttpEvents = HubPlatform::CommonHttp::Resources::HttpEvents;
        http.on( HttpEvents::HttpConnectionClosed, eventReceiver.makeConnectionClosed() );
        http.on( HttpEvents::HttpDataReceived, eventReceiver.makeDataReceivedAndClose() );

        CommonHttp::HttpOptions httpOptions;
        httpOptions.url = urlBroadcasts;
        httpOptions.handler = "fake-handler";
        httpOptions.timeoutMs = m_defaultTimeoutSec * 1000;
        httpOptions.keepAlive = true;
        httpOptions.newDataCache = false;
        httpOptions.verbose = true;
        httpOptions.headers.emplace( basicAuthHeaderPair( getPassword() ) );
        std::string requestId;

        try {
            requestId = http.request( pluginName, httpOptions );
        }
        catch( const std::exception& e ) {
            FAIL() << "http.request thrown an exception: " << e.what();
        }

        std::thread helperThread =
            std::thread( [urlDelayedEmitBroadcast, headers, request, timeoutSec, &delayedRequest]() {
                Log::trace( cc::trace( "Helper thread: " ) + "started" );
                std::this_thread::sleep_for( std::chrono::seconds( timeoutSec / 3 ) );

                std::string result;

                Log::trace( "Sync request emit_broadcast started..." );
                HttpResult http = postUrl( urlDelayedEmitBroadcast, headers, request.dump(), timeoutSec, result );
                delayedRequest.set_value( http );
                Log::trace( "Sync request emit_broadcast finished. Helper thread finished." );
            } );

        Log::trace( cc::trace( "/v1/broadcasts waiting for helper thread: " ) + result );
        auto waitResult = future.wait_for( std::chrono::seconds( timeoutSec + 1 ) );
        ASSERT_EQ( waitResult, std::future_status::ready );

        HttpResult asyncResult = future.get();
        Log::trace( "Async http result obtained: code=" + cc::num10( asyncResult.code ) +
                    ", text=" + cc::note( asyncResult.text ) );

        if( helperThread.joinable() )
            helperThread.join();

        auto received = eventReceiver.waitForEvent( m_defaultTimeoutSec, "data_received event", __FILE__, __LINE__ );
        LOGGER_INFO( "Event received: " + received.first + ", data = " + cc::j( received.second ) );

        Json header;
        const char* const keyHeaders = "headers";
        const char* const valueAccessControl = "access-control-allow-origin";
        const char* const valueCacheControl = "cache-control";

        EXPECT_TRUE( received.second.count( keyHeaders ) > 0 ) << "Should have response headers";
        EXPECT_TRUE( json_get( received.second, { keyHeaders, valueCacheControl }, header ) )
            << "Should contain 'cache-control' header";
        EXPECT_TRUE( json_get( received.second, { keyHeaders, valueAccessControl }, header ) )
            << "Should contain 'cache-control' header";

        auto closed = eventReceiver.waitForEvent( m_defaultTimeoutSec, "connection_closed event", __FILE__, __LINE__ );
        LOGGER_INFO( "Event received: " + closed.first + ", data = " + cc::j( received.second ) );
    }
}

// DONE        3.2 /v1/broadcasts: Unauthorized (401)
TEST_F( LocalHttpServerTests, broadcastsNotAuthorized401 ) /* 3.2 */ {
    std::string url = makeInsecureUrl( "/v1/broadcasts" );
    int timeoutSec = m_defaultTimeoutSec;
    std::string result;

    auto http = getUrl( url, CurlWrapper::Headers{}, timeoutSec, result );

    ASSERT_EQ( http.code, 401 ) << url << ", error: " << http.text;
}

// DONE        3.3 /v1/broadcasts: too many connections (429)
TEST_F( LocalHttpServerTests, broadcastsTooManyConnection429 ) /* 3.3 */ {
    std::string urlDelayed = makeUrl( "/v1/broadcasts?suite=LocalHttpServerTests&test=broadcasts&start_broadcasts" );

    const int connectionsLimit = 5;
    auto& apiBridge = HttpBridge::getApiBridge();
    auto prevLimit = apiBridge.parallelSessionsLimit();
    apiBridge.parallelSessionsLimit() = connectionsLimit;

    std::string url = makeUrl( "/v1/broadcasts?suite=LocalHttpServerTests&test=broadcastsTooManyConnection429" );
    int timeoutSec = 4;
    CurlWrapper::Headers headers = { basicAuthHeader( getPassword() ) };

    std::string result;
    assert( connectionsLimit >= 2 );
    int repeats = connectionsLimit - 1;
    std::atomic_int threadCounter{ 0 };

    std::vector< std::thread > threads;
    for( int i = repeats; --i >= 0; ) {
        std::thread helperThread = std::thread( [&]() {
            int thisThreadNum = ++threadCounter;
            Log::trace( cc::trace( "Helper thread: " ) + "started, repeat count: " + cc::num10( thisThreadNum ) + "/" +
                        cc::num10( repeats ) );

            std::string result;
            CurlWrapper::curlCustomGet( urlDelayed, headers, "", "", "", result, timeoutSec, timeoutSec );
            Log::trace( cc::trace( "Helper thread: " ) + cc::num10( thisThreadNum ) + " finished" );
        } );
        threads.emplace_back( std::move( helperThread ) );
    }

    // wait for all the threads to start
    while( threadCounter < repeats )
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    // wait they all start listening broadcasts
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    Log::trace( cc::trace( "/v1/broadcasts starting: " ) + result );
    auto http = getUrl( url, headers, timeoutSec, result );
    Log::trace( cc::trace( "/v1/broadcasts download complete: " ) + result );

    Log::trace( cc::trace( "/v1/broadcasts output: " ) + result );
    ASSERT_EQ( http.code, 429 ) << url << ", error: " << http.text;

    for( auto& thread : threads ) {
        if( thread.joinable() )
            thread.join();
    }
}

}  // namespace Tests
}  // namespace HubPlatform