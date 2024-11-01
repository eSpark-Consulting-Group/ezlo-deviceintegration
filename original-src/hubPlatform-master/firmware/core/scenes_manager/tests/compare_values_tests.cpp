#include "test_utils/temporary_root_folder_wrapper.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/events/item_event_base_factory.h"
#include "scenes_manager/scenes_engine/events/compare_values_event.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

 TEST PLAN

 DONE   1. CompareValuesEvent
 DONE      1.1 invalid construction
 DONE      1.2 invalid values comparison
 DONE      1.3 valid comparison, equal
 DONE      1.4 valid comparison, less

 */

class CompareValuesEventTest : public ::testing::Test {
public:
    static ItemValueWrapper makePayload( std::string valueType, Json value );
};

ItemValueWrapper CompareValuesEventTest::makePayload( std::string valueType, Json value ) {
    return ItemValueWrapper( { { "type", valueType }, { "value", value } } );
}

TEST_F( CompareValuesEventTest, invalidConstruction ) {
    ItemEventBasePtr event;
    {
        EXPECT_THROW( event = ItemEventBaseFactory::makeCompareValues( makePayload( "int", 50 ), "==", "" ),
                      SceneEngine::SceneException );
    }
    {
        EXPECT_NO_THROW( event = ItemEventBaseFactory::makeCompareValues( makePayload( "", 50 ), "==", "int" ) )
            << "Throw on empty payload value type";
    }
    {
        EXPECT_THROW(
            event = ItemEventBaseFactory::makeCompareValues( makePayload( "", 50 ), "invalid_comparator", "int" ),
            SceneEngine::SceneException )
            << "invalid comparator passed";
    }
    {
        Json cameraStreamValue = { { "localRtspUrl", "https:127.0.0.1:1400/stream" } };
        event =
            ItemEventBaseFactory::makeCompareValues( makePayload( "CameraStream", cameraStreamValue ), "<=", "int" );
        EXPECT_THROW( event = ItemEventBaseFactory::makeCompareValues(
                          makePayload( "CameraStream", cameraStreamValue ), "<invalid_comparator>", "int" ),
                      SceneEngine::SceneException )
            << "No operator <()/isLess for CameraStream";
    }
}

TEST_F( CompareValuesEventTest, invalidValuesComparison ) {
    ItemValueWrapper rightValue = makePayload( "int", 50 );
    auto event = ItemEventBaseFactory::makeCompareValues( rightValue, "==", "int" );

    // left.type != right.type
    ItemValueWrapper leftValue = makePayload( "float", 100 );
    EXPECT_THROW( event->compare( leftValue ), SceneEngine::SceneException );
    // left.type != this->type
    EXPECT_THROW( event->compare( leftValue ), SceneEngine::SceneException );
}

TEST_F( CompareValuesEventTest, equal ) {
    auto event = ItemEventBaseFactory::makeCompareValues( makePayload( "int", 50 ), "==", "int" );
    EXPECT_TRUE( event->compare( makePayload( "int", 50 ) ) );
    EXPECT_FALSE( event->compare( makePayload( "int", 49 ) ) );
    EXPECT_FALSE( event->compare( makePayload( "int", 51 ) ) );

    EXPECT_THROW( event->compare( makePayload( "float", 100 ) ), SceneEngine::SceneException );
}

TEST_F( CompareValuesEventTest, less ) {
    auto event = ItemEventBaseFactory::makeCompareValues( makePayload( "int", 50 ), "<", "int" );
    EXPECT_TRUE( event->compare( makePayload( "int", 49 ) ) );
    EXPECT_FALSE( event->compare( makePayload( "int", 50 ) ) );
    EXPECT_FALSE( event->compare( makePayload( "int", 51 ) ) );
}

TEST_F( CompareValuesEventTest, withoutState ) {
    ItemValueWrapper rightValue = makePayload( "int", 50 );
    auto event = ItemEventBaseFactory::makeCompareValues( rightValue, "==", "int" );
    CompareValuesEvent* compareValuesEvent = (CompareValuesEvent*) event.get();
    compareValuesEvent->setKeepState( false );
    EXPECT_FALSE( compareValuesEvent->compare( makePayload( "int", 49 ) ) );
    EXPECT_TRUE( compareValuesEvent->compare( makePayload( "int", 50 ) ) );
    EXPECT_FALSE( compareValuesEvent->compare( makePayload( "int", 51 ) ) );
}

// TODO: expression variants

}  // namespace UT
}  // namespace SceneEngine
