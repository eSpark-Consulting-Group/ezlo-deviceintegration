#include "scenes_manager/scenes_engine/events/compare_strings_event.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"
#include "scenes_manager/scenes_engine/events/compare_values_event.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

class CompareValueTypesTest : public ::testing::Test {};

TEST_F( CompareValueTypesTest, compareWithMismatchTypesInStringEvent ) {
    ItemValueWrapper itemValue1( { { "type", "string" }, { "value", "1" } } );
    ItemValueWrapper itemValue2( { { "type", "int" }, { "value", 1 } } );

    CompareStringsEvent event( itemValue1, std::less_equal< CompareStringsEvent::ItemValue >() );
    EXPECT_THROW( event.compare( itemValue2 ), SceneEngine::SceneException );
    itemValue2 = ItemValueWrapper( { { "type", "string" }, { "value", "1" } } );
    EXPECT_TRUE( event.compare( itemValue2 ) );
}

TEST_F( CompareValueTypesTest, compareWithMismatchTypesInNumberEvent ) {
    ItemValueWrapper itemValue1( { { "type", "int" }, { "value", 1 } } );
    ItemValueWrapper itemValue2( { { "type", "string" }, { "value", "1" } } );

    CompareNumbersEvent event( itemValue1, std::less_equal< CompareStringsEvent::ItemValue >() );
    EXPECT_THROW( event.compare( itemValue2 ), SceneEngine::SceneException );
    itemValue2 = ItemValueWrapper( { { "type", "int" }, { "value", 1 } } );
    EXPECT_TRUE( event.compare( itemValue2 ) );
}

TEST_F( CompareValueTypesTest, compareWithMismatchTypesInValuesEvent ) {
    ItemValueWrapper itemValue1;
    itemValue1.setValueType( "int" );
    ItemValueWrapper itemValue2;
    itemValue2.setValueType( "string" );
    CompareValuesEvent event( itemValue1, "==", "int" );
    EXPECT_THROW( event.compare( itemValue2 ), SceneEngine::SceneException );
    itemValue2.setValueType( "int" );
    itemValue2.setValueType( "int" );
    EXPECT_TRUE( event.compare( itemValue2 ) );
}

TEST_F( CompareValueTypesTest, compareWithMismatchTypesInItemEvent ) {
    ItemValueWrapper itemValue1( { { "type", "int" }, { "value", 1 } } );
    ItemValueWrapper itemValue2( { { "type", "string" }, { "value", "1" } } );
    itemValue2.setValueType( "string" );
    ItemEvent event( "item1", makeItemValueProvider( std::move( itemValue1 ) ) );
    EXPECT_THROW( event.compare( "itemId1", itemValue2, ItemEvent::ArmedState::FALSE ), SceneEngine::SceneException );
    itemValue2 = ItemValueWrapper( { { "type", "int" }, { "value", 1 } } );
    EXPECT_TRUE( event.compare( "item1", itemValue2, ItemEvent::ArmedState::FALSE ) );
}

}  // namespace UT
}  // namespace SceneEngine
