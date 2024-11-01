#include "zwave/common/auth_mode_utils.h"

#include "zwave/common/zwave_exceptions.h"
#include "zwave/resources/zwave_resources.h"

#include <znet/znet.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*

TESTPLAN

DONE    1. Key list to modes
DONE        1.1 Empty
DONE        1.2 Not empty
DONE    2. Modes to key list
DONE        2.1 Empty
DONE        2.2 Not empty
DONE        2.3 Unsupported mode
DONE    3. Check auth mode
DONE        3.1 Supported
DONE        3.2 Unsupported

*/

namespace HubPlatform {
namespace Zwave {
namespace UT {

using namespace ::testing;

class AuthModeUtilsTestSuite : public Test {};

TEST_F( AuthModeUtilsTestSuite, testKeyListToModesEmpty /*1.1*/ ) {
    constexpr uint8_t emptyKeyList{ 0 };

    const auto modes = AuthModeUtils::keyListToModes( emptyKeyList );

    EXPECT_TRUE( modes.empty() );
}

TEST_F( AuthModeUtilsTestSuite, testKeyListToModesNotEmpty /*1.2*/ ) {
    uint8_t keyList{ 0 };
    keyList |= ZNET_SECURITY2_S2_CLASS_0;

    const auto modes = AuthModeUtils::keyListToModes( keyList );

    EXPECT_EQ( modes.size(), 1 );
    EXPECT_EQ( modes[ 0 ], Resources::AuthModes::Unauthenticated );
}

TEST_F( AuthModeUtilsTestSuite, testModesToKeyListEmpty /*2.1*/ ) {
    const AuthModeUtils::Modes modes;

    const auto keyList = AuthModeUtils::modesToKeyList( modes );

    EXPECT_EQ( keyList, 0 /*emptyKeyList*/ );
}

TEST_F( AuthModeUtilsTestSuite, testModesToKeyListNotEmpty /*2.2*/ ) {
    const AuthModeUtils::Modes modes{ Resources::AuthModes::Unauthenticated, Resources::AuthModes::ClientSide };

    const auto keyList = AuthModeUtils::modesToKeyList( modes );

    EXPECT_TRUE( keyList & ZNET_SECURITY2_S2_CLASS_0 );
    EXPECT_TRUE( keyList & ZNET_SECURITY2_S2_CSA );
}

TEST_F( AuthModeUtilsTestSuite, testModesToKeyListUnsupportedMode /*2.3*/ ) {
    const AuthModeUtils::Modes modes{ "UnsupportedMode" };

    EXPECT_THROW( AuthModeUtils::modesToKeyList( modes ), AuthModeError );
}

TEST_F( AuthModeUtilsTestSuite, testCheckAuthModeSupported /*3.1*/ ) {
    EXPECT_NO_THROW( AuthModeUtils::checkAuthMode( Resources::AuthModes::AccessControl ) );
    EXPECT_NO_THROW( AuthModeUtils::checkAuthMode( Resources::AuthModes::Authenticated ) );
    EXPECT_NO_THROW( AuthModeUtils::checkAuthMode( Resources::AuthModes::Unauthenticated ) );
    EXPECT_NO_THROW( AuthModeUtils::checkAuthMode( Resources::AuthModes::S0 ) );
    EXPECT_NO_THROW( AuthModeUtils::checkAuthMode( Resources::AuthModes::ClientSide ) );
}

TEST_F( AuthModeUtilsTestSuite, testCheckAuthModeUnsupported /*3.2*/ ) {
    EXPECT_THROW( AuthModeUtils::checkAuthMode( "UnsupportedMode" ), AuthModeError );
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
