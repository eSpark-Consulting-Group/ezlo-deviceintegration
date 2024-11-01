#include "scenes_manager/scenes_engine/events/is_item_state_changed_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. integer type
DONE        1.1 "any" -> "any" change
DONE        1.2 "any" -> "value" change
DONE        1.3 "value" -> "any" change
DONE        1.4 "value" -> "value" change
DONE            1.4.1 in one step
DONE            1.4.2 with multiple steps
DONE    2. scalable types
DONE        2.1 values of same type
DONE        2.2 values of comparable types
DONE    3. string type
DONE    4. incomparable values

 */

class ItemStateChangedEventTests : public testing::Test {
public:
    using ItemValue = ItemValueWrapper;

    static ItemStateChangedEvent createItemStateChangedEvent( ItemValue startValue, ItemValue finishValue ) {
        ItemValueProviderPtr startValueProvider;
        ItemValueProviderPtr finishValueProvider;

        if( !startValue.empty() )
            startValueProvider = makeItemValueProvider( std::move( startValue ) );
        if( !finishValue.empty() )
            finishValueProvider = makeItemValueProvider( std::move( finishValue ) );

        return { std::move( startValueProvider ), std::move( finishValueProvider ) };
    }
};

TEST_F( ItemStateChangedEventTests, ints_anyToAny ) {  // 1.1
    ItemStateChangedEvent event = createItemStateChangedEvent( ItemValue{}, ItemValue{} );

    const ItemValue value1( R"({ "type": "int", "value": 10 })"_json );
    const ItemValue value2( R"({ "type": "int", "value": 20 })"_json );
    const ItemValue value3( R"({ "type": "int", "value": 30 })"_json );

    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
    EXPECT_TRUE( event.checkChangeExpected( value2, value3 ) );
}

TEST_F( ItemStateChangedEventTests, ints_anyToValue ) {  // 1.2
    const Json finishValuePayload = R"({ "type": "int", "value": 20 })"_json;
    ItemStateChangedEvent event = createItemStateChangedEvent( ItemValue{}, ItemValue( finishValuePayload ) );

    const ItemValue value1( R"({ "type": "int", "value": 10 })"_json );
    const ItemValue value2( finishValuePayload );

    EXPECT_FALSE( event.checkChangeExpected( value2, value1 ) );
    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
}

TEST_F( ItemStateChangedEventTests, ints_valueToAny ) {  // 1.3
    const Json startValuePayload = R"({ "type": "int", "value": 10 })"_json;
    ItemStateChangedEvent event = createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue{} );

    const ItemValue value1( startValuePayload );
    const ItemValue value2( R"({ "type": "int", "value": 20 })"_json );

    EXPECT_FALSE( event.checkChangeExpected( value2, value1 ) );
    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
}

TEST_F( ItemStateChangedEventTests, ints_valueToValueInOneStep ) {  // 1.4.1
    const Json startValuePayload = R"({ "type": "int", "value": 10 })"_json;
    const Json finishValuePayload = R"({ "type": "int", "value": 20 })"_json;
    ItemStateChangedEvent event =
        createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue( finishValuePayload ) );

    const ItemValue value1( startValuePayload );
    const ItemValue value2( finishValuePayload );

    EXPECT_FALSE( event.checkChangeExpected( value2, value1 ) );
    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
}

TEST_F( ItemStateChangedEventTests, ints_valueToValueWithMultipleSteps ) {  // 1.4.2
    const Json startValuePayload = R"({ "type": "int", "value": 10 })"_json;
    const Json finishValuePayload = R"({ "type": "int", "value": 20 })"_json;
    ItemStateChangedEvent event =
        createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue( finishValuePayload ) );

    const ItemValue value1( R"({ "type": "int", "value": 15 })"_json );
    const ItemValue value2( startValuePayload );
    const ItemValue value3( R"({ "type": "int", "value": 5 })"_json );
    const ItemValue value4( R"({ "type": "int", "value": 25 })"_json );
    const ItemValue value5( finishValuePayload );
    const ItemValue value6( R"({ "type": "int", "value": 20 })"_json );

    EXPECT_FALSE( event.checkChangeExpected( value1, value2 ) );
    EXPECT_FALSE( event.checkChangeExpected( value2, value3 ) );
    EXPECT_FALSE( event.checkChangeExpected( value3, value4 ) );
    EXPECT_TRUE( event.checkChangeExpected( value4, value5 ) );
    EXPECT_FALSE( event.checkChangeExpected( value5, value6 ) );
    EXPECT_FALSE( event.checkChangeExpected( value4, value5 ) );
}

TEST_F( ItemStateChangedEventTests, scalableTypes_valuesOfSameType ) {  // 2.1
    const Json startValuePayload = R"({ "type": "temperature", "value": { "value": 10.0, "scale": "celsius" } })"_json;
    const Json finishValuePayload = R"({ "type": "temperature", "value": { "value": 20.0, "scale": "celsius" } })"_json;
    ItemStateChangedEvent event =
        createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue( finishValuePayload ) );

    const ItemValue value1( startValuePayload );
    const ItemValue value2( finishValuePayload );

    EXPECT_FALSE( event.checkChangeExpected( value2, value1 ) );
    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
}

TEST_F( ItemStateChangedEventTests, scalableTypes_comparableValues ) {  // 2.2
    Json startValuePayload = R"({ "type": "temperature", "value": { "value": 0.0, "scale": "celsius" } })"_json;
    Json finishValuePayload = R"({ "type": "temperature", "value": { "value": 33.8, "scale": "fahrenheit" } })"_json;
    ItemStateChangedEvent event =
        createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue( finishValuePayload ) );

    const ItemValue value1( startValuePayload );
    const ItemValue value2( finishValuePayload );

    EXPECT_FALSE( event.checkChangeExpected( value2, value1 ) );
    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
}

TEST_F( ItemStateChangedEventTests, stringType ) {  // 3.
    const Json startValuePayload = R"({ "type": "string", "value": "hello_world" })"_json;
    const Json finishValuePayload = R"({ "type": "string", "value": "world_hello" })"_json;
    ItemStateChangedEvent event =
        createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue( finishValuePayload ) );

    const ItemValue value1( startValuePayload );
    const ItemValue value2( finishValuePayload );

    EXPECT_FALSE( event.checkChangeExpected( value2, value1 ) );
    EXPECT_TRUE( event.checkChangeExpected( value1, value2 ) );
}

TEST_F( ItemStateChangedEventTests, incomparableTypes ) {  // 4.
    const Json startValuePayload = R"({ "type": "int", "value": "hello_world" })"_json;
    const Json finishValuePayload = R"({ "type": "string", "value": "world_hello" })"_json;
    ItemStateChangedEvent event =
        createItemStateChangedEvent( ItemValue( startValuePayload ), ItemValue( finishValuePayload ) );

    const ItemValue value1( startValuePayload );
    const ItemValue value2( finishValuePayload );

    EXPECT_THROW( event.checkChangeExpected( value2, value1 ), SceneException );
}

}  // namespace UT
}  // namespace SceneEngine
