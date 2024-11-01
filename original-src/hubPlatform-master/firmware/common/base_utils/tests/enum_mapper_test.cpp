#include "base_utils/enum_mapper.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace HubPlatform {
namespace UT {

namespace {
struct Data {
    const char* m_name;
};

enum class UnitTypes {
    Int,
    Float,
    String,
    Custom,

    Count,
    Invalid = Count
};

}  // namespace

TEST( EnumMapperTestSuite, enumMapper ) {
    const EnumMapper< UnitTypes, Data > mapper( {

        { UnitTypes::Float, { "float" } },
        { UnitTypes::Int, { "int" } },
        { UnitTypes::Custom, { "custom" } },
        { UnitTypes::String, { "string" } },

    } );

    {
        auto it = mapper.find( UnitTypes::Float );
        ASSERT_NE( it, mapper.end() );
        EXPECT_EQ( std::string( "float" ), it->m_name );
    }

    {
        auto it = mapper.find( UnitTypes::Custom );
        ASSERT_NE( it, mapper.end() );
        EXPECT_EQ( std::string( "custom" ), it->m_name );
    }

    {
        auto it = mapper.find( UnitTypes::Count );
        EXPECT_EQ( it, mapper.end() );
    }

    const auto namesToTypes =
        mapper.createReverseMapper( []( const Data& data ) { return data.m_name; },
                                    []( const char* a, const char* b ) { return strcmp( a, b ) < 0; } );

    EXPECT_EQ( namesToTypes.find( "int" ), UnitTypes::Int );
    EXPECT_EQ( namesToTypes.find( "float" ), UnitTypes::Float );
    EXPECT_EQ( namesToTypes.find( "string" ), UnitTypes::String );
    EXPECT_EQ( namesToTypes.find( "custom" ), UnitTypes::Custom );

    EXPECT_EQ( namesToTypes.find( "" ), UnitTypes::Invalid );
    EXPECT_EQ( namesToTypes.find( "float1" ), UnitTypes::Invalid );
    EXPECT_EQ( namesToTypes.find( "z" ), UnitTypes::Invalid );
}

TEST( EnumMapperTestSuite, invalidInit ) {
    auto createIncompleteMapper = []() {
        return EnumMapper< UnitTypes, Data >( {

            { UnitTypes::Float, { "float" } },
            { UnitTypes::Int, { "int" } },
            { UnitTypes::Custom, { "custom" } },
            { UnitTypes::String, { "string" } },
            { UnitTypes::Custom, { "custom" } },

        } );
    };

    auto createMapperWithDuplications = []() {
        return EnumMapper< UnitTypes, Data >( {

            { UnitTypes::Float, { "float" } },
            { UnitTypes::Int, { "int" } },
            { UnitTypes::Custom, { "custom" } },
            { UnitTypes::String, { "string" } },
            { UnitTypes::Custom, { "" } },

        } );
    };

    auto createMapperWithWrongIndexes = []() {
        return EnumMapper< UnitTypes, Data >( {

            { UnitTypes::Float, { "float" } },
            { UnitTypes::Int, { "int" } },
            { UnitTypes::Custom, { "custom" } },
            { UnitTypes::String, { "string" } },
            { UnitTypes::Invalid, { "invalid" } },

        } );
    };

    EXPECT_THROW( createIncompleteMapper(), std::logic_error );
    EXPECT_THROW( createMapperWithDuplications(), std::logic_error );
    EXPECT_THROW( createMapperWithWrongIndexes(), std::logic_error );
}

}  // namespace UT
}  // namespace HubPlatform
