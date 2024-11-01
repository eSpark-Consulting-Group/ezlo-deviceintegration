#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "scenes_manager/test_remote_interface/test_scenes_remote_interface.h"
#include "base_utils/gregorian_calendar.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE        1 Date range ( isDateRange condition )
DONE            1.1 Errors
DONE            1.2 daily
DONE            1.3 monthly
DONE            1.4 yearly
DONE            1.5 years
DONE            1.6 and operator
DONE            1.7 default values
DONE            1.8 scene not triggered on creation if the time range already begun
DONE            1.9 Event State

*/

class SceneDateRangeTests : public SceneManagerTestsFixture {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    void checkSceneStatus( const JsonObject& response, const std::string& state );
    void checkTimeSceneList( const std::string& sceneId, const std::string& executionDate );
};

void SceneDateRangeTests::TearDown() {
    SceneManagerTestsFixture::TearDown();
}

void SceneDateRangeTests::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

void SceneDateRangeTests::checkSceneStatus( const JsonObject& response, const std::string& state ) {
    JsonObject result, status;
    JsonArray when;
    json_get_obj( response, "result", result );
    json_get_obj( result, "status", status );
    ASSERT_TRUE( json_contains( status, "when" ) );
    json_get_arr( status, "when", when );
    ASSERT_EQ( json_get_str( when[ 0 ], "state" ), state );
    ASSERT_EQ( json_get_str( status, "state" ), state );
}

void SceneDateRangeTests::checkTimeSceneList( const std::string& sceneId, const std::string& executionDate ) {
    JsonObject result;
    auto timeScenesList = nma().sendtoHubAndGetAnswer( getTimeScenesList );
    JsonArray timeScenesArray = JSON_EMPTY_ARR;
    ASSERT_TRUE( json_get_obj( timeScenesList, "result", result ) );
    ASSERT_TRUE( json_get_arr( result, "timeScenes", timeScenesArray ) );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "sceneId" ), sceneId );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "sceneName" ), "IsDateRange" );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "methodName" ), "isDateRange" );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "executionDate" ), executionDate );
}

TEST_F( SceneDateRangeTests, isDateRangeErrors ) {  // 1.1
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T09:19:58+0000" ).toSeconds() );

    auto maxYear = std::to_string( std::numeric_limits< uint16_t >::max() );
    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "" },
                                                { "__START_DAY__", "1" },
                                                { "__START_MONTH__", "1" },
                                                { "__START_YEAR__", "1" },
                                                { "__END_TIME__", "17:10" },
                                                { "__END_DAY__", "31" },
                                                { "__END_MONTH__", "12" },
                                                { "__END_YEAR__", maxYear } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ),
               "ezlo.scenes.block.when.time.format" );  //"ezlo.scenes.block.when.date.range.not.defined"
    EXPECT_EQ( json_get_str( error, "message" ),
               "Wrong time format" );  //"Start or End of date range should be defined"

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "1" },
                                           { "__START_MONTH__", "1" },
                                           { "__START_YEAR__", "1" },
                                           { "__END_TIME__", "" },
                                           { "__END_DAY__", "31" },
                                           { "__END_MONTH__", "12" },
                                           { "__END_YEAR__", maxYear } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.time.format" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong time format" );

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "0" },
                                           { "__START_MONTH__", "1" },
                                           { "__START_YEAR__", "1" },
                                           { "__END_TIME__", "17:10" },
                                           { "__END_DAY__", "31" },
                                           { "__END_MONTH__", "12" },
                                           { "__END_YEAR__", maxYear } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.date" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong date" );

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "1" },
                                           { "__START_MONTH__", "0" },
                                           { "__START_YEAR__", "1" },
                                           { "__END_TIME__", "17:10" },
                                           { "__END_DAY__", "31" },
                                           { "__END_MONTH__", "12" },
                                           { "__END_YEAR__", maxYear } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.date" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong date" );

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "1" },
                                           { "__START_MONTH__", "1" },
                                           { "__START_YEAR__", "0" },
                                           { "__END_TIME__", "17:10" },
                                           { "__END_DAY__", "31" },
                                           { "__END_MONTH__", "12" },
                                           { "__END_YEAR__", maxYear } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.date" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong date" );

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "1" },
                                           { "__START_MONTH__", "1" },
                                           { "__START_YEAR__", "1" },
                                           { "__END_TIME__", "17:10" },
                                           { "__END_DAY__", "0" },
                                           { "__END_MONTH__", "12" },
                                           { "__END_YEAR__", maxYear } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.date" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong date" );

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "1" },
                                           { "__START_MONTH__", "1" },
                                           { "__START_YEAR__", "1" },
                                           { "__END_TIME__", "17:10" },
                                           { "__END_DAY__", "31" },
                                           { "__END_MONTH__", "0" },
                                           { "__END_YEAR__", maxYear } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.date" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong date" );

    isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                         { { "__START_TIME__", "17:10" },
                                           { "__START_DAY__", "1" },
                                           { "__START_MONTH__", "1" },
                                           { "__START_YEAR__", "1" },
                                           { "__END_TIME__", "17:10" },
                                           { "__END_DAY__", "31" },
                                           { "__END_MONTH__", "12" },
                                           { "__END_YEAR__", "0" } } );
    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    answer = nma().sendtoHubAndGetAnswer( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code" ), -32500 );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.scenes.block.when.date" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong date" );
}

TEST_F( SceneDateRangeTests, isDateRangeDaily ) {  // 1.2
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T09:19:58+0000" ).toSeconds() );

    auto maxYear = std::to_string( std::numeric_limits< uint16_t >::max() );
    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "10:20" },
                                                { "__START_DAY__", "1" },
                                                { "__START_MONTH__", "1" },
                                                { "__START_YEAR__", "1" },
                                                { "__END_TIME__", "17:10" },
                                                { "__END_DAY__", "31" },
                                                { "__END_MONTH__", "12" },
                                                { "__END_YEAR__", maxYear } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T10:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T17:10:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-02T10:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-02T17:10:00+0000" ).toSeconds() );
}

TEST_F( SceneDateRangeTests, isDateRangeMonthly ) {  // 1.3
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T09:19:58+0000" ).toSeconds() );

    auto maxYear = std::to_string( std::numeric_limits< uint16_t >::max() );
    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "15:20" },
                                                { "__START_DAY__", "5" },
                                                { "__START_MONTH__", "1" },
                                                { "__START_YEAR__", "1" },
                                                { "__END_TIME__", "10:10" },
                                                { "__END_DAY__", "15" },
                                                { "__END_MONTH__", "12" },
                                                { "__END_YEAR__", maxYear } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-15T10:10:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-02-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-02-15T10:10:00+0000" ).toSeconds() );
}

TEST_F( SceneDateRangeTests, isDateRangeYearly ) {  // 1.4
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T09:19:58+0000" ).toSeconds() );

    auto maxYear = std::to_string( std::numeric_limits< uint16_t >::max() );
    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "15:20" },
                                                { "__START_DAY__", "5" },
                                                { "__START_MONTH__", "2" },
                                                { "__START_YEAR__", "1" },
                                                { "__END_TIME__", "10:10" },
                                                { "__END_DAY__", "15" },
                                                { "__END_MONTH__", "10" },
                                                { "__END_YEAR__", maxYear } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-02-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-10-15T10:10:00+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-02-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-10-15T10:10:00+0000" ).toSeconds() );
}

TEST_F( SceneDateRangeTests, isDateRangeYears ) {  // 1.5
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-04T15:19:58+0000" ).toSeconds() );

    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "15:20" },
                                                { "__START_DAY__", "5" },
                                                { "__START_MONTH__", "3" },
                                                { "__START_YEAR__", "2021" },
                                                { "__END_TIME__", "10:10" },
                                                { "__END_DAY__", "2" },
                                                { "__END_MONTH__", "9" },
                                                { "__END_YEAR__", "2022" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-03-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2022-09-02T10:10:00+0000" ).toSeconds() );

    deleteScenes();

    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 4 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2022-09-03T10:10:00+0000" ).toSeconds() );
    sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-03-05T00:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2022-09-02T23:59:00+0000" ).toSeconds() );
}

TEST_F( SceneDateRangeTests, isDateRangeAndOperator ) {  // 1.6
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-04T15:19:58+0000" ).toSeconds() );
    auto deviceId = addDevice( coreRemoteInterface,
                               "testDevice1",
                               "smoke",
                               "134_3_24",
                               "security_sensor",
                               "smoke",
                               false /*batteryPowered*/,
                               __LINE__ );

    auto itemId1 = addItem(
        coreRemoteInterface, deviceId, "testItem1", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "15:20" },
                                                { "__START_DAY__", "5" },
                                                { "__START_MONTH__", "3" },
                                                { "__START_YEAR__", "2021" },
                                                { "__END_TIME__", "10:10" },
                                                { "__END_DAY__", "2" },
                                                { "__END_MONTH__", "9" },
                                                { "__END_YEAR__", "2022" } } );

    auto isItemStateBlock1 = replaceTokensInString(
        isItemStateWhenBlock, { { "__ITEM_ID__", itemId1 }, { "__TYPE__", "bool" }, { "__VALUE__", "true" } } );
    isItemStateBlock1 += ",";
    isItemStateBlock1.append( std::move( isDateRange ) );
    auto andOperator = replaceTokensInString( andOperatorWhenBlock, { { "__BLOCKS__", isItemStateBlock1 } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "isDateRangeAndOperator" }, { "__WHEN_BLOCKS__", andOperator }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-03-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    using namespace CoreD::Model::Resources;
    coreRemoteInterface.setItemValue(
        JSON( ItemFields::IdFieldName << itemId1 << ItemFields::ValueFieldName << true ) );

    checkBroadcasts(
        { [this, &sceneId, &request]( const Json& broadcast ) {
             return isRunProgressBroadcast( broadcast, request, sceneId, "started" );
         },
          [this, &sceneId, &request]( const Json& broadcast ) {
              return isRunProgressBroadcast( broadcast, request, sceneId, "finished" );
          },
          [this, &itemId1]( const Json& broadcast ) { return isItemUpdateBroadcast( broadcast, itemId1, true ); } },
        __LINE__ );

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2022-09-02T10:10:01+0000" ).toSeconds() );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    setItemValue( coreRemoteInterface, itemId1, false, __LINE__ );
    setItemValue( coreRemoteInterface, itemId1, true, __LINE__ );
}

TEST_F( SceneDateRangeTests, isDateRangeDefaultValues ) {  // 1.7
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2020-01-04T15:19:58+0000" ).toSeconds() );

    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "15:20" },
                                                { "__START_DAY__", "5" },
                                                { "__START_MONTH__", "3" },
                                                { "__START_YEAR__", "2021" },
                                                { "__END_TIME__", "10:10" },
                                                { "__END_DAY__", "2" },
                                                { "__END_MONTH__", "9" },
                                                { "__END_YEAR__", "2022" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );

    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 4 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 4 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 4 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 0 );

    auto sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T00:00:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2022-12-31T23:59:00+0000" ).toSeconds() );

    deleteScenes();

    request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );

    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 7 );
    request[ "params" ][ "when" ][ 0 ][ "fields" ].erase( 3 );

    sceneId = createScene( request );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2023-03-05T15:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2023-09-02T10:10:00+0000" ).toSeconds() );
}

TEST_F( SceneDateRangeTests, isDateRangeNotTriggerSceneOnCreation ) {  // 1.8
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-03-05T16:00:00+0000" ).toSeconds() );
    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "15:20" },
                                                { "__START_DAY__", "5" },
                                                { "__START_MONTH__", "3" },
                                                { "__START_YEAR__", "2021" },
                                                { "__END_TIME__", "10:10" },
                                                { "__END_DAY__", "2" },
                                                { "__END_MONTH__", "9" },
                                                { "__END_YEAR__", "2022" } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );

    auto sceneId = createScene( request );
    checkNoBroadcastReceived( "Scene should not be executed on creation", __LINE__, std::chrono::seconds( 2 ) );
}

TEST_F( SceneDateRangeTests, isDateRangeEventStatus ) {  // 1.9
    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T09:19:58+0000" ).toSeconds() );

    auto maxYear = std::to_string( std::numeric_limits< uint16_t >::max() );
    auto isDateRange = replaceTokensInString( isDateRangeWhenBlock,
                                              { { "__START_TIME__", "10:20" },
                                                { "__START_DAY__", "1" },
                                                { "__START_MONTH__", "1" },
                                                { "__START_YEAR__", "1" },
                                                { "__END_TIME__", "17:10" },
                                                { "__END_DAY__", "31" },
                                                { "__END_MONTH__", "12" },
                                                { "__END_YEAR__", maxYear } } );
    auto request = replaceTokens(
        createSceneRequest,
        { { "__NAME__", "IsDateRange" }, { "__WHEN_BLOCKS__", isDateRange }, { "__THEN_BLOCKS__", "" } } );
    SceneID sceneId = createScene( request );

    {
        auto answer = getSceneStatus( sceneId, __LINE__ );
        checkSceneStatus( answer, "false" );
        checkTimeSceneList( sceneId, "2021-01-01T10:20:00+0000" );
    }

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T10:20:00+0000" ).toSeconds() );
    testRunProgressBroadcast( request, sceneId, "started", __LINE__ );
    testRunProgressBroadcast( request, sceneId, "finished", __LINE__ );

    {
        auto answer = getSceneStatus( sceneId, __LINE__ );
        checkSceneStatus( answer, "true" );
        checkTimeSceneList( sceneId, "2021-01-01T17:10:00+0000" );
    }

    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2021-01-01T18:10:00+0000" ).toSeconds() );
    nma().sendBroadcast( HUB_SYSTEM_DATETIME_CHANGED, JSON_EMPTY_OBJ );
    auto broadcast = waitForBroadcast( "hub.system.datetime.changed", "hub time changed broadcast" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.system.datetime.changed" );
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        auto answer = getSceneStatus( sceneId, __LINE__ );
        checkSceneStatus( answer, "false" );
        checkTimeSceneList( sceneId, "2021-01-02T10:20:00+0000" );
    }

    deleteScene( sceneId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
