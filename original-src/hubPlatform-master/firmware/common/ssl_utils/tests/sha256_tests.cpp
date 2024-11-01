#include "ssl_utils/sha256.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1 SHA256 hashes

 */

using namespace ::testing;
using namespace HubPlatform::SslUtils;

class Sha256TestSuite : public Test {};

TEST_F( Sha256TestSuite, testSha256Hashes /*1*/ ) {
    EXPECT_EQ( sha256( "" ), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" );
    EXPECT_EQ( sha256( "test" ), "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08" );
    EXPECT_EQ( sha256( "ABCDEFJHIJKLMNOPQRSTUVWKYZabcdefghijklmnopqrstuvwxyz0123456789" ),
               "9a8e7c3b950d6a56e6affba21a181de88548cc7ea1094517e749bb53ce70969c" );
}
