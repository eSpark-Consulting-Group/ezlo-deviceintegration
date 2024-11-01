#include "opkg_utils/keys/key_file.h"

#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1 Conversion from string
DONE        1.1 Correct file format
DONE        1.2 Empty file
DONE        1.3 Incorrect prefix
DONE        1.4 Empty signature
DONE        1.5 No key
DONE        1.6 Empty key
DONE        1.7 Extra lines
DONE    2 Conversion to string
DONE        2.1 Correct KeyFile
DONE        2.2 Incorrect KeyFile

*/

using namespace ::testing;
using namespace HubPlatform::Opkg;

class KeyFileTestSuite : public Test {
public:
    KeyFileTestSuite() {
        // Do not print errors and warnings on erroneous test cases
        // TODO: implement fake log and check error messages too.
        Log::setLocalLogLevel( Log::FATAL );
    }
};

TEST_F( KeyFileTestSuite, testFromStringCorrectFileFormat /*1.1*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "untrusted comment: eZLO Innovation OpenWrt signature\n"
                          "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_TRUE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, fingerprint );
    EXPECT_EQ( keyFile.comment, "eZLO Innovation OpenWrt signature" );
    EXPECT_EQ( keyFile.key, "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ" );
}

TEST_F( KeyFileTestSuite, testFromStringEmptyFile /*1.2*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_FALSE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "" );
    EXPECT_EQ( keyFile.comment, "" );
    EXPECT_EQ( keyFile.key, "" );
}

TEST_F( KeyFileTestSuite, testFromStringIncorrectPrefix /*1.3*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "trusted comment: eZLO Innovation OpenWrt signature\n"
                          "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_FALSE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "" );
    EXPECT_EQ( keyFile.comment, "" );
    EXPECT_EQ( keyFile.key, "" );
}

TEST_F( KeyFileTestSuite, testFromStringEmptySignature /*1.4*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "untrusted comment: \n"
                          "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_FALSE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "" );
    EXPECT_EQ( keyFile.comment, "" );
    EXPECT_EQ( keyFile.key, "" );
}

TEST_F( KeyFileTestSuite, testFromStringNoKey /*1.5*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "untrusted comment: eZLO Innovation OpenWrt signature";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_FALSE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "" );
    EXPECT_EQ( keyFile.comment, "" );
    EXPECT_EQ( keyFile.key, "" );
}

TEST_F( KeyFileTestSuite, testFromStringEmptyKey /*1.6*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "untrusted comment: eZLO Innovation OpenWrt signature\n";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_FALSE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "" );
    EXPECT_EQ( keyFile.comment, "" );
    EXPECT_EQ( keyFile.key, "" );
}

TEST_F( KeyFileTestSuite, testFromStringExtraLines /*1.7*/ ) {
    std::string fingerprint = "1f73e8b83819ae5";
    std::string content = "untrusted comment: eZLO Innovation OpenWrt signature\n"
                          "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ\n"
                          "RWQQNaxzzE5Z41cVmEh2rilAPKLsyfPKm+S4BJWA1Yv+LP1hKebmGtXi";

    auto keyFile = KeyFile::fromString( fingerprint, content );

    EXPECT_FALSE( keyFile.isValid() );
    EXPECT_EQ( keyFile.fingerprint, "" );
    EXPECT_EQ( keyFile.comment, "" );
    EXPECT_EQ( keyFile.key, "" );
}

TEST_F( KeyFileTestSuite, testToStringCorrectFileFormat /*1.1*/ ) {
    KeyFile keyFile = { "1f73e8b83819ae5",
                        "eZLO Innovation OpenWrt signature",
                        "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ" };

    EXPECT_EQ( KeyFile::toString( keyFile ),
               "untrusted comment: eZLO Innovation OpenWrt signature\n"
               "RWQB9z6Lg4Ga5QzcBzVINgMgvKwj4INe2JWc3iLIFFapvkBqBwOPCpnQ" );
}

TEST_F( KeyFileTestSuite, testToStringIncorrectFileFormat /*1.2*/ ) {
    KeyFile keyFile = {};

    EXPECT_EQ( KeyFile::toString( keyFile ), "" );
}
