#pragma once

#include "common_test_resources/fixtures/core_test_fixture.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/resources.h"
#include "http_bridge/taskloop_handlers.h"
#include "uid/handlers/hub_offline_commands_handler.h"
#include "common_utils/curl_wrapper.h"
#include "system_utils/environment.h"
#include "json_utils/json_fwd.h"
#include "ssl_utils/sha256.h"
#include "tiny/tiny_utils.h"
#include <deque>
#include <string>
#include <mutex>
#include <atomic>

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class LocalHttpServerFixture : public CoreNmaTest {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    static std::string makeHostName( unsigned port = 0 );
    static std::string makeUrl( const char* path, const char* hostName = "", const char* proto = "" );
    static std::string makeInsecureUrl( const char* path, const ::testing::TestInfo* testInfo = nullptr );

    static int& getPort();
    static int& getSecurePort();
    static std::string& getHostname();

    static void setCredentials( int version,
                                const std::string& controllerUuid,
                                const std::vector< std::string >& keys );
    static bool authenticateUser( const std::string& user, const std::string& password );

    static const std::string& getSerial();
    static const std::string& getUUID();
    static const std::string& getPassword();

    static std::string basicAuthHeader( const std::string& password );
    static std::pair< std::string, std::string > basicAuthHeaderPair( const std::string& password );
    using Headers = CurlWrapper::Headers;
    struct HttpResult {
        int code{ 0 };
        std::string text;

    public:
        explicit HttpResult( int httpCode = 0, const char* httpError = "" )
        : code( httpCode )
        , text( httpError ) {}
    };
    static HttpResult getUrl( const std::string& url, const Headers& headers, int timeoutSec, std::string& result );
    static HttpResult postUrl( const std::string& url,
                               const Headers& headers,
                               const std::string& payload,
                               int timeoutSec,
                               std::string& result );
    static HttpResult postUrlAsMethod( const std::string& url,
                                       const Headers& headers,
                                       const std::string& payload,
                                       int timeoutSec,
                                       std::string& result );

    void SetUp() override;
    void TearDown() override;

protected:
    unsigned m_defaultTimeoutSec{ 45 };
};

}  // namespace Tests
}  // namespace HubPlatform