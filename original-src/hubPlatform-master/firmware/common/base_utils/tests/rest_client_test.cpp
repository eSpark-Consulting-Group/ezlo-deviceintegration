#include "base_utils/rest_client.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

using namespace ::testing;
using ::testing::_;
using ::testing::Invoke;

const std::string restServerBaseUrl = "http://localhost:12000";

class RestClientSessionTestSuite : public Test {
public:
    void SetUp() override {
        m_restClientSession.reset( new RestClientSession( restServerBaseUrl ) );
    }
    std::string formUrl( const RestClientSession::RequestPtr& request ) {
        return m_restClientSession->formUrl( request );
    }
    std::unique_ptr< RestClientSession > m_restClientSession;
};

TEST_F( RestClientSessionTestSuite, formUrl ) {
    {
        auto request = std::make_shared< RestClientSession::Request >( "/path/very/long" );
        const std::string expectedUrl = restServerBaseUrl + "/path/very/long";
        const std::string url = formUrl( request );
        EXPECT_EQ( expectedUrl, url );
    }

    {
        RestClientSession::QueryParams params = { { "param1", "value1" }, { "param2", "value2" } };
        auto request = std::make_shared< RestClientSession::Request >( "/path", params );
        const std::string expectedUrl = restServerBaseUrl + "/path" + "?param1=value1&param2=value2";
        const std::string url = formUrl( request );
        EXPECT_EQ( expectedUrl, url );
    }

    {
        auto request = std::make_shared< RestClientSession::Request >( "" );
        const std::string expectedUrl = restServerBaseUrl;
        const std::string url = formUrl( request );
        EXPECT_EQ( expectedUrl, url );
    }
}

TEST_F( RestClientSessionTestSuite, formUrlWithEncoding ) {
    {
        RestClientSession::QueryParams params = { { "param", "[ \"number\" : 5 ]" } };
        auto request = std::make_shared< RestClientSession::Request >( "/path", params );
        const std::string expectedUrl = restServerBaseUrl + "/path" + "?param=%5B%20%22number%22%20%3A%205%20%5D";
        const std::string url = formUrl( request );
        EXPECT_EQ( expectedUrl, url );
    }

    {
        RestClientSession::QueryParams params = { { "Hello Günter", "value" }, { "param2", "value2" } };
        auto request = std::make_shared< RestClientSession::Request >( "/path", params );
        const std::string expectedUrl = restServerBaseUrl + "/path" + "?Hello%20G%C3%BCnter=value&param2=value2";
        const std::string url = formUrl( request );
        EXPECT_EQ( expectedUrl, url );
    }
}

TEST_F( RestClientSessionTestSuite, asyncRequests ) {
    auto request1 = std::make_shared< RestClientSession::Request >( "/path" );
    auto request2 = std::make_shared< RestClientSession::Request >( "/path" );
    auto futureResponse1 = m_restClientSession->sendRequestAsync( request1 );
    auto futureResponse2 = m_restClientSession->sendRequestAsync( request2 );

    EXPECT_THROW( futureResponse1.get(), RestClientException );
    EXPECT_THROW( futureResponse2.get(), RestClientException );
}

TEST_F( RestClientSessionTestSuite, syncRequests ) {
    auto request1 = std::make_shared< RestClientSession::Request >( "/path" );
    EXPECT_THROW( m_restClientSession->sendRequest( request1 ), RestClientException );
}

TEST_F( RestClientSessionTestSuite, mixedRequests ) {
    auto request2 = std::make_shared< RestClientSession::Request >( "/path" );
    auto futureResponse2 = m_restClientSession->sendRequestAsync( request2 );

    auto request1 = std::make_shared< RestClientSession::Request >( "/path" );
    EXPECT_THROW( m_restClientSession->sendRequest( request1 ), RestClientException );

    EXPECT_THROW( futureResponse2.get(), RestClientException );
}

TEST_F( RestClientSessionTestSuite, asyncRequestsWithPause ) {
    auto request1 = std::make_shared< RestClientSession::Request >( "/path" );
    auto futureResponse1 = m_restClientSession->sendRequestAsync( request1 );
    EXPECT_THROW( futureResponse1.get(), RestClientException );

    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    auto request2 = std::make_shared< RestClientSession::Request >( "/path" );
    auto futureResponse2 = m_restClientSession->sendRequestAsync( request2 );
    EXPECT_THROW( futureResponse2.get(), RestClientException );
}

}  // namespace UT
}  // namespace HubPlatform
