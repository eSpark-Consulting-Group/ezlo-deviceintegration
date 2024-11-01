#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "networkd/gsm/nm_at_executor.h"
#include "common_utils/cpp14.h"

/*

TESTPLAN

DONE    1. ATUtils tests
DONE        1.1 toStrings
DONE        1.2 isOK
DONE        1.3 isValidICCID
DONE        1.4 extractATParameter
*/

namespace HubPlatform {
namespace UT {

using testing::_;
using testing::Return;

using namespace HubPlatform::Network::GSM;

class ATUtilsTest : public ::testing::Test {};

TEST_F( ATUtilsTest, toStrings /*1.1*/ ) {
    using Result = std::vector< std::string >;

    EXPECT_EQ( ATUtils::toStrings( "\r \nline\r\n \r\nOK\r\n" ), Result( { "line", "OK" } ) );
    EXPECT_EQ( ATUtils::toStrings( "\r\nOK\r\n" ), Result( { "OK" } ) );
    EXPECT_EQ( ATUtils::toStrings( "line\nOK" ), Result( { "line", "OK" } ) );
    EXPECT_EQ( ATUtils::toStrings( "OK" ), Result( { "OK" } ) );

    EXPECT_EQ( ATUtils::toStrings( "\r\n \r\n" ), Result( {} ) );
    EXPECT_EQ( ATUtils::toStrings( "" ), Result( {} ) );
}

TEST_F( ATUtilsTest, isOK /*1.2*/ ) {
    EXPECT_TRUE( ATUtils::isOK( { "text", "OK" }, 2 ) );
    EXPECT_TRUE( ATUtils::isOK( { "OK" }, 1 ) );

    EXPECT_FALSE( ATUtils::isOK( { "text", "OK" }, 1 ) );
    EXPECT_FALSE( ATUtils::isOK( { "NOTOK" }, 1 ) );
    EXPECT_FALSE( ATUtils::isOK( { "ERROR" }, 1 ) );
    EXPECT_FALSE( ATUtils::isOK( { "OK" }, 0 ) );
    EXPECT_FALSE( ATUtils::isOK( { "" }, 1 ) );
    EXPECT_FALSE( ATUtils::isOK( {}, 1 ) );
}

TEST_F( ATUtilsTest, isValidICCID /*1.3*/ ) {
    // Should be a string of hexadecimal symbols starting from prefix "89"
    ASSERT_TRUE( ATUtils::isValidICCID( "890123456789" ) );

    ASSERT_FALSE( ATUtils::isValidICCID( "89abcdefgh" ) );
    ASSERT_FALSE( ATUtils::isValidICCID( "abcdefgh" ) );
    ASSERT_FALSE( ATUtils::isValidICCID( "890123456789\r\n\n\n something else" ) );
}

TEST_F( ATUtilsTest, extractATParameter /*1.4*/ ) {
    auto atCommand = "EADY\n\n\n\n+QUSIM: 1\n\n\n\n+QIND: SMS DONE\n\n\n\n+QIND: PB "
                     "DONE\n\n\r\n+QCCID: 8901260882281059342F\n\n\r\nOK\r\n";

    auto pair = ATUtils::AtResponse::extractATParameter( atCommand, "QCCID" );
    ASSERT_TRUE( pair.second );
    ASSERT_EQ( pair.first, "8901260882281059342F" ) << "should have a valid CCID value";
}

}  // namespace UT
}  // namespace HubPlatform
