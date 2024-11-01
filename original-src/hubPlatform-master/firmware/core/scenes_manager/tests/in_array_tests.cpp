#include "scenes_manager/scenes_engine/events/in_array_event.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

namespace ArrayOperations = Model::Resources::ArrayOperations;

/*

TEST PLAN

DONE    1. "in" operation
DONE        1.1 for ints
DONE        1.2 for strings
DONE        1.3 for scalable values same values
DONE        1.4 for incomparable values
DONE        1.5 for not same, but comparable numeric values
DONE        1.6 for not same, but comparable scalable values
TODO        1.7 for array with different scalable values DISABLED due to issues with comparison of floats
DONE    2. "not_in" operation
DONE        2.1 for ints
DONE        2.2 for strings
DONE        2.3 for scalable values
DONE        2.4 for incomparable values
DONE        2.5 for not same, but comparable numeric values
DONE        2.6 for not same, but comparable scalable values
TODO        2.7 for array with different scalable values DISABLED due to issues with comparison of floats
DONE    3. event is not created with incorrect operation
DONE    4. event is not created with array of incorrect size
DONE    5. event is created with empty "operation"
DONE    6. Stateless Operation
DONE       6.1 Stateless "in" operation for ints
DONE       6.2 Stateless "in" operation for strings
DONE       6.3 Stateless "not_in" operation for ints
DONE       6.4 Stateless "not_in" operation for strings

 */

class InArrayEventTests : public testing::Test {
public:
    InArrayEvent createInArrayEvent( Json valuePayload, const std::string& operation ) {
        ItemValueWrapper value( std::move( valuePayload ) );
        return InArrayEvent( value.parseArrayValues(), operation );
    }

    void checkExceptionOnEventCreation( Json valuePayload,
                                        const std::string& operation,
                                        const std::string& expectedMessage ) {
        bool exceptionCaught = false;
        try {
            InArrayEvent event = createInArrayEvent( std::move( valuePayload ), operation );
        }
        catch( const SceneException& ex ) {
            exceptionCaught = true;
            EXPECT_EQ( ex.what(), expectedMessage );
        }
        EXPECT_TRUE( exceptionCaught );
    }
};

TEST_F( InArrayEventTests, inForInts ) {  // 1.1
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 1 } ) );
    EXPECT_FALSE( event.compare( { 1 } ) );
}

TEST_F( InArrayEventTests, inForStrings ) {  // 1.2
    Json payload = R"({ "type": "array.string", "value": ["1", "2", "3"] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    EXPECT_FALSE( event.compare( { "0" } ) );
    EXPECT_TRUE( event.compare( { "1" } ) );
    EXPECT_FALSE( event.compare( { "1" } ) );
}

TEST_F( InArrayEventTests, inForScalableValuesSameTypes ) {  // 1.3
    Json payload = R"({
        "type": "array.temperature",
        "value": [
            { "value": 1, "scale": "fahrenheit" },
            { "value": 2, "scale": "fahrenheit" },
            { "value": 3, "scale": "fahrenheit" }
        ]
    })"_json;

    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    const Json item0F = R"({ "value": 0, "scale": "fahrenheit" })"_json;
    const Json item1F = R"({ "value": 1, "scale": "fahrenheit" })"_json;
    const Json item2F = R"({ "value": 2, "scale": "fahrenheit" })"_json;
    const Json item3F = R"({ "value": 3, "scale": "fahrenheit" })"_json;

    EXPECT_FALSE( event.compare( item0F ) );
    EXPECT_TRUE( event.compare( item1F ) );
    EXPECT_FALSE( event.compare( item1F ) );

    EXPECT_FALSE( event.compare( item0F ) );
    EXPECT_TRUE( event.compare( item2F ) );

    EXPECT_FALSE( event.compare( item0F ) );
    EXPECT_TRUE( event.compare( item3F ) );
}

TEST_F( InArrayEventTests, inForIncomparableValues ) {  // 1.4
    Json payload = R"({ "type": "array.string", "value": ["1", "2", "3"] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    EXPECT_THROW( event.compare( { 0 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 1 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 2 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 3 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 4 } ), SceneEngine::SceneException );
}

TEST_F( InArrayEventTests, inForNotSameButComparableNumericValues ) {  // 1.5
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 1.0f } ) );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 2.0f } ) );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 3.0f } ) );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_FALSE( event.compare( { 4.0f } ) );
}

TEST_F( InArrayEventTests, inForNotSameButComparableScalableValues ) {  // 1.6
    // array is equal to [1.0C, 2.0C, 3.0C]
    Json payload = R"({
        "type": "array.temperature",
        "value": [
            { "value": 33.8, "scale": "fahrenheit" },
            { "value": 35.6, "scale": "fahrenheit" },
            { "value": 37.4, "scale": "fahrenheit" }
        ]
    })"_json;

    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    const Json celsius0 = R"({ "type": "temperature", "value": { "value": 0.0, "scale": "celsius" } })"_json;
    const Json celsius1 = R"({ "type": "temperature", "value": { "value": 1.0, "scale": "celsius" } })"_json;  // 33.8F
    const Json celsius2 = R"({ "type": "temperature", "value": { "value": 2.0, "scale": "celsius" } })"_json;  // 35.6F
    const Json celsius3 = R"({ "type": "temperature", "value": { "value": 3.0, "scale": "celsius" } })"_json;  // 37.4F

    EXPECT_FALSE( event.compare( celsius0 ) );
    EXPECT_TRUE( event.compare( celsius1 ) );
    EXPECT_FALSE( event.compare( celsius1 ) );

    EXPECT_FALSE( event.compare( celsius0 ) );
    EXPECT_TRUE( event.compare( celsius2 ) );

    EXPECT_FALSE( event.compare( celsius0 ) );
    EXPECT_TRUE( event.compare( celsius3 ) );
}

TEST_F( InArrayEventTests, DISABLED_inForArrayWithDifferentScalableValues ) {  // 1.7
    Json payload = R"({
        "type": "array.temperature",
        "value": [
            { "value": 1.0, "scale": "celsius" },
            { "value": 35.6, "scale": "fahrenheit" },
            { "value": 3.0, "scale": "celsius" }
        ]
    })"_json;

    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    const Json celsius0 = R"({ "type": "temperature", "value": { "value": 0.0, "scale": "celsius" } })"_json;
    const Json celsius1 = R"({ "type": "temperature", "value": { "value": 33.8, "scale": "fahrenheit" } })"_json;
    const Json celsius2 = R"({ "type": "temperature", "value": { "value": 2.0, "scale": "celsius" } })"_json;
    const Json celsius3 = R"({ "type": "temperature", "value": { "value": 37.4, "scale": "fahrenheit" } })"_json;

    EXPECT_FALSE( event.compare( celsius0 ) );
    EXPECT_TRUE( event.compare( celsius1 ) );
    EXPECT_FALSE( event.compare( celsius1 ) );

    EXPECT_FALSE( event.compare( celsius0 ) );
    EXPECT_TRUE( event.compare( celsius2 ) );

    EXPECT_FALSE( event.compare( celsius0 ) );
    EXPECT_TRUE( event.compare( celsius3 ) );
}

TEST_F( InArrayEventTests, notInForInts ) {  // 2.1
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );

    EXPECT_FALSE( event.compare( { 1 } ) );
    EXPECT_FALSE( event.compare( { 2 } ) );
    EXPECT_FALSE( event.compare( { 3 } ) );
    EXPECT_TRUE( event.compare( { 0 } ) );
    EXPECT_FALSE( event.compare( { 0 } ) );
}

TEST_F( InArrayEventTests, notInForStrings ) {  // 2.2
    Json payload = R"({ "type": "array.string", "value": ["1", "2", "3"] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );

    EXPECT_FALSE( event.compare( { "1" } ) );
    EXPECT_FALSE( event.compare( { "2" } ) );
    EXPECT_FALSE( event.compare( { "3" } ) );
    EXPECT_TRUE( event.compare( { "0" } ) );
    EXPECT_FALSE( event.compare( { "0" } ) );
}

TEST_F( InArrayEventTests, notInForScalableValues ) {  // 2.3
    Json payload = R"({
        "type": "array.temperature",
        "value": [
            { "value": 1.0, "scale": "fahrenheit" },
            { "value": 2.0, "scale": "fahrenheit" },
            { "value": 3.0, "scale": "fahrenheit" }
        ]
    })"_json;

    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );
    ;

    const Json item0F = R"({ "type": "temperature", "value": { "value": 0.0, "scale": "fahrenheit" } })"_json;
    const Json item1F = R"({ "type": "temperature", "value": { "value": 1.0, "scale": "fahrenheit" } })"_json;
    const Json item2F = R"({ "type": "temperature", "value": { "value": 2.0, "scale": "fahrenheit" } })"_json;
    const Json item3F = R"({ "type": "temperature", "value": { "value": 3.0, "scale": "fahrenheit" } })"_json;
    const Json item4F = R"({ "type": "temperature", "value": { "value": 4.0, "scale": "fahrenheit" } })"_json;

    EXPECT_FALSE( event.compare( item1F ) );
    EXPECT_FALSE( event.compare( item2F ) );
    EXPECT_FALSE( event.compare( item3F ) );

    EXPECT_TRUE( event.compare( item0F ) );
    EXPECT_FALSE( event.compare( item0F ) );

    EXPECT_FALSE( event.compare( item1F ) );
    EXPECT_TRUE( event.compare( item4F ) );
}

TEST_F( InArrayEventTests, notInForIncomparableValues ) {  // 2.4
    Json payload = R"({ "type": "array.string", "value": ["1", "2", "3"] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );

    EXPECT_THROW( event.compare( { 0 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 1 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 2 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 3 } ), SceneEngine::SceneException );
    EXPECT_THROW( event.compare( { 4 } ), SceneEngine::SceneException );
}

TEST_F( InArrayEventTests, notInForNotSameButComparableNumericValues ) {  // 2.5
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );

    EXPECT_FALSE( event.compare( { 1.0f } ) );
    EXPECT_FALSE( event.compare( { 2.0f } ) );
    EXPECT_FALSE( event.compare( { 3.0f } ) );

    EXPECT_TRUE( event.compare( { 0.0f } ) );
    EXPECT_FALSE( event.compare( { 0.0f } ) );

    EXPECT_FALSE( event.compare( { 1.0f } ) );
    EXPECT_TRUE( event.compare( { 4.0f } ) );
    EXPECT_FALSE( event.compare( { 4.0f } ) );
}

TEST_F( InArrayEventTests, notInForNotSameButComparableScalableValues ) {  // 2.6
    // array is equal to [1.0C, 2.0C, 3.0C]
    Json payload = R"({
        "type": "array.temperature",
        "value": [
            { "value": 33.8, "scale": "fahrenheit" },
            { "value": 35.6, "scale": "fahrenheit" },
            { "value": 37.4, "scale": "fahrenheit" }
        ]
    })"_json;

    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );

    const Json celsius0 = R"({ "type": "temperature", "value": { "value": 0.0, "scale": "celsius" } })"_json;
    const Json celsius1 = R"({ "type": "temperature", "value": { "value": 1.0, "scale": "celsius" } })"_json;  // 33.8F
    const Json celsius2 = R"({ "type": "temperature", "value": { "value": 2.0, "scale": "celsius" } })"_json;  // 35.6F
    const Json celsius3 = R"({ "type": "temperature", "value": { "value": 3.0, "scale": "celsius" } })"_json;  // 37.4F
    const Json celsius4 = R"({ "type": "temperature", "value": { "value": 4.0, "scale": "celsius" } })"_json;  // 39.2F

    EXPECT_FALSE( event.compare( celsius1 ) );
    EXPECT_FALSE( event.compare( celsius2 ) );
    EXPECT_FALSE( event.compare( celsius3 ) );

    EXPECT_TRUE( event.compare( celsius0 ) );
    EXPECT_FALSE( event.compare( celsius0 ) );

    EXPECT_FALSE( event.compare( celsius1 ) );
    EXPECT_TRUE( event.compare( celsius4 ) );
    EXPECT_FALSE( event.compare( celsius4 ) );
}

TEST_F( InArrayEventTests, DISABLED_notInForArrayWithDifferentScalableValues ) {  // 2.7
    Json payload = R"({
        "type": "array.temperature",
        "value": [
            { "value": 1.0, "scale": "celsius" },
            { "value": 35.6, "scale": "fahrenheit" },
            { "value": 3.0, "scale": "celsius" }
        ]
    })"_json;

    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );

    const Json celsius0 = R"({ "type": "temperature", "value": { "value": 0.0, "scale": "celsius" } })"_json;
    const Json celsius1 = R"({ "type": "temperature", "value": { "value": 33.8, "scale": "fahrenheit" } })"_json;
    const Json celsius2 = R"({ "type": "temperature", "value": { "value": 2.0, "scale": "celsius" } })"_json;
    const Json celsius3 = R"({ "type": "temperature", "value": { "value": 37.4, "scale": "fahrenheit" } })"_json;
    const Json celsius4 = R"({ "type": "temperature", "value": { "value": 39.2, "scale": "fahrenheit" } })"_json;

    EXPECT_FALSE( event.compare( celsius1 ) );
    EXPECT_FALSE( event.compare( celsius2 ) );
    EXPECT_FALSE( event.compare( celsius3 ) );

    EXPECT_TRUE( event.compare( celsius0 ) );
    EXPECT_FALSE( event.compare( celsius0 ) );

    EXPECT_FALSE( event.compare( celsius1 ) );
    EXPECT_TRUE( event.compare( celsius4 ) );
    EXPECT_FALSE( event.compare( celsius4 ) );
}

TEST_F( InArrayEventTests, eventNotCreatedWithIncorrectOperation ) {  // 3.
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    const std::string operation = "invalid_op";
    const std::string expectedMessage = "inArray: unknown operation: " + operation;

    checkExceptionOnEventCreation( payload, operation, expectedMessage );
}

TEST_F( InArrayEventTests, eventNotCreatedWithArrayOfIncorrectSize ) {  // 4.
    const std::string expectedMessage = "inArray: array should have at least 2 values, for comparing against a "
                                        "single value use 'isItemState' method";

    Json payloadEmptyArray = R"({ "type": "array.int", "value": [] })"_json;
    Json payloadArrayWithOneElement = R"({ "type": "array.int", "value": [1] })"_json;

    checkExceptionOnEventCreation( payloadEmptyArray, ArrayOperations::In, expectedMessage );
    checkExceptionOnEventCreation( payloadArrayWithOneElement, ArrayOperations::In, expectedMessage );
}

TEST_F( InArrayEventTests, eventIsCreatedWithEmptyOperation ) {  // 5.
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 1 } ) );
    EXPECT_FALSE( event.compare( { 1 } ) );
}

TEST_F( InArrayEventTests, inForIntsWithoutState ) {  // 6.1
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );
    event.setKeepState( false );

    EXPECT_FALSE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 1 } ) );
    EXPECT_TRUE( event.compare( { 1 } ) );
}

TEST_F( InArrayEventTests, inForStringsWithoutState ) {  // 6.2
    Json payload = R"({ "type": "array.string", "value": ["1", "2", "3"] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::In );
    event.setKeepState( false );

    EXPECT_FALSE( event.compare( { "0" } ) );
    EXPECT_TRUE( event.compare( { "1" } ) );
    EXPECT_TRUE( event.compare( { "2" } ) );
}

TEST_F( InArrayEventTests, notInForIntsWithoutState ) {  // 6.3
    Json payload = R"({ "type": "array.int", "value": [1, 2, 3] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );
    event.setKeepState( false );

    EXPECT_FALSE( event.compare( { 1 } ) );
    EXPECT_FALSE( event.compare( { 2 } ) );
    EXPECT_TRUE( event.compare( { 0 } ) );
    EXPECT_TRUE( event.compare( { 5 } ) );
}

TEST_F( InArrayEventTests, notInForStringsWithoutState ) {  // 6.4
    Json payload = R"({ "type": "array.string", "value": ["1", "2", "3"] })"_json;
    InArrayEvent event = createInArrayEvent( std::move( payload ), ArrayOperations::NotIn );
    event.setKeepState( false );

    EXPECT_FALSE( event.compare( { "1" } ) );
    EXPECT_FALSE( event.compare( { "2" } ) );
    EXPECT_TRUE( event.compare( { "0" } ) );
    EXPECT_TRUE( event.compare( { "5" } ) );
}

}  // namespace UT
}  // namespace SceneEngine
