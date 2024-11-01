#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include "scenes_manager/test_remote_interface/test_scenes_remote_interface.h"

#include "base_utils/gregorian_calendar.h"
#include "base_utils/timeutils.h"
#include "tiny/tiny_consolecolors.h"

#include <gtest/gtest.h>
#include <ctime>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

*/

class ScenesTimeListSuite : public SceneManagerTestsFixture {
public:
    static void SetUpTestCase();
    void SetUp() override;
    void TearDown() override;
};

void ScenesTimeListSuite::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );

    SceneManagerTestsFixture::TearDown();
}

void ScenesTimeListSuite::SetUpTestCase() {}

void ScenesTimeListSuite::SetUp() {
    SceneManagerTestsFixture::SetUp();
    callingFile( __FILE__ );
}

/* DONE: EFWR-3965: Enabled back */
TEST_F( ScenesTimeListSuite, getTimeScenesList ) {  // 1.22
    std::time_t current_time;
    std::time( &current_time );
    struct std::tm* timeinfo = std::localtime( &current_time );
    const std::string timeOffset = TimeUtils::timeStructToString( *timeinfo, "%z" );

    ScenesManager::TestRemoteInterface scenesManagerInterface( nma() );
    const GregorianCalendar::Date testDate( "2019-07-25T08:27:58" + timeOffset );
    const auto timeZoneOffset = std::chrono::duration_cast< std::chrono::hours >( testDate.getTimeZoneOffset() );
    scenesManagerInterface.setTestClock( testDate.toSeconds(), timeZoneOffset );
    setGeoCoordinates();

    SceneID onceSceneId = createScene( createIsOnceScene );
    SceneID dailySceneId = createScene( createDailyScene );
    SceneID onceSceneId1 = createScene( createIsOnceScene );
    SceneID dailySunsetSceneId = createScene( getSunStateRequest( "daily", "sunset", "intime" ) );
    SceneID intervalSceneId = createScene( createInterval1HourScene );

    auto timeScenesList = nma().sendtoHubAndGetAnswer( getTimeScenesList );
    JsonObject result = JSON_EMPTY_OBJ;
    JsonArray timeScenesArray = JSON_EMPTY_ARR;
    ASSERT_TRUE( json_get_obj( timeScenesList, "result", result ) );
    ASSERT_TRUE( json_get_arr( result, "timeScenes", timeScenesArray ) );

    // TODO: EFWR-3993: isInterval timezone offset
    const auto& intervalScene = timeScenesArray[ 0 ];
    EXPECT_EQ( json_get_str( intervalScene, "sceneId" ), intervalSceneId );
    EXPECT_EQ( json_get_str( intervalScene, "sceneName" ), "IntervalScene" );
    EXPECT_EQ( json_get_str( intervalScene, "methodName" ), "isInterval" );
    EXPECT_EQ( json_get_str( intervalScene, "executionDate" ), "2019-07-25T09:27:58" + timeOffset );

    const auto& dailyScene = timeScenesArray[ 1 ];
    EXPECT_EQ( json_get_str( dailyScene, "sceneId" ), dailySceneId );
    EXPECT_EQ( json_get_str( dailyScene, "sceneName" ), "DailyEvent" );
    EXPECT_EQ( json_get_str( dailyScene, "methodName" ), "isDate" );
    EXPECT_EQ( json_get_str( dailyScene, "executionDate" ), "2019-07-25T12:45:00" + timeOffset );

    const auto& dailySunsetScene = timeScenesArray[ 2 ];
    EXPECT_EQ( json_get_str( dailySunsetScene, "sceneId" ), dailySunsetSceneId );
    EXPECT_EQ( json_get_str( dailySunsetScene, "sceneName" ), "isSunState" );
    EXPECT_EQ( json_get_str( dailySunsetScene, "methodName" ), "isSunState" );
    const auto expectedSunsetDate =
        GregorianCalendar::Date( "2019-07-25T17:28:00" + timeOffset ).addHours( timeZoneOffset.count() );
    EXPECT_EQ( json_get_str( dailySunsetScene, "executionDate" ), expectedSunsetDate.toString() );

    const auto& onceScene = timeScenesArray[ 3 ];
    EXPECT_EQ( json_get_str( onceScene, "sceneId" ), onceSceneId );
    EXPECT_EQ( json_get_str( onceScene, "sceneName" ), "IsOnce" );
    EXPECT_EQ( json_get_str( onceScene, "methodName" ), "isOnce" );
    EXPECT_EQ( json_get_str( onceScene, "executionDate" ), "2019-12-05T10:20:00" + timeOffset );

    const auto& onceScene1 = timeScenesArray[ 4 ];
    EXPECT_EQ( json_get_str( onceScene1, "sceneId" ), onceSceneId1 );
    EXPECT_EQ( json_get_str( onceScene1, "sceneName" ), "IsOnce" );
    EXPECT_EQ( json_get_str( onceScene1, "methodName" ), "isOnce" );
    EXPECT_EQ( json_get_str( onceScene1, "executionDate" ), "2019-12-05T10:20:00" + timeOffset );

    deleteScenes();

    timeScenesList = nma().sendtoHubAndGetAnswer( getTimeScenesList );
    ASSERT_TRUE( json_get( timeScenesList, { "result", "timeScenes" }, timeScenesArray ) );
    ASSERT_TRUE( timeScenesArray.empty() );

    onceSceneId = createScene( createIsOnceScene );
    scenesManagerInterface.setTestClock( GregorianCalendar::Date( "2019-12-05T10:19:58" + timeOffset ).toSeconds(),
                                         timeZoneOffset );

    testRunProgressBroadcast( createIsOnceScene, onceSceneId, "started", __LINE__ );
    testRunProgressBroadcast( createIsOnceScene, onceSceneId, "finished", __LINE__ );

    timeScenesList = nma().sendtoHubAndGetAnswer( getTimeScenesList );
    ASSERT_TRUE( json_get( timeScenesList, { "result", "timeScenes" }, timeScenesArray ) );
    ASSERT_EQ( timeScenesArray.size(), 1 );

    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "sceneId" ), onceSceneId );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "sceneName" ), "IsOnce" );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "methodName" ), "isOnce" );
    EXPECT_EQ( json_get_str( timeScenesArray[ 0 ], "executionDate" ), "2019-12-05T10:20:00" + timeOffset );
}

}  // namespace Tests
}  // namespace HubPlatform