#include "cored/model/values/value_type.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {

namespace CoreD {
namespace Model {
namespace ValueTypeInfo {
namespace UT {

TEST( ValueTypeInfoTestSuite, fromString ) {
    EXPECT_EQ( ValueType::fromString( "float" ), ValueType::Enum::Float );
    EXPECT_EQ( ValueType::fromString( "humidity" ), ValueType::Enum::Humidity );
    EXPECT_EQ( ValueType::fromString( "air_quality" ), ValueType::Enum::AirQuality );

    EXPECT_EQ( ValueType::fromString( "humidit" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "humidity." ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "air_quality." ), ValueType::Enum::NoValue );

    EXPECT_EQ( ValueType::fromString( "dictionary" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "dictionary." ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "token.dictionary" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( ".dictionary" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( ".dictionary" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "dictionary.dictionary" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "dictionary.int" ), ValueType::makeDictionaryType( ValueType::Enum::Int ) );

    EXPECT_EQ( ValueType::fromString( "array" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "array." ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "int.array" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "array.array" ), ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "array.string" ), ValueType::makeArrayType( ValueType::Enum::String ) );

    EXPECT_EQ( ValueType::fromString( "userCode.dictionary.array.dictionary.dictionary.userCode" ),
               ValueType::Enum::NoValue );
    EXPECT_EQ( ValueType::fromString( "dictionary.array.dictionary.dictionary.userCode" ),
               ValueType::makeDictionaryType( ValueType::makeArrayType(
                   ValueType::makeDictionaryType( ValueType::makeDictionaryType( ValueType::Enum::UserCode ) ) ) ) );
}

TEST( ValueTypeInfoTestSuite, toString ) {
    EXPECT_EQ( ValueType( ValueType::Enum::Power ).toString(), "power" );
    EXPECT_EQ( ValueType( ValueType::Enum::Int ).toString(), "int" );

    EXPECT_EQ( ValueType( ValueType::Enum::NoValue ).toString(), "" );
    EXPECT_EQ( ValueType( ValueType::makeDictionaryType( ValueType::Enum::Temperature ) ).toString(),
               "dictionary.temperature" );

    EXPECT_EQ( ValueType( ValueType::makeDictionaryType( ValueType::Enum::NoValue ) ).toString(), "" );
    EXPECT_EQ( ValueType( ValueType::makeArrayType( ValueType::Enum::NoValue ) ).toString(), "" );

    EXPECT_EQ( ValueType::makeArrayType( ValueType::makeDictionaryType( ValueType::makeDictionaryType(
                                             ValueType::makeArrayType( ValueType::Enum::Token ) ) ) )
                   .toString(),
               "array.dictionary.dictionary.array.token" );
}

TEST( ValueTypeInfoTestSuite, getSubAndMainType ) {
    // simple type
    {
        const auto simpleType = ValueType( ValueType::Enum::Humidity );
        EXPECT_EQ( simpleType.getMainType(), ValueType::Enum::Humidity );
        EXPECT_EQ( simpleType.getSubType(), nullptr );
    }

    // complicated type
    {
        const auto complicatedType = ValueType::makeArrayType( ValueType::makeDictionaryType(
            ValueType::makeDictionaryType( ValueType::makeArrayType( ValueType::Enum::Token ) ) ) );

        std::vector< ValueType::Enum > mainTypes;

        for( auto type = &complicatedType; type; type = type->getSubType().get() ) {
            mainTypes.push_back( type->getMainType() );
        }

        EXPECT_THAT( mainTypes,
                     ::testing::ElementsAre( ValueType::Enum::Array,
                                             ValueType::Enum::Dictionary,
                                             ValueType::Enum::Dictionary,
                                             ValueType::Enum::Array,
                                             ValueType::Enum::Token ) );
    }
}

TEST( ValueTypeInfoTestSuite, hasType ) {
    // simple type
    {
        const auto simpleType = ValueType( ValueType::Enum::Humidity );
        EXPECT_TRUE( simpleType.hasType( ValueType::Enum::Humidity ) );
        EXPECT_FALSE( simpleType.hasType( ValueType::Enum::KiloVoltAmpereHour ) );
        EXPECT_FALSE( simpleType.hasType( ValueType::Enum::Array ) );
        EXPECT_FALSE( simpleType.hasType( ValueType::Enum::Dictionary ) );
    }

    // dictionary type
    {
        const auto dictionaryType = ValueType::makeDictionaryType( ValueType::Enum::Token );

        EXPECT_FALSE( dictionaryType.hasType( ValueType::Enum::Humidity ) );
        EXPECT_FALSE( dictionaryType.hasType( ValueType::Enum::Array ) );
        EXPECT_TRUE( dictionaryType.hasType( ValueType::Enum::Dictionary ) );
        EXPECT_TRUE( dictionaryType.hasType( ValueType::Enum::Token ) );
    }

    // complicated type
    {
        const auto complicatedType = ValueType::makeArrayType( ValueType::makeDictionaryType(
            ValueType::makeDictionaryType( ValueType::makeArrayType( ValueType::Enum::Token ) ) ) );

        EXPECT_FALSE( complicatedType.hasType( ValueType::Enum::Humidity ) );
        EXPECT_TRUE( complicatedType.hasType( ValueType::Enum::Array ) );
        EXPECT_TRUE( complicatedType.hasType( ValueType::Enum::Dictionary ) );
        EXPECT_TRUE( complicatedType.hasType( ValueType::Enum::Token ) );
    }
}

}  // namespace UT
}  // namespace ValueTypeInfo
}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
