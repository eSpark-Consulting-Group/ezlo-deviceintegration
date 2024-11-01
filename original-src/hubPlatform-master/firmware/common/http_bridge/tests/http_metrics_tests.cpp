#include "http_bridge/http_metrics.h"
#include <gtest/gtest.h>
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace UT {

using namespace HubPlatform::HttpBridge;

class HttpMetricsTests : public ::testing::Test {};

TEST_F( HttpMetricsTests, toJsonRequireFeildsList ) {
    HttpServerMetrics metrics;

    Json serialized = metrics.toJson();
    EXPECT_TRUE( json_contains( serialized, "received" ) );
    EXPECT_TRUE( json_contains( serialized, "success" ) );
    EXPECT_TRUE( json_contains( serialized, "errors" ) );
    EXPECT_TRUE( json_contains( serialized, "wrongAuth" ) );
    EXPECT_TRUE( json_contains( serialized, "timedOut" ) );
    EXPECT_TRUE( json_contains( serialized, "limitReached" ) );
    EXPECT_TRUE( json_contains( serialized, "rxBytes" ) );
    EXPECT_TRUE( json_contains( serialized, "txBytes" ) );
    EXPECT_TRUE( json_contains( serialized, "unhandled" ) );
    EXPECT_TRUE( json_contains( serialized, "parallelRequests" ) );
}

TEST_F( HttpMetricsTests, incrementers ) {
    HttpServerMetrics metrics;

    metrics.incrementReceivedRequest();
    metrics.incrementHandledRequest();
    metrics.incrementErrorRequest();
    metrics.incrementRequestTimedOut();
    metrics.incrementUnhandledRequest();
    metrics.incrementWrongAuthRequest();
    metrics.incrementRequestsLimitReached();

    Json serialized = metrics.toJson();
    EXPECT_EQ( json_get_int( serialized, "received" ), 1 );
    EXPECT_EQ( json_get_int( serialized, "success" ), 1 );

    EXPECT_EQ( json_get_int( serialized, "errors" ), 4 ) << "Cumulative number of errors, should be 4";
    EXPECT_EQ( json_get_int( serialized, "wrongAuth" ), 1 );
    EXPECT_EQ( json_get_int( serialized, "timedOut" ), 1 );
    EXPECT_EQ( json_get_int( serialized, "limitReached" ), 1 );
    EXPECT_EQ( json_get_int( serialized, "unhandled" ), 1 );
}

TEST_F( HttpMetricsTests, adders ) {
    HttpServerMetrics metrics;

    size_t rxBytes = 1000;
    metrics.appendRxBytes( rxBytes );

    size_t txBytes = 2000;
    metrics.appendTxBytes( txBytes );

    size_t parallelRequests = 95;
    metrics.parallelRequests( parallelRequests );

    Json serialized = metrics.toJson();
    EXPECT_EQ( json_get_int( serialized, "rxBytes" ), rxBytes );
    EXPECT_EQ( json_get_int( serialized, "txBytes" ), txBytes );
    EXPECT_EQ( json_get_int( serialized, "parallelRequests" ), parallelRequests );
}

}  // namespace UT
}  // namespace HubPlatform