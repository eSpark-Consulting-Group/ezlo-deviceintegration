#include "metrics/internal_metrics/internal_metrics.h"
#include "resources/test_resources.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

class InternalMetricsSuite : public ::testing::Test {};

TEST_F( InternalMetricsSuite, emptyResponse ) {
    Metrics::InternalMetrics metrics;

    auto report = metrics.toJson();
    Log::setLocalLogLevel( Log::TRACE );
    Log::info( cc::j( report ) );

    EXPECT_EQ( json_get_int( report, "sentAt" ), 0 );
    EXPECT_EQ( json_get_int( report, "errorAt" ), 0 );
    EXPECT_EQ( json_get_int( report, "packets" ), 0 );
    EXPECT_EQ( json_get_int( report, "sentSize" ), 0 );
    EXPECT_EQ( json_get_int( report, "errors" ), 0 );
    Json bus;
    EXPECT_TRUE( json_get( report, "bus", bus ) );
}

TEST_F( InternalMetricsSuite, sentOk ) {
    Metrics::InternalMetrics metrics;
    metrics.registerSend( 1234 );
    auto report = metrics.toJson();
    EXPECT_EQ( json_get_int( report, "packets" ), 1 );
    EXPECT_EQ( json_get_int( report, "sentSize" ), 1234 );
    EXPECT_GT( json_get_int( report, "sentAt" ), 0 );
}

TEST_F( InternalMetricsSuite, sentError ) {
    Metrics::InternalMetrics metrics;
    metrics.registerError();
    auto rep = metrics.toJson();
    EXPECT_EQ( json_get_int( rep, "packets" ), 0 );
    EXPECT_EQ( json_get_int( rep, "errors" ), 1 );
    EXPECT_GT( json_get_int( rep, "errorAt" ), 0 );
}

TEST_F( InternalMetricsSuite, busRequests ) {
    Metrics::InternalMetrics metrics;
    metrics.bus().registerRequest();
    auto report = metrics.toJson();
    Json bus;
    ASSERT_TRUE( json_get( report, "bus", bus ) );
    EXPECT_EQ( json_get_int( bus, "requests" ), 1 );
}

TEST_F( InternalMetricsSuite, busErrors ) {
    Metrics::InternalMetrics metrics;
    metrics.bus().registerError();
    auto report = metrics.toJson();
    Json bus;
    ASSERT_TRUE( json_get( report, "bus", bus ) );
    EXPECT_EQ( json_get_int( bus, "errors" ), 1 );
}

TEST_F( InternalMetricsSuite, busStructure ) {
    Metrics::InternalMetrics metrics;
    metrics.bus().registerError();
    auto report = metrics.toJson();
    Json bus;
    ASSERT_TRUE( json_get( report, "bus", bus ) );

    std::set< std::string > fields = {
        "requests",          "errors",           "attempts",           "attemptAt",       "connectionChecks",
        "connectionCheckAt", "reconnectSuccess", "reconnectSuccessAt", "reconnectFailed", "reconnectFailedAt"
    };

    for( const auto& field : fields ) {
        EXPECT_TRUE( json_contains( bus, field ) ) << "Field should be present: " << field;
    }
}

// TODO: EFWR-4524: bus metrics tests

}  // namespace UT
}  // namespace HubPlatform
