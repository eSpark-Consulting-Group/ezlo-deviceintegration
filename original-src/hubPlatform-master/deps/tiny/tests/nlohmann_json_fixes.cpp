#include "include/json.hpp"
#include "tiny/tiny_consolecolors.h"
#include "tiny/tiny_consolecolors_filters.h"

#include <gtest/gtest.h>
#include <chrono>

namespace tiny {
namespace UT {
TEST( JsonTestSuite, fixBrokenConstancyOnConstChildGetting ) {
    nlohmann::basic_json<> json;

    json[ "1" ] = "2";

    const auto& constJson = json;

    EXPECT_FALSE( constJson[ "1" ].is_null() );
    EXPECT_TRUE( constJson[ "2" ].is_null() );

    EXPECT_FALSE( constJson[ std::string( "1" ) ].is_null() );
    EXPECT_TRUE( constJson[ std::string( "2" ) ].is_null() );

    EXPECT_EQ( constJson.find( "2" ), constJson.end() );
    EXPECT_NE( constJson.find( "1" ), constJson.end() );
}

TEST( ConsoleColors, formatIntegers ) {
    nlohmann::basic_json<> json;

    using namespace std::chrono;
    int64_t now = duration_cast< milliseconds >( system_clock::now().time_since_epoch() ).count();
    json[ "now" ] = now;

    auto prevColorsOn = cc::_on_.load();
    if( prevColorsOn )
        cc::_on_.exchange( false );  // remove colors from json dump

    auto dump = json.dump();
    auto coloredDump = cc::c( json, -1 );

    nlohmann::basic_json<> parseFromDump = nlohmann::basic_json<>::parse( coloredDump );
    ASSERT_EQ( parseFromDump, json ) << "Should not truncate int64_t to int";

    if( cc::_on_ != prevColorsOn )
        cc::_on_.exchange( prevColorsOn );
}

}  // namespace UT
}  // namespace tiny
