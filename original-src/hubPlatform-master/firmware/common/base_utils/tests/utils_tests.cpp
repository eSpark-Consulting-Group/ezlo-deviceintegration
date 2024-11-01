#include "base_utils/utils.h"
#include "base_utils/unit_converter.h"
#include "base_utils/build_version.h"
#include "base_utils/firmware_version.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

TEST( UtilsTestSuite, escapeSymbolsByFirst ) {
    EXPECT_EQ( escapeSymbolsByFirst( "", "" ), "" );
    EXPECT_EQ( escapeSymbolsByFirst( "", "/" ), "" );
    EXPECT_EQ( escapeSymbolsByFirst( R"(a%b#c+\)", "%#" ), R"(a%%b%#c+\)" );
    EXPECT_EQ( escapeSymbolsByFirst( R"(\n/%'")", R"(\)" ), R"(\\n/%'")" );
    EXPECT_EQ( escapeSymbolsByFirst( R"(p\a$sw`or"d)", R"(\"`$)", 2 ), R"(p\\\a\\$sw\\`or\\"d)" );
    EXPECT_EQ( escapeSymbolsByFirst( R"(p\a$sw`or"d)", R"(\"`$)", 0 ), R"(p\a$sw`or"d)" );
}

// https://www.rapidtables.com/convert/color/rgb-to-hex.html
TEST( UtilsTestSuite, rgbToFormattedString ) {
    EXPECT_EQ( UnitConverter::rgbToFormattedString( 0, 0, 0 ), "#000000" );
    EXPECT_EQ( UnitConverter::rgbToFormattedString( 255, 255, 255 ), "#FFFFFF" );
    EXPECT_EQ( UnitConverter::rgbToFormattedString( 255, 0, 0 ), "#FF0000" );
    EXPECT_EQ( UnitConverter::rgbToFormattedString( 0, 255, 0 ), "#00FF00" );
    EXPECT_EQ( UnitConverter::rgbToFormattedString( 0, 0, 255 ), "#0000FF" );
    EXPECT_EQ( UnitConverter::rgbToFormattedString( 0, 0, 128 ), "#000080" );
}

TEST( UtilsTestSuite, buildVersionCompare ) {
    using V = Common::BuildVersion;
    EXPECT_EQ( V( "1" ), V( 1, 0 ) );
    EXPECT_EQ( V( 1, 0 ), V( 1, 0 ) );
    EXPECT_EQ( V( 2, 3, "test" ), V( 2, 3, "test" ) );
    EXPECT_EQ( V( 2, 3, "test.42" ), V( 2, 3, "test.42" ) );
    EXPECT_EQ( V( 2, 3, "test.042" ), V( 2, 3, "test.42" ) );
    EXPECT_EQ( V( 2, 3, "test.042" ), V( 2, 3, "test.0042" ) );

    EXPECT_LT( V( 1, 0 ), V( 2, 0 ) );
    EXPECT_LT( V( 2, 0 ), V( 2, 3 ) );
    EXPECT_LT( V( 2, 9 ), V( 2, 10 ) );
    EXPECT_LT( V( 3, 1, "testa" ), V( 3, 1, "testb" ) );
    EXPECT_LT( V( 3, 1, "test" ), V( 3, 1, "testsome" ) );
    EXPECT_LT( V( 3, 1, "test.6" ), V( 3, 1, "test.7" ) );
    EXPECT_LT( V( 3, 1, "test.9" ), V( 3, 1, "test.10" ) );
    EXPECT_LT( V( 3, 1, "test9some" ), V( 3, 1, "test10some" ) );
    EXPECT_LT( V( 3, 1, "test.09" ), V( 3, 1, "test.10" ) );
    EXPECT_LT( V( 3, 1, "test.09" ), V( 3, 1, "test.0010" ) );
}

TEST( UtilsTestSuite, firmwareVersionCompare ) {
    using V = HubPlatform::Firmware::Version;
    EXPECT_EQ( V( "1" ), V( 1, 0, 0 ) );
    EXPECT_EQ( V( 1, 0, 0 ), V( 1, 0, 0 ) );
    EXPECT_EQ( V( 1, 0, 2 ), V( 1, 0, 2 ) );
    EXPECT_EQ( V( 2, 3, 0, "test" ), V( 2, 3, 0, "test" ) );
    EXPECT_EQ( V( 2, 3, 0, "test.42" ), V( 2, 3, 0, "test.42" ) );
    EXPECT_EQ( V( 2, 3, 0, "test.042" ), V( 2, 3, 0, "test.42" ) );
    EXPECT_EQ( V( 2, 3, 0, "test.042" ), V( 2, 3, 0, "test.0042" ) );

    EXPECT_LT( V( 1, 0, 0 ), V( 2, 0, 0 ) );
    EXPECT_LT( V( 2, 0, 0 ), V( 2, 3, 0 ) );
    EXPECT_LT( V( 2, 9, 0 ), V( 2, 10, 0 ) );
    EXPECT_LT( V( 2, 7, 15 ), V( 2, 7, 16 ) );
    EXPECT_LT( V( 3, 1, 0, "testa" ), V( 3, 1, 0, "testb" ) );
    EXPECT_LT( V( 3, 1, 0, "test" ), V( 3, 1, 0, "testsome" ) );
    EXPECT_LT( V( 3, 1, 0, "53.6" ), V( 3, 1, 0, "53.7" ) );
    EXPECT_LT( V( 3, 1, 0, "test.9" ), V( 3, 1, 0, "test.10" ) );
    EXPECT_LT( V( 3, 1, 0, "test9some" ), V( 3, 1, 0, "test10some" ) );
    EXPECT_LT( V( 3, 1, 0, "test.09" ), V( 3, 1, 0, "test.10" ) );
    EXPECT_LT( V( 3, 1, 0, "test.09" ), V( 3, 1, 0, "test.0010" ) );
}

TEST( UtilsTestSuite, binaryFromHex ) {
    std::string result;
    char message[] = "\x0A\x0B\x0C\x0D";

    // convert from lowercase
    auto hexString =
        hexToString( reinterpret_cast< const unsigned char* >( message + 0 ), sizeof( message ) - 1, false );
    std::cerr << hexString << std::endl;

    ASSERT_TRUE( getBinaryFromHex( hexString, result ) );
    std::cerr << result.size() << std::endl;
    ASSERT_EQ( result, message );

    // convert from uppercase
    hexString = hexToString( reinterpret_cast< const unsigned char* >( message + 0 ), sizeof( message ) - 1, true );
    std::cerr << hexString << std::endl;

    ASSERT_TRUE( getBinaryFromHex( hexString, result ) );
    std::cerr << result.size() << std::endl;
    ASSERT_EQ( result, message );
}

TEST( UtilsTestSuite, binaryFromHexErrors ) {
    std::string result;
    ASSERT_FALSE( getBinaryFromHex( "", result ) );
    ASSERT_TRUE( result.empty() );

    ASSERT_FALSE( getBinaryFromHex( "A", result ) );
    ASSERT_TRUE( result.empty() );
    ASSERT_FALSE( getBinaryFromHex( "FFF", result ) );
    ASSERT_TRUE( result.empty() );
    ASSERT_FALSE( getBinaryFromHex( "ZZ", result ) );
    ASSERT_TRUE( result.empty() );
    ASSERT_FALSE( getBinaryFromHex( "  ", result ) );
    ASSERT_TRUE( result.empty() );
    ASSERT_FALSE( getBinaryFromHex( "gg", result ) );
    ASSERT_TRUE( result.empty() );
    ASSERT_FALSE( getBinaryFromHex( "0A0B0C0", result ) );
    ASSERT_TRUE( result.empty() );
}

}  // namespace UT
}  // namespace HubPlatform
