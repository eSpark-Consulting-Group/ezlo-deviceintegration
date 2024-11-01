#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "common_test_resources/resources/cored_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include <gtest/gtest.h>
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Tests {

static const char* const Feature = "feature";

class FeatureStateEventsTests : public SceneManagerTestsFixture {
public:
    void initFeature( const std::string& version, bool status );
    void setFeatureStatus();
    void testFeatureStateVersion( const std::string& version, const std::string& op );
};

void FeatureStateEventsTests::initFeature( const std::string& version, bool status ) {
    auto& dbClient = nma().getDBClient();
    const bool editable = true;
    auto table = CoreD::Model::Resources::DBNames::FeaturesDB;
    Json record = JSON( "_id" << Feature << "editable" << editable << "is_custom" << false << "status" << status
                              << "version" << version );
    dbClient.insert( table, record );
}

void FeatureStateEventsTests::setFeatureStatus() {
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.feature.status.set" },
                                                 { "params",
                                                   JSON( "feature" << Feature << "status"
                                                                   << "on"
                                                                   << "version"
                                                                   << "1.0" ) } } );

    JsonObject broadcast;
    broadcast = waitForBroadcast( "hub.feature.updated", getCodeLineString( __LINE__ ) );

    EXPECT_FALSE( broadcast.empty() ) << cc::j( broadcast );
    EXPECT_EQ( json_get_str( broadcast, "id" ), "ui_broadcast" ) << cc::j( broadcast );
}

void FeatureStateEventsTests::testFeatureStateVersion( const std::string& version, const std::string& op ) {
    initFeature( version, false );
    std::string block;
    block = replaceTokensInString(
        isFeatureStateWhenBlock,
        { { "__NAME__", Feature }, { "__STATUS__", "true" }, { "__VERSION__", "2.0" }, { "__COMPARATOR__", op } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareFeatureState" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );
    LOGGER_TRACE( "request: " + cc::j( request ) );
    SceneID sceneId = createScene( request, __LINE__ );
    setFeatureStatus();
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( FeatureStateEventsTests, testIsFeatureVersionCompare ) {  // 1
    testFeatureStateVersion( "2.0", "==" );
    testFeatureStateVersion( "1.0", "<" );
    testFeatureStateVersion( "1.0", "<=" );
    testFeatureStateVersion( "2.2", ">" );
    testFeatureStateVersion( "2.1", ">=" );
    testFeatureStateVersion( "3.0", "!=" );
}

TEST_F( FeatureStateEventsTests, testIsFeatureStateTestInit ) {  // 2

    initFeature( "1.0", true );
    std::string block;
    block = replaceTokensInString(
        isFeatureStateWhenBlock,
        { { "__NAME__", Feature }, { "__STATUS__", "true" }, { "__VERSION__", "1.0" }, { "__COMPARATOR__", "==" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareFeatureState" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );
    LOGGER_TRACE( "request: " + cc::j( request ) );
    SceneID sceneId = createScene( request, __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( FeatureStateEventsTests, testIsFeatureStateOnlyVersionTest ) {  // 3
    initFeature( "1.0", false );
    std::string block;
    block = replaceTokensInString( isFeatureStateWithoutStatusWhenBlock,
                                   { { "__NAME__", Feature }, { "__VERSION__", "1.0" }, { "__COMPARATOR__", "==" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareFeatureState" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );
    LOGGER_TRACE( "request: " + cc::j( request ) );
    SceneID sceneId = createScene( request, __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
    setFeatureStatus();
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

TEST_F( FeatureStateEventsTests, testIsFeatureStateOnlySatusTest ) {  // 4
    initFeature( "1.0", false );
    std::string block;
    block = replaceTokensInString( isFeatureStateWithoutVersionWhenBlock,
                                   { { "__NAME__", Feature }, { "__STATUS__", "true" } } );

    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "compareFeatureState" }, { "__WHEN_BLOCKS__", block }, { "__THEN_BLOCKS__", "" } } );

    LOGGER_TRACE( "request: " + cc::j( request ) );
    SceneID sceneId = createScene( request, __LINE__ );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
    setFeatureStatus();
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    deleteScene( sceneId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
