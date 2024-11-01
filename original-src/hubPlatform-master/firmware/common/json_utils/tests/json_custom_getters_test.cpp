#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "json_utils/json_custom_getters.h"
#include "json_utils/json_wrapper.h"

namespace JsonCustomGetters {

using namespace testing;

class JsonCustomGettersTestSuite : public Test {};

TEST_F( JsonCustomGettersTestSuite, getVectorOfDefaultTypesFromArray ) {
    {
        const auto getter = makeDefaultGetter< std::vector< int > >();
        std::vector< int > result;
        EXPECT_TRUE( getter( JSON_ARRAY( 1 << 10 << 100.1f ), result ) );
        EXPECT_EQ( result, std::vector< int >( { 1, 10, 100 } ) );
    }

    {
        const auto getter = makeDefaultGetter< std::vector< bool > >();
        std::vector< bool > result;
        EXPECT_TRUE( getter( JSON_ARRAY( true << false ), result ) );
        EXPECT_EQ( result, std::vector< bool >( { true, false } ) );
    }

    {
        const auto getter = makeDefaultGetter< std::vector< std::string > >();
        std::vector< std::string > result;
        EXPECT_TRUE( getter( JSON_ARRAY( "a"
                                         << "b"
                                         << "c" ),
                             result ) );
        EXPECT_EQ( result, std::vector< std::string >( { "a", "b", "c" } ) );
    }
}

TEST_F( JsonCustomGettersTestSuite, getEmptyVectorFromArray ) {
    const auto getter = makeDefaultGetter< std::vector< int > >();
    std::vector< int > result;
    EXPECT_FALSE( getter( Json::array(), result, false ) );
    EXPECT_TRUE( result.empty() );
}

TEST_F( JsonCustomGettersTestSuite, getVectorOfDefaultTypesFromDifferentArrayTypes ) {
    const JsonArray array = JSON_ARRAY( 1 << 2.4f << "a" << true );

    {
        std::vector< int > result;
        EXPECT_TRUE( makeDefaultGetter< std::vector< int > >()( array, result, false ) );
        EXPECT_EQ( result, std::vector< int >( { 1, 2 } ) );
    }

    {
        std::vector< bool > result;
        EXPECT_TRUE( makeDefaultGetter< std::vector< bool > >()( array, result, false ) );
        EXPECT_EQ( result, std::vector< bool >( { true } ) );
    }

    {
        std::vector< std::string > result;
        EXPECT_TRUE( makeDefaultGetter< std::vector< std::string > >()( array, result, false ) );
        EXPECT_EQ( result, std::vector< std::string >( { "a" } ) );
    }

    {
        std::vector< std::string > result;
        EXPECT_FALSE( makeVectorGetter( makeKeyGetter() )( array, result, false ) );
        EXPECT_TRUE( result.empty() );
    }
}

TEST_F( JsonCustomGettersTestSuite, getVectorOfDefaultTypesFromObject ) {
    const JsonObject obj = JSON( "id" << 1 << "description"
                                      << "test" );

    {
        std::vector< std::string > keys;
        const auto getter = makeVectorGetter( makeKeyGetter() );
        EXPECT_TRUE( getter( obj, keys, false ) );
        EXPECT_THAT( keys, ElementsAre( "description", "id" ) );
    }

    {
        std::vector< int > integers;
        const auto getter = makeDefaultGetter< std::vector< int > >();
        EXPECT_TRUE( getter( obj, integers, false ) );
        EXPECT_THAT( integers, ElementsAre( 1 ) );
    }
}

TEST_F( JsonCustomGettersTestSuite, getSimpleStruct ) {
    struct SimpleStruct {
        int m_id;
        std::string m_description;
    };

    {
        const auto getter = makeStructGetter( makeMemberMapping( &SimpleStruct::m_id, "id" ),
                                              makeMemberMapping( &SimpleStruct::m_description, "description" ) );

        SimpleStruct result{ 0, {} };
        EXPECT_TRUE( getter( JSON( "id" << 1 << "description"
                                        << "test" ),
                             result,
                             false ) );

        EXPECT_EQ( result.m_id, 1 );
        EXPECT_EQ( result.m_description, "test" );
    }

    {
        const auto getter = makeStructGetter( makeMemberMapping( &SimpleStruct::m_id, "myid" ),
                                              makeMemberMapping( &SimpleStruct::m_description, "description" ) );

        SimpleStruct result{ 0, {} };
        EXPECT_FALSE( getter( JSON( "id" << 1 << "description"
                                         << "test" ),
                              result,
                              false ) );
        EXPECT_EQ( result.m_description, "test" );
        EXPECT_EQ( result.m_id, 0 );
    }
}

TEST_F( JsonCustomGettersTestSuite, getSimpleStructWithPolicies ) {
    struct SimpleStruct {
        int m_id;
        std::string m_description;
    };

    {
        const auto getter = makeStructGetter( makeMemberMapping( &SimpleStruct::m_id, "myid", OptionalMemberPolicy() ),
                                              makeMemberMapping( &SimpleStruct::m_description, "description" ) );

        SimpleStruct result{ 0, {} };
        EXPECT_TRUE( getter( JSON( "id" << 1 << "description"
                                        << "test" ),
                             result,
                             false ) );
        EXPECT_EQ( result.m_description, "test" );
        EXPECT_EQ( result.m_id, 0 );
    }

    {
        const auto getter =
            makeStructGetter( makeMemberMapping( &SimpleStruct::m_id, "myid", OptionalMemberPolicy( false ) ),
                              makeMemberMapping( &SimpleStruct::m_description, "description" ) );

        SimpleStruct result{ 12, {} };
        EXPECT_TRUE( getter( JSON( "description"
                                   << "test" ),
                             result,
                             false ) );
        EXPECT_EQ( result.m_description, "test" );
        EXPECT_EQ( result.m_id, 12 );
    }

    {
        const auto getter = makeStructGetter( makeMemberMapping( &SimpleStruct::m_id, "myid", CriticalErrorPolicy() ),
                                              makeMemberMapping( &SimpleStruct::m_description, "description" ) );

        SimpleStruct result{ 0, {} };
        EXPECT_FALSE( getter( JSON( "id" << 1 << "description"
                                         << "test" ),
                              result,
                              false ) );
    }
}

TEST_F( JsonCustomGettersTestSuite, getVectorOfComplexStructsWithPolicies ) {
    struct Value {
        bool m_boolValue;
        std::vector< int > m_intValues;

        bool operator==( const Value& other ) const {
            return m_boolValue == other.m_boolValue && m_intValues == other.m_intValues;
        }
    };

    struct Struct {
        std::string m_name;
        int m_id;
        Value m_value;

        bool operator==( const Struct& other ) const {
            return m_name == other.m_name && m_id == other.m_id && m_value == other.m_value;
        }
    };

    const JsonObject obj =
        JSON( "first" << JSON( "id" << 1 << "value" << JSON( "bool" << false << "integers" << JSON_ARRAY( 1 << 2 ) ) )
                      << "second" << JSON( "id" << 2 << "value" << JSON( "bool" << true ) ) );

    {
        auto valueGetter = makeStructGetter( makeMemberMapping( &Value::m_boolValue, "bool" ),
                                             makeMemberMapping( &Value::m_intValues, "integers" ) );

        const auto getter = makeVectorGetter( makeStructGetter(
            makeMemberMapping( &Struct::m_name, makeKeyGetter() ),
            makeMemberMapping( &Struct::m_id, "id" ),
            makeMemberMapping( &Struct::m_value, makeFieldGetter( "value", std::move( valueGetter ) ) ) ) );

        std::vector< Struct > result;
        EXPECT_TRUE( getter( obj, result, false ) );

        const std::vector< Struct > expected{ { "first", 1, { false, { 1, 2 } } } };

        EXPECT_EQ( result, expected );
    }

    {
        auto valueGetter = makeStructGetter( makeMemberMapping( &Value::m_boolValue, "bool" ),
                                             makeMemberMapping( &Value::m_intValues, "integers" ) );

        const auto getter = makeVectorGetter( makeStructGetter(
            makeMemberMapping( &Struct::m_name, makeKeyGetter() ),
            makeMemberMapping( &Struct::m_id, "id" ),
            makeMemberMapping( &Struct::m_value,
                               makeFieldGetter( "value", std::move( valueGetter ), OptionalMemberPolicy() ) ) ) );

        std::vector< Struct > result;
        EXPECT_TRUE( getter( obj, result, false ) );

        const std::vector< Struct > expected{ { "first", 1, { false, { 1, 2 } } },
                                              { "second", 2, { false /* reseted by optional policy */, {} } } };

        EXPECT_EQ( result, expected );
    }

    {
        auto valueGetter = makeStructGetter( makeMemberMapping( &Value::m_boolValue, "bool" ),
                                             makeMemberMapping( &Value::m_intValues, "integers" ) );

        const auto getter = makeVectorGetter( makeStructGetter(
            makeMemberMapping( &Struct::m_name, makeKeyGetter() ),
            makeMemberMapping( &Struct::m_id, "id" ),
            makeMemberMapping( &Struct::m_value,
                               makeFieldGetter( "value", std::move( valueGetter ), CriticalErrorPolicy() ) ) ) );

        std::vector< Struct > result;
        EXPECT_FALSE( getter( obj, result, false ) );
    }
}

TEST_F( JsonCustomGettersTestSuite, getValuesFromEmptyObject ) {
    const JsonObject obj;

    int val = 0;
    EXPECT_FALSE( makeDefaultGetter< int >()( obj, val, false ) );

    std::vector< int > vals;
    EXPECT_FALSE( makeDefaultGetter< std::vector< int > >()( obj, vals, false ) );

    std::pair< int, int > pair( 0, 0 );
    EXPECT_FALSE( makeStructGetter( makeMemberMapping( &std::pair< int, int >::first, "" ) )( obj, pair, false ) );
}

TEST_F( JsonCustomGettersTestSuite, getValuesFromInconsistentObjectType ) {
    const JsonObject obj = "123";

    int val = 0;
    EXPECT_FALSE( makeDefaultGetter< int >()( obj, val, false ) );

    std::vector< int > vals;
    EXPECT_FALSE( makeDefaultGetter< std::vector< int > >()( obj, vals, false ) );

    std::pair< int, int > pair( 0, 0 );
    EXPECT_FALSE( makeStructGetter( makeMemberMapping( &std::pair< int, int >::first, "" ) )( obj, pair, false ) );

    std::vector< std::string > strings;
    EXPECT_TRUE( makeDefaultGetter< std::vector< std::string > >()( obj, strings ) );
    EXPECT_EQ( strings, std::vector< std::string >{ "123" } );
}

TEST_F( JsonCustomGettersTestSuite, appendValuesToVector ) {
    std::vector< int > values;
    EXPECT_TRUE( makeDefaultGetter< std::vector< int > >()( JSON_ARRAY( 1 << 2 ), values, false ) );
    EXPECT_TRUE( makeDefaultGetter< std::vector< int > >()( JSON_ARRAY( 3 << 4 ), values, false ) );
    EXPECT_FALSE( makeDefaultGetter< std::vector< int > >()( JSON_ARRAY( "" ), values, false ) );

    EXPECT_THAT( values, ElementsAre( 1, 2, 3, 4 ) );
}

}  // namespace JsonCustomGetters
