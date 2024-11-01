#include "crypto_utils/digests.h"
#include "tiny/tiny_consolecolors.h"
#include <gtest/gtest.h>

/*

TEST PLAN

1. md5: block
2. sha1: blocks and stream
3. sha256: block and stream
4 All Digests
    4.1 Calculate sha1 by name
    4.2 Calculate sha224 by name
    4.3 Calculate sha256 by name
    4.4 Calculate sha384 by name
    4.5 Calculate sha512 by name
    4.6 Calculate dss1 by name
    4.7 Calculate ripemd160 by name
5. All Digests, errors
    5.1 Error: unknown algorithm
    5.2 Error: emptyAlgorithm
6. Service functions
    6.1 toHex
    6.2 supportedDigests

*/

namespace HubPlatform {
namespace UT {

class DigestsTests : public ::testing::Test {
public:
    static void checkDigestName( const std::string& digestName,
                                 const std::string& data,
                                 const std::string& expectedDigest,
                                 const char* file,
                                 int line );
};

void DigestsTests::checkDigestName( const std::string& digestName,
                                    const std::string& data,
                                    const std::string& expectedDigest,
                                    const char* file,
                                    int line ) {
    std::string digest;
    std::string errorAt = std::string{ " failed at: " } + file + ":" + std::to_string( line );
    {
        EXPECT_TRUE( CryptoUtils::Digests::getInstance().calculateDigest( digestName, data, digest ) )
            << digestName << errorAt;
        EXPECT_EQ( digest, expectedDigest ) << digestName << ", from string" << errorAt;
    }
    {
        std::stringstream ss;
        ss << data;
        EXPECT_TRUE( CryptoUtils::Digests::getInstance().calculateDigest( digestName, ss, digest ) )
            << digestName << ", from string" << errorAt;
        EXPECT_EQ( digest, expectedDigest ) << digestName << ", from stream" << errorAt;
    }
}

namespace Resources {
const char TestMessage[] = "This is a test message to make digests from. Good luck!";
}

TEST_F( DigestsTests, md5 ) /* 1 */ {
    std::string data = Resources::TestMessage;
    auto result = CryptoUtils::Digests::md5( data );
    std::string expected = "bf27e2246034b80d5a78d0c9d88e74d4";

    ASSERT_EQ( result, expected ) << "from string";

    std::stringstream ss;
    ss << data;
    result = CryptoUtils::Digests::md5( ss );
    EXPECT_EQ( result, expected ) << "from stream";
}

TEST_F( DigestsTests, sha1 ) /* 2 */ {
    std::string data = Resources::TestMessage;
    auto digest = CryptoUtils::Digests::sha1( data );
    const char* expectedDigest = "e827e43c232a35b5bd723e9a57f362ca86106582";
    EXPECT_EQ( digest, expectedDigest ) << "from string";

    std::stringstream ss;
    ss << data;
    digest = CryptoUtils::Digests::sha1( ss );
    EXPECT_EQ( digest, expectedDigest ) << "from stream";
}

TEST_F( DigestsTests, sha256 ) /* 3 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "ea17a90d6ac4c5d9bf13b683c2f7a1e24925245e5be15ca5e2598b4c60e1e739";

    std::string digest;
    {
        digest = CryptoUtils::Digests::sha256( data );
        EXPECT_EQ( digest, expectedDigest ) << "from string";
    }
    {
        std::stringstream ss;
        ss << data;
        digest = CryptoUtils::Digests::sha256( ss );
        EXPECT_EQ( digest, expectedDigest ) << "from stream";
    }
}

TEST_F( DigestsTests, allDigest_sha1 ) /* 4.1 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "e827e43c232a35b5bd723e9a57f362ca86106582";
    const char* digestName = "sha1";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigest_sha224 ) /* 4.2 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "287d8698e767f68cd66d6e6bfffba140321513163d8e0332d2d1687d";
    const char* digestName = "sha224";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigest_sha256 ) /* 4.3 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "ea17a90d6ac4c5d9bf13b683c2f7a1e24925245e5be15ca5e2598b4c60e1e739";
    const char* digestName = "sha256";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigest_sha384 ) /* 4.4 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest =
        "77a8e255f1770ee229c89bd11b2c095320fc79c3af2eb5e7715dffc8190309e5b7b5322f4d874aeea3534cbd061e9c28";
    const char* digestName = "sha384";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigest_sha512 ) /* 4.5 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "83d872eb64348dc5da130130909422ca4d3310e28aa1744cd5ae2370f530fcb247e47ad15b66d5892c3c5"
                                 "d44e1c3dadb718cc7f75659e97681d6f08b3a9278ad";
    const char* digestName = "sha512";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigest_dss1 ) /* 4.6 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "e827e43c232a35b5bd723e9a57f362ca86106582";
    const char* digestName = "dss1";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigest_ripemd160 ) /* 4.7 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "43cbf043ddc598668f0f6121fc9c72b7b7c6a44c";
    const char* digestName = "ripemd160";
    checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
}

TEST_F( DigestsTests, allDigestErrors_unknownAlgorithm ) /* 5.1 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "a84699b4313109ab100d193ffe63da61a372323b";
    const char* digestName = "unknownAlgorithm";
    try {
        checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
        FAIL() << "Should throw: unknown algorithm";
    }
    catch( const std::exception& e ) {
        SUCCEED();
    }
}

TEST_F( DigestsTests, allDigestErrors_emptyAlgorithm ) /* 5.2 */ {
    std::string data = Resources::TestMessage;
    const char* expectedDigest = "a84699b4313109ab100d193ffe63da61a372323b";
    const char* digestName = "";
    try {
        checkDigestName( digestName, data, expectedDigest, __FILE__, __LINE__ );
        FAIL() << "Should throw: unknown algorithm";
    }
    catch( const std::exception& e ) {
        SUCCEED();
    }
}

TEST_F( DigestsTests, toHex ) /* 6.1 */ {
    char message[] = "This is a message";

    auto hexString = CryptoUtils::toHex( std::string{ message } );
    auto hexBuffer = CryptoUtils::toHex( reinterpret_cast< unsigned char* >( message ), sizeof( message ) - 1 );
    ASSERT_EQ( hexString, hexBuffer );
}

TEST_F( DigestsTests, supportedDigests ) /* 6.2 */ {
    auto& digests = CryptoUtils::Digests::getInstance();
    auto supportedDigestNames = digests.supportedDigests();

    ASSERT_FALSE( supportedDigestNames.empty() );

    std::vector< std::string > shouldHaveNames = { "md4", "md5", "sha1", "sha256", "sha512", "dss1", "ripemd160" };
    for( auto name : shouldHaveNames ) {
        EXPECT_EQ( supportedDigestNames.count( "md5" ), 1 );
    }
}

}  // namespace UT
}  // namespace HubPlatform