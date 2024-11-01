#include "base_utils/string_utils.h"
#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. StringUtils tests
DONE        1.0 strip
DONE        1.1 split
DONE        1.2 split by delimiter
DONE        1.3 splitSequence
DONE        1.4 splitByLines
DONE        1.5 splitByStrippedLinesWithText
DONE        1.6 count tokens
DONE        1.7 replace tokenss
*/

namespace HubPlatform {
namespace UT {

TEST( StringUtilsTestSuite, strip /*1.0*/ ) {
    EXPECT_EQ( StringUtils::strip( "\n\r\t\v text \t\v\r\n" ), "text" );
    EXPECT_EQ( StringUtils::strip( "\n\r \t\vtext" ), "text" );
    EXPECT_EQ( StringUtils::strip( "text\t\v\r \n" ), "text" );
    EXPECT_EQ( StringUtils::strip( "text" ), "text" );
    EXPECT_EQ( StringUtils::strip( "" ), "" );
}

TEST( StringUtilsTestSuite, split /*1.1*/ ) {
    using Result = std::vector< std::string >;

    EXPECT_EQ( StringUtils::split( " text1 text2 \t\t  text3 \r\n text4" ),
               Result( { "text1", "text2", "text3", "text4" } ) );
    EXPECT_EQ( StringUtils::split( "" ), Result( {} ) );
}

TEST( StringUtilsTestSuite, splitByDelim /*1.2*/ ) {
    using Result = std::vector< std::string >;

    EXPECT_EQ( StringUtils::split( "text1,text2", "," ), Result( { "text1", "text2" } ) );
    EXPECT_EQ( StringUtils::split( "text1__text2__text3", "__" ), Result( { "text1", "text2", "text3" } ) );
    EXPECT_EQ( StringUtils::split( "text__", { "__" } ), Result( { "text", "" } ) );
    EXPECT_EQ( StringUtils::split( "__text", { "__" } ), Result( { "", "text" } ) );
    EXPECT_EQ( StringUtils::split( "__", { "__" } ), Result( { "", "" } ) );
    EXPECT_EQ( StringUtils::split( "text", { "" } ), Result( { "text" } ) );
    EXPECT_EQ( StringUtils::split( "text", { "__" } ), Result( { "text" } ) );

    EXPECT_EQ( StringUtils::split( "", { "__" } ), Result( {} ) );
}

TEST( StringUtilsTestSuite, splitSequence /*1.3*/ ) {
    using Result = std::vector< std::string >;

    EXPECT_EQ( StringUtils::splitSequence( "text1,text2", { "," } ), Result( { "text1", "text2" } ) );
    EXPECT_EQ( StringUtils::splitSequence( "text1|A|text2|B|text3", { "|A|", "|B|" } ),
               Result( { "text1", "text2", "text3" } ) );
    EXPECT_EQ( StringUtils::splitSequence( "text__", { "__" } ), Result( { "text", "" } ) );
    EXPECT_EQ( StringUtils::splitSequence( "__text", { "__" } ), Result( { "", "text" } ) );
    EXPECT_EQ( StringUtils::splitSequence( "__", { "__" } ), Result( { "", "" } ) );
    EXPECT_EQ( StringUtils::splitSequence( "text", { "" } ), Result( { "text" } ) );

    EXPECT_EQ( StringUtils::splitSequence( "text1|A|text2|_|text3", { "|A|", "|B|" } ), Result( {} ) );
    EXPECT_EQ( StringUtils::splitSequence( "text", { "__" } ), Result( {} ) );
    EXPECT_EQ( StringUtils::splitSequence( "", { "__" } ), Result( {} ) );
}

TEST( StringUtilsTestSuite, splitByLines /*1.4*/ ) {
    using Result = std::vector< std::string >;

    EXPECT_EQ( StringUtils::splitByLines( "1\n2\r\n \r\n3\r\n" ), Result( { "1", "2\r", " \r", "3\r" } ) );
    EXPECT_EQ( StringUtils::splitByLines( "" ), Result( {} ) );
}

TEST( StringUtilsTestSuite, splitByStrippedLinesWithText /*1.5*/ ) {
    using Result = std::vector< std::string >;

    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "\r \nline\r\n \r\nOK\r\n" ), Result( { "line", "OK" } ) );
    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "\nline1\n\nline2\n" ), Result( { "line1", "line2" } ) );
    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "\r\nOK\r\n" ), Result( { "OK" } ) );
    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "line\nOK" ), Result( { "line", "OK" } ) );
    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "OK" ), Result( { "OK" } ) );

    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "\r\n \r\n" ), Result( {} ) );
    EXPECT_EQ( StringUtils::splitByStrippedLinesWithText( "" ), Result( {} ) );
}

TEST( StringUtilsTestSuite, countTokens /*1.6*/ ) {
    EXPECT_EQ( StringUtils::countTokens( "foo.bar.foobar foobar", "foo" ), 3 );
    EXPECT_EQ( StringUtils::countTokens( "bar", "foo" ), 0 );
    EXPECT_EQ( StringUtils::countTokens( "1", "foo" ), 0 );
    EXPECT_EQ( StringUtils::countTokens( "", "foo" ), 0 );

    EXPECT_EQ( StringUtils::countTokens( "", "" ), 1 );
    EXPECT_EQ( StringUtils::countTokens( "nonempty", "" ), 0 );
}

TEST( StringUtilsTestSuite, replaceTokens /*1.7*/ ) {
    {  // Replace tokens in different positions
        std::string str = "<TOKEN><TOKEN>.content1.<TOKEN><TOKEN>.content2.<TOKEN><TOKEN>";
        EXPECT_EQ( StringUtils::replaceTokens( str, "<TOKEN>", "value" ), 6 );
        EXPECT_EQ( str, "valuevalue.content1.valuevalue.content2.valuevalue" );
    }

    {  // Replace tokens with empty string
        std::string str = "<TOKEN><TOKEN>.content1.<TOKEN><TOKEN>.content2.<TOKEN><TOKEN>";
        EXPECT_EQ( StringUtils::replaceTokens( str, "<TOKEN>", "" ), 6 );
        EXPECT_EQ( str, ".content1..content2." );
    }

    {  // Replace tokens in empty string
        std::string str = "";
        EXPECT_EQ( StringUtils::replaceTokens( str, "<TOKEN>", "value" ), 0 );
        EXPECT_EQ( str, "" );
    }
}

}  // namespace UT
}  // namespace HubPlatform
