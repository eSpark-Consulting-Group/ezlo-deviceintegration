#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "jsonrpc/jsonrpc.h"
#include "common_test_resources/resources/cored_resources.h"
#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Feature update test
DONE    2. Feature no update test
*/

namespace HubPlatform {
namespace Tests {

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

static const char* const Feature = "feature";

class FeatureUpdateTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    static void SetUpTestCase();
};

void FeatureUpdateTests::SetUp() {
    CoreNmaTest::SetUp();
    clearExistingDevices( __LINE__ );
    callingFile( __FILE__ );
}

void FeatureUpdateTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void FeatureUpdateTests::SetUpTestCase() {
    auto& dbClient = nma().getDBClient();
    const bool editable = true;
    auto table = CoreD::Model::Resources::DBNames::FeaturesDB;
    Json record = JSON( "_id" << Feature << "editable" << editable << "is_custom" << false << "status"
                              << "off"
                              << "version"
                              << "1.0" );
    dbClient.insert( table, record );
}

TEST_F( FeatureUpdateTests, featureUpdated ) {
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.feature.status.set" },
                                                 { "params",
                                                   JSON( "feature" << Feature << "status"
                                                                   << "on" ) } } );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.feature.updated", getCodeLineString( __LINE__ ) );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.feature.updated" ) << cc::j( broadcast );
    EXPECT_TRUE( !broadcast[ "result" ].empty() );
    EXPECT_EQ( broadcast[ "result" ][ "_id" ], Feature );
    EXPECT_EQ( broadcast[ "result" ][ "status" ], true );
}

TEST_F( FeatureUpdateTests, featureNotUpdated ) {
    auto& dbClient = nma().getDBClient();
    const bool editable = true;

    auto table = CoreD::Model::Resources::DBNames::FeaturesDB;
    Json record = JSON( "_id" << Feature << "editable" << editable << "is_custom" << false << "status"
                              << "off"
                              << "version"
                              << "1.0" );
    dbClient.insert( table, record );

    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.feature.status.set" },
                                                 { "params",
                                                   JSON( "feature" << Feature << "status"
                                                                   << "off" ) } } );

    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
}

}  // namespace Tests
}  // namespace HubPlatform
