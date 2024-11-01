#include "scenes_manager/scenes_engine/events/item_value_wrapper.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"

#include "bus/base_bus.h"
#include "json_utils/json_utils.h"
#include "test_utils/temporary_root_folder_wrapper.h"
#include <gtest/gtest.h>

namespace SceneEngine {

void PrintTo( const ItemValueWrapper& itemValue, std::ostream* os ) {
    *os << itemValue.getPayload().dump();
}

std::ostream& operator<<( std::ostream& os, const ItemValueWrapper& itemValue ) {
    PrintTo( itemValue, &os );
    return os;
}

namespace UT {

using ::testing::Test;

class ItemValueWrapperTests : public Test {};

TEST_F( ItemValueWrapperTests, compareWrappers ) {
    Json itemInt1 = R"({"value": 10, "type": "int"})"_json;
    ItemValueWrapper itemValue1( itemInt1 );
    ItemValueWrapper itemValue2( itemInt1 );

    EXPECT_EQ( itemValue1, itemValue2 );

    Json itemFloat = R"({"value": 10.1, "type": "float"})"_json;
    itemValue1.setPayload( itemFloat );
    itemValue2.setPayload( itemFloat );

    EXPECT_EQ( itemValue1, itemValue2 );
    EXPECT_TRUE( itemValue1 == itemValue2 );
    EXPECT_FALSE( itemValue1 != itemValue2 );
    EXPECT_TRUE( itemValue1 <= itemValue2 );
    EXPECT_TRUE( itemValue1 >= itemValue2 );
    EXPECT_FALSE( itemValue1 < itemValue2 );
    EXPECT_FALSE( itemValue1 > itemValue2 );

    // 17 C == 62.6 F
    Json itemFahrenheit = R"({"type": "temperature", "value": { "value": 62.6, "scale": "fahrenheit"} })"_json;
    itemValue1.setPayload( itemFahrenheit );
    itemValue2.setPayload( itemFahrenheit );
    EXPECT_TRUE( itemValue1 == itemValue2 );
    EXPECT_FALSE( itemValue1 != itemValue2 );
    EXPECT_TRUE( itemValue1 <= itemValue2 );
    EXPECT_TRUE( itemValue1 >= itemValue2 );
    EXPECT_FALSE( itemValue1 < itemValue2 );
    EXPECT_FALSE( itemValue1 > itemValue2 );

    Json itemCelsius = R"({"type": "temperature", "value": { "value": 17.00, "scale": "celsius"} })"_json;
    itemValue1.setPayload( itemCelsius );
    itemValue2.setPayload( itemCelsius );
    EXPECT_TRUE( itemValue1 == itemValue2 );
    EXPECT_FALSE( itemValue1 != itemValue2 );
    EXPECT_TRUE( itemValue1 <= itemValue2 );
    EXPECT_TRUE( itemValue1 >= itemValue2 );
    EXPECT_FALSE( itemValue1 < itemValue2 );
    EXPECT_FALSE( itemValue1 > itemValue2 );
}

TEST_F( ItemValueWrapperTests, compareEqualDiffScales ) {
    // 17 C == 62.6 F
    LOGGER_DEBUG( "Compare 17 C == 62.6 F, should be equal" );
    Json itemFahrenheit = R"({"type": "temperature", "value": {"value": 62.6, "scale": "fahrenheit"}})"_json;
    Json itemCelsius = R"({"type": "temperature", "value": {"value": 17.00, "scale": "celsius"}})"_json;

    ItemValueWrapper itemValue1( itemFahrenheit );
    ItemValueWrapper itemValue2( itemCelsius );

    EXPECT_TRUE( itemValue1 == itemValue2 );
    EXPECT_FALSE( itemValue1 != itemValue2 );
    EXPECT_TRUE( itemValue1 <= itemValue2 );
    EXPECT_TRUE( itemValue1 >= itemValue2 );
    EXPECT_FALSE( itemValue1 < itemValue2 );
    EXPECT_FALSE( itemValue1 > itemValue2 );

    EXPECT_TRUE( itemValue2 == itemValue1 );
    EXPECT_FALSE( itemValue2 != itemValue1 );
    EXPECT_TRUE( itemValue2 <= itemValue1 );
    EXPECT_TRUE( itemValue2 >= itemValue1 );
    EXPECT_FALSE( itemValue2 < itemValue1 );
    EXPECT_FALSE( itemValue2 > itemValue1 );
}

TEST_F( ItemValueWrapperTests, compareDifferentWithDiffScales ) {
    // 17 C == 62.6 F, 80 F = 26.66 C
    LOGGER_DEBUG( "Compare 17 C (62.6 F) and 80 F (26.66 C), should be less" );
    Json itemFahrenheit = R"({"type": "temperature", "value": {"value": 80, "scale": "fahrenheit"}})"_json;
    Json itemCelsius = R"({"type": "temperature", "value": {"value": 17.00, "scale": "celsius"}})"_json;

    ItemValueWrapper itemValue26_5C( itemFahrenheit );
    ItemValueWrapper itemValue17C( itemCelsius );

    EXPECT_FALSE( itemValue26_5C == itemValue17C );
    EXPECT_TRUE( itemValue26_5C != itemValue17C );
    EXPECT_FALSE( itemValue26_5C <= itemValue17C );
    EXPECT_TRUE( itemValue26_5C >= itemValue17C );
    EXPECT_FALSE( itemValue26_5C < itemValue17C );
    EXPECT_TRUE( itemValue26_5C > itemValue17C );
}

TEST_F( ItemValueWrapperTests, compareDifferentWithSameScales ) {
    // 17 C == 62.6 F, 80 F = 26.66 C
    LOGGER_DEBUG( "Compare 17 C (62.6 F) and 80 F (26.66 C), should be less" );
    Json item26_6C = R"({"type": "temperature", "value": {"value": 26.6, "scale": "celsius"}})"_json;
    Json item17C = R"({"type": "temperature", "value": {"value": 17.00, "scale": "celsius"}})"_json;

    ItemValueWrapper itemValue26_5C( item26_6C );
    ItemValueWrapper itemValue17C( item17C );

    EXPECT_FALSE( itemValue26_5C == itemValue17C );
    EXPECT_TRUE( itemValue26_5C != itemValue17C );
    EXPECT_FALSE( itemValue26_5C <= itemValue17C );
    EXPECT_TRUE( itemValue26_5C >= itemValue17C );
    EXPECT_FALSE( itemValue26_5C < itemValue17C );
    EXPECT_TRUE( itemValue26_5C > itemValue17C );
}

TEST_F( ItemValueWrapperTests, compareComparableNumericValues ) {
    Json tempValue26 = R"({"type": "temperature", "value": {"value": 26, "scale": "celsius"}})"_json;
    Json intValue17 = R"({"type": "int", "value": 17})"_json;

    ItemValueWrapper value26( tempValue26 );
    ItemValueWrapper value17( intValue17 );

    EXPECT_FALSE( value26 == value17 );
    EXPECT_TRUE( value26 != value17 );
    EXPECT_FALSE( value26 <= value17 );
    EXPECT_TRUE( value26 >= value17 );
    EXPECT_FALSE( value26 < value17 );
    EXPECT_TRUE( value26 > value17 );
}

TEST_F( ItemValueWrapperTests, compareComparableStringValues ) {
    Json stringValue = R"({"type": "string", "value": "text1"})"_json;
    Json tokenValue = R"({"type": "token", "value": "text2"})"_json;

    ItemValueWrapper value1( stringValue );
    ItemValueWrapper value2( tokenValue );

    EXPECT_FALSE( value2 == value1 );
    EXPECT_TRUE( value2 != value1 );
    EXPECT_FALSE( value2 <= value1 );
    EXPECT_TRUE( value2 >= value1 );
    EXPECT_FALSE( value2 < value1 );
    EXPECT_TRUE( value2 > value1 );
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormats ) {
    // 0 C == 32 F
    Json item0C = R"({"value": 0, "scale": "celsius"})"_json;  // 0C == 32F
    Json item33F = R"({"value": 33, "scale": "fahrenheit"})"_json;
    Json item32F = R"({"value": 32, "scale": "fahrenheit"})"_json;

    ItemValueWrapper itemValue0C( item0C );
    ItemValueWrapper itemValue33F( item33F );

    EXPECT_TRUE( itemValue0C < itemValue33F );
    EXPECT_TRUE( itemValue0C <= itemValue33F );
    EXPECT_FALSE( itemValue0C > itemValue33F );
    EXPECT_FALSE( itemValue0C >= itemValue33F );
    EXPECT_FALSE( itemValue0C == itemValue33F );
    EXPECT_TRUE( itemValue0C != itemValue33F );

    auto nestedValue = JSON( "value" << item33F );
    ItemValueWrapper nestedWrapper = { nestedValue };
    EXPECT_FALSE( itemValue0C == nestedWrapper ) << "expression to item nested value comparison, ==";
    EXPECT_TRUE( itemValue0C != nestedWrapper ) << "expression to item nested value comparison, !=";
    EXPECT_TRUE( itemValue0C < nestedWrapper ) << "expression to item nested value comparison, <";
    EXPECT_FALSE( itemValue0C > nestedWrapper ) << "expression to item nested value comparison, >";
    EXPECT_TRUE( itemValue0C <= nestedWrapper ) << "expression to item nested value comparison, <=";
    EXPECT_FALSE( itemValue0C >= nestedWrapper ) << "expression to item nested value comparison, >=";

    nestedValue = JSON( "value" << item32F );
    nestedWrapper = { nestedValue };
    EXPECT_TRUE( itemValue0C == nestedWrapper ) << "expression to item nested value comparison, ==";
    EXPECT_FALSE( itemValue0C != nestedWrapper ) << "expression to item nested value comparison, !=";
    EXPECT_FALSE( itemValue0C < nestedWrapper ) << "expression to item nested value comparison, <";
    EXPECT_FALSE( itemValue0C > nestedWrapper ) << "expression to item nested value comparison, >";
    EXPECT_TRUE( itemValue0C <= nestedWrapper ) << "expression to item nested value comparison, <=";
    EXPECT_TRUE( itemValue0C >= nestedWrapper ) << "expression to item nested value comparison, >=";
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormatsReverseOrderEqualValues ) {
    // 0 C == 32 F
    Json item0C = R"({"value": 0, "scale": "celsius"})"_json;  // 0C == 32F
    Json item32F = R"({"value": 32, "scale": "fahrenheit"})"_json;

    ItemValueWrapper itemValue0C( item0C );

    auto nestedValue = JSON( "value" << item32F );
    ItemValueWrapper nestedWrapper = { nestedValue };

    // change operands order
    EXPECT_TRUE( nestedWrapper == itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, ==";
    EXPECT_FALSE( nestedWrapper != itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, !=";
    EXPECT_FALSE( nestedWrapper < itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, <";
    EXPECT_FALSE( nestedWrapper > itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, >";
    EXPECT_TRUE( nestedWrapper <= itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, <=";
    EXPECT_TRUE( nestedWrapper >= itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, >=";
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormatsReverseOrderDifferentValues ) {
    // 0 C == 32 F
    Json item0C = R"({"value": 0, "scale": "celsius"})"_json;  // 0C == 32F

    ItemValueWrapper itemValue0C( item0C );

    // 33F > (0C == 32F)
    auto nestedValue = Json{ { "value", 33 }, { "scale", "fahrenheit" } };
    ItemValueWrapper nestedWrapper( JSON( "value" << nestedValue ) );

    // change operands order
    EXPECT_FALSE( nestedWrapper == itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, ==";
    EXPECT_TRUE( nestedWrapper != itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, !=";
    EXPECT_FALSE( nestedWrapper < itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, <";
    EXPECT_TRUE( nestedWrapper > itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, >";
    EXPECT_FALSE( nestedWrapper <= itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, <=";
    EXPECT_TRUE( nestedWrapper >= itemValue0C )
        << "back order: nested op itemValue; expression to item nested value comparison, >=";
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormatsExactLess_equal_loudnessBlockHasScale ) {
    ItemValueWrapper itemValue(
        { { "value", { { "value", 50 }, { "scale", "decibel" } } }, { "valueType", "loudness" } } );
    ItemValueWrapper whenBlockValuePlain( Json{ { "value", 50 }, { "scale", "decibel" } } );

    EXPECT_TRUE( itemValue == whenBlockValuePlain ) << itemValue << " == " << whenBlockValuePlain;
    EXPECT_FALSE( itemValue != whenBlockValuePlain ) << itemValue << " != " << whenBlockValuePlain;
    EXPECT_FALSE( itemValue < whenBlockValuePlain ) << itemValue << " < " << whenBlockValuePlain;
    EXPECT_TRUE( itemValue <= whenBlockValuePlain ) << itemValue << " <= " << whenBlockValuePlain;
    EXPECT_FALSE( itemValue > whenBlockValuePlain ) << itemValue << " > " << whenBlockValuePlain;
    EXPECT_TRUE( itemValue >= whenBlockValuePlain ) << itemValue << " >= " << whenBlockValuePlain;

    // reverse
    LOGGER_TRACE_RAW( "Reverse order:" );
    EXPECT_TRUE( whenBlockValuePlain == itemValue ) << whenBlockValuePlain << " == " << itemValue;
    EXPECT_FALSE( whenBlockValuePlain != itemValue ) << whenBlockValuePlain << " != " << itemValue;
    EXPECT_FALSE( whenBlockValuePlain < itemValue ) << whenBlockValuePlain << " < " << itemValue;
    EXPECT_TRUE( whenBlockValuePlain <= itemValue ) << whenBlockValuePlain << " <= " << itemValue;
    EXPECT_FALSE( whenBlockValuePlain > itemValue ) << whenBlockValuePlain << " > " << itemValue;
    EXPECT_TRUE( whenBlockValuePlain >= itemValue ) << whenBlockValuePlain << " >= " << itemValue;
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormatsExactLess_less_loudnessBlockHasScale ) {
    ItemValueWrapper itemValue(
        { { "value", { { "value", 0 }, { "scale", "decibel" } } }, { "valueType", "loudness" } } );
    ItemValueWrapper whenBlockValuePlain( Json{ { "value", 50 }, { "scale", "decibel" } } );

    EXPECT_FALSE( itemValue == whenBlockValuePlain ) << itemValue << " == " << whenBlockValuePlain;
    EXPECT_TRUE( itemValue != whenBlockValuePlain ) << itemValue << " != " << whenBlockValuePlain;
    EXPECT_TRUE( itemValue < whenBlockValuePlain ) << itemValue << " < " << whenBlockValuePlain;
    EXPECT_TRUE( itemValue <= whenBlockValuePlain ) << itemValue << " <= " << whenBlockValuePlain;
    EXPECT_FALSE( itemValue > whenBlockValuePlain ) << itemValue << " > " << whenBlockValuePlain;
    EXPECT_FALSE( itemValue >= whenBlockValuePlain ) << itemValue << " >= " << whenBlockValuePlain;
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormatsExactLess_loudnessBlockNoScale ) {
    ItemValueWrapper itemValue( { { "value", { { "value", 0 }, { "scale", "decibel" } } },
                                  { "valueType", "loudness" },
                                  { "deviceId", "none" },
                                  { "name", "someItemName" } } );
    ItemValueWrapper whenBlockNoScale( Json{ { "value", 50 } } );

    EXPECT_TRUE( itemValue != whenBlockNoScale ) << itemValue << " != " << whenBlockNoScale;
    EXPECT_TRUE( itemValue < whenBlockNoScale ) << itemValue << " < " << whenBlockNoScale;
    EXPECT_TRUE( itemValue <= whenBlockNoScale ) << itemValue << " <= " << whenBlockNoScale;

    // They are false because of bad schema, not because of comparison result
    EXPECT_FALSE( itemValue == whenBlockNoScale ) << itemValue << " == " << whenBlockNoScale;
    EXPECT_FALSE( itemValue > whenBlockNoScale ) << itemValue << " > " << whenBlockNoScale;
    EXPECT_FALSE( itemValue >= whenBlockNoScale ) << itemValue << " >= " << whenBlockNoScale;
}

TEST_F( ItemValueWrapperTests, compareDeprecatedFormatsExactEqual_loudnessBlockNoScale ) {
    ItemValueWrapper itemValue( { { "value", { { "value", 50 }, { "scale", "decibel" } } },
                                  { "valueType", "loudness" },
                                  { "deviceId", "none" },
                                  { "name", "someItemName" } } );
    ItemValueWrapper whenBlockNoScale( Json{ { "value", 50 } } );

    EXPECT_TRUE( itemValue == whenBlockNoScale ) << itemValue << " == " << whenBlockNoScale;
    EXPECT_FALSE( itemValue != whenBlockNoScale ) << itemValue << " != " << whenBlockNoScale;
    EXPECT_FALSE( itemValue < whenBlockNoScale ) << itemValue << " < " << whenBlockNoScale;
    EXPECT_TRUE( itemValue <= whenBlockNoScale ) << itemValue << " <= " << whenBlockNoScale;
    EXPECT_FALSE( itemValue > whenBlockNoScale ) << itemValue << " > " << whenBlockNoScale;
    EXPECT_TRUE( itemValue >= whenBlockNoScale ) << itemValue << " >= " << whenBlockNoScale;
}

TEST_F( ItemValueWrapperTests, setPayload ) {
    const char* Scale = "decibel";
    Json plainValue = Json{ { "value", 2 }, { "scale", Scale } };
    ItemValueWrapper itemValue;
    itemValue.setPayload( Json{ { "value", plainValue } } );

    EXPECT_EQ( itemValue.getScale(), Scale ) << "Scale property should be updated";
}

TEST_F( ItemValueWrapperTests, PayloadFormats ) {
    {
        // Default CoreD item format: { "type":"<type>", "value": <value> }
        auto payload = JSON( "type"
                             << "temperature"
                             << "value"
                             << JSON( "value" << 3.0 << "scale"
                                              << "celsius" ) );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), payload.dump() );
    }
    {
        // Deprecated usage of "float" instead of scalable type: { "type":"float", "value": <scalable-value> }
        auto payload = JSON( "type"
                             << "float"
                             << "value"
                             << JSON( "value" << 3.0 << "scale"
                                              << "celsius" ) );
        auto expected = JSON( "type"
                              << "temperature"
                              << "value"
                              << JSON( "value" << 3.0 << "scale"
                                               << "celsius" ) );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), expected.dump() );
    }
    {
        // Deprecated broadcast format: { "valueType":"<type>", "value": <value> }
        auto payload = JSON( "valueType"
                             << "temperature"
                             << "value"
                             << JSON( "value" << 3.0 << "scale"
                                              << "celsius" ) );
        auto expected = JSON( "type"
                              << "temperature"
                              << "value"
                              << JSON( "value" << 3.0 << "scale"
                                               << "celsius" ) );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), expected.dump() );
    }
    {
        // Deprecated scalable format: { "type":"<type>", "value": <value>, "scale": "<scale>" }
        auto payload = JSON( "value"
                             << "temperature"
                             << "value" << 3.0 << "scale"
                             << "celsius" );
        auto expected = JSON( "type"
                              << "temperature"
                              << "value"
                              << JSON( "value" << 3.0 << "scale"
                                               << "celsius" ) );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), expected.dump() );
    }
    {
        // Deprecated format: { "value": { "value": <value>, "scale": "<scale>"} }
        auto payload = JSON( "value" << JSON( "value" << 3.0 << "scale"
                                                      << "celsius" ) );
        auto expected = JSON( "type"
                              << "temperature"
                              << "value"
                              << JSON( "value" << 3.0 << "scale"
                                               << "celsius" ) );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), expected.dump() );
    }
    {
        // Deprecated format: { "value": 3.0 }
        auto payload = JSON( "value" << 3.0 );
        auto expected = JSON( "type"
                              << "float"
                              << "value" << 3.0 );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), expected.dump() );
    }
    {
        // Deprecated format: Json( <value> )
        auto payload = Json( 3.0 );
        auto expected = JSON( "type"
                              << "float"
                              << "value" << 3.0 );
        ItemValueWrapper wrapper( payload );
        EXPECT_EQ( wrapper.getPayload().dump(), expected.dump() );
    }
}

TEST_F( ItemValueWrapperTests, isValid ) {
    EXPECT_FALSE( ItemValueWrapper().isValid() );

    Json validPayload = R"({"type": "int", "value": 10})"_json;
    EXPECT_TRUE( ItemValueWrapper( validPayload ).isValid() );

    Json invalidPayload = R"({"type": "int", "value": "text"})"_json;
    EXPECT_FALSE( ItemValueWrapper( invalidPayload ).isValid() ) << "string value vs int type";

    Json validTemperature = R"({"type": "temperature", "value": { "value": 62.6, "scale": "fahrenheit"} })"_json;
    EXPECT_TRUE( ItemValueWrapper( validTemperature ).isValid() );

    Json invalidTemperatureScale = R"({"type": "temperature", "value": { "value": 62.6, "scale": "seconds"} })"_json;
    EXPECT_FALSE( ItemValueWrapper( invalidTemperatureScale ).isValid() );

    Json invalidTemperatureValue =
        R"({"type": "temperature", "value": { "value": "text", "scale": "fahrenheit"} })"_json;
    EXPECT_FALSE( ItemValueWrapper( invalidTemperatureValue ).isValid() );

    Json invalidArrayWithDifferentTypes = R"({"type": "array.int", "value": ["1", 2]})"_json;
    EXPECT_FALSE( ItemValueWrapper( std::move( invalidArrayWithDifferentTypes ) ).isValid() );
}

TEST_F( ItemValueWrapperTests, isArray ) {
    EXPECT_FALSE( ItemValueWrapper().isArray() );

    Json invalidNonArrayPayload = R"({"type": "int", "value": 10})"_json;
    EXPECT_FALSE( ItemValueWrapper( std::move( invalidNonArrayPayload ) ).isArray() );

    Json validStringsArrayPayload = R"({"type": "array.string", "value": ["1", "2", "3"]})"_json;
    EXPECT_TRUE( ItemValueWrapper( std::move( validStringsArrayPayload ) ).isArray() );

    Json validIntsArrayPayload = R"({"type": "array.int", "value": [1, 2, 3]})"_json;
    EXPECT_TRUE( ItemValueWrapper( std::move( validIntsArrayPayload ) ).isArray() );
}

TEST_F( ItemValueWrapperTests, arrayValuesParsedCorrectly ) {
    EXPECT_THROW( ItemValueWrapper().parseArrayValues(), EventException );

    {
        Json stringsArrayPayload = R"({"type": "array.string", "value": ["1", "2", "3"]})"_json;
        const ItemValueWrapper array( std::move( stringsArrayPayload ) );

        std::vector< ItemValueWrapper > arrayValues = array.parseArrayValues();
        EXPECT_EQ( arrayValues.size(), 3 );
        for( const ItemValueWrapper& valueWrapper : arrayValues )
            EXPECT_EQ( valueWrapper.getValueType(), "string" );
    }
    {
        Json intsArrayPayload = R"({"type": "array.int", "value": [1, 2, 3, 4, 5]})"_json;
        const ItemValueWrapper array( std::move( intsArrayPayload ) );

        std::vector< ItemValueWrapper > arrayValues = array.parseArrayValues();
        EXPECT_EQ( arrayValues.size(), 5 );
        for( const ItemValueWrapper& valueWrapper : arrayValues )
            EXPECT_EQ( valueWrapper.getValueType(), "int" );
    }
}

}  // namespace UT
}  // namespace SceneEngine
