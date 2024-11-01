#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_engine/scene.h"
#include "scenes_manager/scenes_engine/events/interval_event.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_value_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/actions/scene_action_execute.h"
#include "scenes_manager/scenes_engine/actions/dispatcher.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"

#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/task_loop_test_helper.h"
#include "bus/base_bus.h"
#include "bus/fake_bus.h"
#include "database_client/fake_db/fake_db_client.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include "test_utils/temporary_root_folder_wrapper.h"
#include "scenes_manager/scenes_service.h"
#include "scenes_manager/scenes_engine/environment.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace SceneEngine {
namespace UT {

TEST( IntervalEvent, generalUsage ) {
    // Right cases
    EXPECT_NO_THROW( {
        JsonObject secs( "2s" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( secs );
    } );
    EXPECT_NO_THROW( {
        JsonObject mins( "15m" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( mins );
    } );
    EXPECT_NO_THROW( {
        JsonObject mins( "99999999m" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( mins );
    } );
    EXPECT_NO_THROW( {
        JsonObject hours( "12h" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( hours );
    } );
    EXPECT_NO_THROW( {
        JsonObject hours( "99999999999999h" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( hours );
    } );
    EXPECT_NO_THROW( {
        JsonObject days( "2d" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( days );
    } );
    EXPECT_NO_THROW( {
        JsonObject days( "9999999999999999d" );
        auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( days );
    } );

    // Wrong simple cases
    EXPECT_THROW(
        {
            JsonObject wrongValue( "12i" );
            auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( wrongValue );
        },
        SceneEngine::SceneException );

    EXPECT_THROW(
        {
            JsonObject wrongValue( "12.0f" );
            auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( wrongValue );
        },
        SceneEngine::SceneException );

    EXPECT_THROW(
        {
            JsonObject wrongValue( 123 );
            auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( wrongValue );
        },
        SceneEngine::SceneException );

    EXPECT_THROW(
        {
            JsonObject wrongValue( "strValue" );
            auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( wrongValue );
        },
        std::exception );

    EXPECT_THROW(
        {
            JsonObject hours( "0h" );
            auto intEvent = std::make_shared< SceneEngine::IntervalEvent >( hours );
        },
        SceneEngine::SceneException );
}

TEST( CompareNumbersEvent, equal ) {
    CompareNumbersEvent event( 50, std::equal_to< CompareNumbersEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_TRUE( event.compare( { 50 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
}

TEST( CompareNumbersEvent, notEqual ) {
    CompareNumbersEvent event( 50, std::not_equal_to< CompareNumbersEvent::ItemValue >() );
    EXPECT_TRUE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 50 } ) );
    EXPECT_TRUE( event.compare( { 51 } ) );
}

TEST( CompareNumbersEvent, greater ) {
    CompareNumbersEvent event( 50, std::greater< CompareNumbersEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { 48 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 50 } ) );
    EXPECT_TRUE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 52 } ) );
    EXPECT_FALSE( event.compare( { 53 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 50 } ) );
    EXPECT_TRUE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 52 } ) );
}

TEST( CompareNumbersEvent, greaterEqual ) {
    CompareNumbersEvent event( 50, std::greater_equal< CompareNumbersEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { 48 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_TRUE( event.compare( { 50 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 52 } ) );
    EXPECT_FALSE( event.compare( { 53 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_TRUE( event.compare( { 50 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 52 } ) );
}

TEST( CompareNumbersEvent, less ) {
    CompareNumbersEvent event( 50, std::less< CompareNumbersEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { 52 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 50 } ) );
    EXPECT_TRUE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 48 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_FALSE( event.compare( { 50 } ) );
    EXPECT_TRUE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 48 } ) );
}

TEST( CompareNumbersEvent, lessEqual ) {
    CompareNumbersEvent event( 50, std::less_equal< CompareNumbersEvent::ItemValue >() );
    EXPECT_FALSE( event.compare( { 52 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_TRUE( event.compare( { 50 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 48 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 51 } ) );
    EXPECT_TRUE( event.compare( { 50 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 48 } ) );
}

TEST( CompareNumbersEvent, withoutState ) {
    CompareNumbersEvent event( 50, std::greater< CompareNumbersEvent::ItemValue >() );
    event.setKeepState( false );
    EXPECT_FALSE( event.compare( { 48 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_FALSE( event.compare( { 50 } ) );
    EXPECT_TRUE( event.compare( { 51 } ) );
    EXPECT_TRUE( event.compare( { 52 } ) );
    EXPECT_FALSE( event.compare( { 48 } ) );
    EXPECT_FALSE( event.compare( { 49 } ) );
    EXPECT_TRUE( event.compare( { 51 } ) );
    EXPECT_TRUE( event.compare( { 52 } ) );
}

TEST( DictionaryValueEvent, compare ) {
    JsonObject element = "low_battery";
    DictionaryValueEvent event( element );
    EXPECT_TRUE( event.compare( "added", element ) );
    EXPECT_TRUE( event.compare( "updated", element ) );
}

}  // namespace UT
}  // namespace SceneEngine
