#include "scenes_manager/scenes_engine/events/comparable_value.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"
#include "json_utils/json_utils.h"
#include "cored/model/values/value.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_factory.h"
#include <gtest/gtest.h>

#include <vector>
#include <utility>

namespace SceneEngine {

namespace {
using namespace HubPlatform::CoreD::Model;

ComparableValue createBoolValue( bool value ) {
    auto itemValue = ValueFactory::createItemValue( ValueType::Enum::Bool, Json( value ) );
    return ComparableValue( *itemValue );
}

ComparableValue createIntValue( int value ) {
    auto itemValue = ValueFactory::createItemValue( ValueType::Enum::Int, Json( value ) );
    return ComparableValue( *itemValue );
}

ComparableValue createFloatValue( float value ) {
    auto itemValue = ValueFactory::createItemValue( ValueType::Enum::Float, Json( value ) );
    return ComparableValue( *itemValue );
}

ComparableValue createScalableValue( const ValueType& type, const Json& value ) {
    auto itemValue = ValueFactory::createItemValue( type, value );
    return ComparableValue( *itemValue );
}

ComparableValue createTemperatureValue( float value ) {
    return createScalableValue( ValueType::Enum::Temperature,
                                JSON( "scale"
                                      << "celsius"
                                      << "value" << value ) );
};

ComparableValue createStringValue( std::string value ) {
    auto itemValue = ValueFactory::createItemValue( ValueType::Enum::String, Json( std::move( value ) ) );
    return ComparableValue( *itemValue );
}

ComparableValue createTokenValue( std::string value ) {
    auto itemValue = ValueFactory::createItemValue( ValueType::Enum::Token, Json( std::move( value ) ) );
    return ComparableValue( *itemValue );
}

}  // namespace

namespace UT {

/*

TESTPLAN

DONE    1. Comparable Value
DONE        1.1 Compare values
DONE            1.1.1 int to int
DONE            1.1.2 int to float
DONE            1.1.3 int to scalable
DONE            1.1.4 float to float
DONE            1.1.5 float to scalable
DONE            1.1.6 string to token
DONE        1.2 Is comparable type
DONE            1.2.1 isNumericType
DONE            1.2.2 isStringType
DONE        1.3 Create non comparable value

*/

using ::testing::Test;

class ComparableValueTests : public Test {};

TEST_F( ComparableValueTests, compareIntToInt ) {  // 1.1.1
    auto valueInt1 = createIntValue( 1 );
    auto valueInt2 = createIntValue( 2 );

    EXPECT_TRUE( valueInt1 == valueInt1 );
    EXPECT_FALSE( valueInt1 == valueInt2 );

    EXPECT_TRUE( valueInt1 < valueInt2 );
    EXPECT_FALSE( valueInt2 < valueInt1 );
}

TEST_F( ComparableValueTests, compareIntToFloat ) {  // 1.1.2
    auto valueInt1 = createIntValue( 1 );

    EXPECT_TRUE( valueInt1 == createFloatValue( 1.0 ) );
    EXPECT_FALSE( valueInt1 == createFloatValue( 1.1 ) );

    EXPECT_TRUE( valueInt1 < createFloatValue( 1.1 ) );
    EXPECT_FALSE( valueInt1 < createFloatValue( 1.0 ) );
    EXPECT_FALSE( valueInt1 < createFloatValue( 0.9 ) );
}

TEST_F( ComparableValueTests, compareIntToScalable ) {  // 1.1.3
    auto valueInt1 = createIntValue( 1 );

    EXPECT_TRUE( valueInt1 == createTemperatureValue( 1.0 ) );
    EXPECT_FALSE( createTemperatureValue( 1.1 ) == valueInt1 );

    EXPECT_TRUE( valueInt1 < createTemperatureValue( 1.1 ) );
    EXPECT_FALSE( valueInt1 < createTemperatureValue( 1.0 ) );
    EXPECT_FALSE( createTemperatureValue( 1.1 ) < valueInt1 );
}

TEST_F( ComparableValueTests, compareFloatToFloat ) {  // 1.1.4
    auto valueFloat1 = createFloatValue( 1.0 );

    EXPECT_TRUE( valueFloat1 == valueFloat1 );
    EXPECT_FALSE( valueFloat1 == createFloatValue( 1.1 ) );

    EXPECT_TRUE( valueFloat1 < createFloatValue( 1.1 ) );
    EXPECT_FALSE( createFloatValue( 1.0 ) < valueFloat1 );
    EXPECT_FALSE( createFloatValue( 1.1 ) < valueFloat1 );
}

TEST_F( ComparableValueTests, compareFloatToScalable ) {  // 1.1.5
    auto valueFloat1 = createFloatValue( 1.0 );

    EXPECT_TRUE( valueFloat1 == valueFloat1 );
    EXPECT_FALSE( valueFloat1 == createTemperatureValue( 1.1 ) );

    EXPECT_TRUE( valueFloat1 < createTemperatureValue( 1.1 ) );
    EXPECT_FALSE( createTemperatureValue( 1.0 ) < valueFloat1 );
    EXPECT_FALSE( createTemperatureValue( 1.1 ) < valueFloat1 );
}

TEST_F( ComparableValueTests, compareStringToToken ) {  // 1.1.6
    auto valueString1 = createStringValue( "text1" );

    EXPECT_TRUE( valueString1 == valueString1 );
    EXPECT_FALSE( valueString1 == createTokenValue( "text2" ) );

    EXPECT_TRUE( valueString1 < createTokenValue( "text2" ) );
    EXPECT_FALSE( createTokenValue( "text1" ) < valueString1 );
    EXPECT_FALSE( createTokenValue( "text2" ) < valueString1 );
}

TEST_F( ComparableValueTests, isNumericType ) {  // 1.2.1
    EXPECT_TRUE( ComparableValue::isNumericType( ValueType::Enum::Int ) );
    EXPECT_TRUE( ComparableValue::isNumericType( ValueType::Enum::Float ) );
    EXPECT_TRUE( ComparableValue::isNumericType( ValueType::Enum::Temperature ) );

    EXPECT_FALSE( ComparableValue::isNumericType( ValueType::Enum::Token ) );
    EXPECT_FALSE( ComparableValue::isNumericType( ValueType::Enum::Bool ) )
        << "Bool should be treated as standalone type";
}

TEST_F( ComparableValueTests, isStringType ) {  // 1.2.2
    EXPECT_TRUE( ComparableValue::isStringType( ValueType::Enum::String ) );
    EXPECT_TRUE( ComparableValue::isStringType( ValueType::Enum::Token ) );

    EXPECT_FALSE( ComparableValue::isStringType( ValueType::Enum::Int ) );
}

TEST_F( ComparableValueTests, createNonComparableType ) {  // 1.3
    auto hotzoneValue = ValueFactory::createDefaultItemValue( ValueType::Enum::CameraHotzone );
    EXPECT_THROW( ComparableValue{ *hotzoneValue }, EventException );

    auto boolValue = ValueFactory::createItemValue( ValueType::Enum::Bool, Json( true ) );
    EXPECT_THROW( ComparableValue{ *boolValue }, EventException ) << "Bool should be treated as standalone type";
}

}  // namespace UT
}  // namespace SceneEngine
