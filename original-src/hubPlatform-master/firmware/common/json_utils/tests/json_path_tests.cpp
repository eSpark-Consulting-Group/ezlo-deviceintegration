#include "json_utils/json_utils.h"
#include "json_utils/json_path.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

class JsonPathTests : public ::testing::Test {};

TEST_F( JsonPathTests, jsonPathToString ) {
    JsonPath path;

    std::vector< Json > keys = { "key1", "key2 with spaces", 0, 22 };
    for( const auto& it : keys )
        path.pushBack( it );

    EXPECT_FALSE( path.empty() ) << "Should be not empty";
    LOGGER_TRACE( path.toString() );
    EXPECT_EQ( path.toString(), "/\"key1\"/\"key2 with spaces\"/[0]/[22]" );
}

TEST_F( JsonPathTests, jsonPathTravel ) {
    JsonPath path;

    std::function< void( int& ) > goDeep = [&]( int& deep ) -> void {
        auto prevKeysSize = path.keys().size();
        auto guard = JsonPath::Guard( path, deep );

        LOGGER_TRACE_RAW( cc::trace( "'" ).append( path.toString() ).append( cc::trace( "'" ) ) );
        EXPECT_EQ( prevKeysSize + 1, path.keys().size() ) << "Should add a key to a path";
        if( --deep >= 0 )
            goDeep( deep );
    };

    int const startFrom = 10;
    int deepness = startFrom;

    auto prevKeysSize = path.keys().size();
    goDeep( deepness );

    EXPECT_EQ( prevKeysSize, path.keys().size() )
        << "JsonPath::Guard should remove all from a stack on every scope leaving";
    EXPECT_TRUE( path.empty() );
}

TEST_F( JsonPathTests, jsonPathBack ) {
    JsonPath path;
    Json key = { "string" };
    path.pushBack( key );

    EXPECT_EQ( path.back(), key ) << "back() should return latest inserted value";

    path.popBack();
    EXPECT_TRUE( path.empty() );

    Json returnFromEmpty;
    ASSERT_NO_THROW( returnFromEmpty = path.back() );

    ASSERT_NO_THROW( path.popBack() );
}

TEST_F( JsonPathTests, append ) {
    JsonPath path, toAppend;
    Json key = { "string" };
    path.pushBack( key );
    toAppend.pushBack( key );
    toAppend.pushBack( key );
    toAppend.pushBack( key );

    ASSERT_NO_THROW( path.append( toAppend ) );
    EXPECT_EQ( path.keys().size(), 1 + toAppend.keys().size() );
}

TEST_F( JsonPathTests, constructors ) {
    // from array
    JsonPath::Keys keys = { "string", 0, 22, "another string" };
    JsonPath path( keys );
    EXPECT_EQ( path.keys().size(), keys.size() ) << path.toString();

    // from initializer_list
    JsonPath path2 = { 1, 2, 3, 4, 5, "key1", "key2" };
    EXPECT_EQ( path2.keys().size(), 7 ) << path2.toString();
}

}  // namespace UT
}  // namespace HubPlatform