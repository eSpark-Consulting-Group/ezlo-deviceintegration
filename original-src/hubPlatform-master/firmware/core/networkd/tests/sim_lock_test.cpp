/*

TESTPLAN

DONE        1.1 Prohibit lock when lock is on already
DONE        1.2 Prohibit lock when incorrect sim pin code is provided
DONE        1.3 Prohibit unlock when lock is off already
DONE        1.4 Prohibit unlock when incorrect sim pin code is provided
DONE        1.5 Prohibit pin enter when lock is off
DONE        1.6 Prohibit pin enter when incorrect sim pin code is provided
DONE        1.7 Prohibit pin enter when sim pin code isn't required
DONE        1.8 Prohibit pin change when lock is off
DONE        1.9 Prohibit pin change when incorrect sim pin code is provided
DONE        1.10 Prohibit pin reset when lock is off
DONE        1.11 Prohibit pin reset when incorrect sim pin code is provided
DONE        1.12 Prohibit pin reset when sim puk code isn't required
DONE        1.13 Prohibit pin reset when a last attempt to put a sim puk code is left
DONE        1.14 Lock is successful
DONE        1.15 Unlock is successful
DONE        1.16 Pin enter is successful
DONE        1.17 Pin change is successful
DONE        1.18 Pin reset is successful
DONE        1.19 isPinCodeRequired test
DONE        1.20 isLocked test

*/

#include "networkd/gsm/ec25/nm_ec25_sim_lock.h"
#include "networkd/tests/fakes/fake_at_executor.h"

#include <gtest/gtest.h>

#define MAKE_STRING_WITH_NULL_SYMBOLS( name, data ) std::string name( data, sizeof( data ) );

namespace HubPlatform {
namespace Network {
namespace GSM {
namespace UT {

TEST( SIMLockTestSuite, prohibitLockWhenLockIsOn ) /*1.1*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.lock( "1234" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitLockWhenIncorrectPinCode ) /*1.2*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.lock( "" ), GSM::GSMModemInvalidPINCode );
}

TEST( SIMLockTestSuite, prohibitUnlockWhenLockIsOff ) /*1.3*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.unlock( "1234" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitUnlockWhenIncorrectPinCode ) /*1.4*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.unlock( "" ), GSM::GSMModemInvalidPINCode );
}

TEST( SIMLockTestSuite, prohibitPinEnterWhenLockIsOff ) /*1.5*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.enterPinCode( "1234" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitPinEnterWhenIncorrectPinCode ) /*1.6*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.enterPinCode( "" ), GSM::GSMModemInvalidPINCode );
}

TEST( SIMLockTestSuite, prohibitPinEnterWhenPinCodeIsNotRequired ) /*1.7*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: READY" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.enterPinCode( "1234" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitPinChangeWhenLockIsOff ) /*1.8*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.changePinCode( "1234", "4321" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitPinChangeWhenIncorrectPinCode ) /*1.9*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.changePinCode( "", "4321" ), GSM::GSMModemInvalidPINCode );
    ASSERT_THROW( simLock.changePinCode( "1234", "" ), GSM::GSMModemInvalidPINCode );
}

TEST( SIMLockTestSuite, prohibitPinResetWhenLockIsOff ) /*1.10*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.resetPinCode( "1234", "4321" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitPinResetWhenIncorrectPinCode ) /*1.11*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.resetPinCode( "", "4321" ), GSM::GSMModemInvalidPINCode );
    ASSERT_THROW( simLock.resetPinCode( "1234", "" ), GSM::GSMModemInvalidPINCode );
}

TEST( SIMLockTestSuite, prohibitPinResetWhenPukCodeIsNotRequired ) /*1.12*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: READY" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.resetPinCode( "1234", "4321" ), GSM::GSMOperationNotAllowed );
}

TEST( SIMLockTestSuite, prohibitPinResetWhenLastAttemptToEnterPukCode ) /*1.13*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: SIM PUK" },
        { "at+qpinc?\r", "+QPINC: \"SC\",3,1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    ASSERT_THROW( simLock.resetPinCode( "1234", "4321" ), GSM::GSMModemPUKCodeLastAttempt );
}

TEST( SIMLockTestSuite, lockSuccessful ) /*1.14*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_NO_THROW( simLock.lock( "1234" ) );
}

TEST( SIMLockTestSuite, unlockSuccessful ) /*1.15*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_NO_THROW( simLock.unlock( "1234" ) );
}

TEST( SIMLockTestSuite, pinEnterSuccessful ) /*1.16*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: SIM PIN" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_NO_THROW( simLock.enterPinCode( "1234" ) );
}

TEST( SIMLockTestSuite, pinChangeSuccessful ) /*1.17*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: READY" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_NO_THROW( simLock.changePinCode( "1234", "4321" ) );

    commandReply = FakeATExecutor::CommandReplyMap{
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: SIM PIN" },
    };

    fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock1 = SIMLock( fakeATExecutor );
    EXPECT_NO_THROW( simLock1.changePinCode( "1234", "4321" ) );
}

TEST( SIMLockTestSuite, pinResetSuccessful ) /*1.18*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
        { "at+cpin?\r", "+CPIN: SIM PUK" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_NO_THROW( simLock.resetPinCode( "1234", "4321" ) );
}

TEST( SIMLockTestSuite, isPinCodeRequiredTest ) /*1.19*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+cpin?\r", "+CPIN: SIM PIN" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_TRUE( simLock.isPinCodeRequired() );

    commandReply = FakeATExecutor::CommandReplyMap{
        { "at+cpin?\r", "+CPIN: READY" },
    };

    fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock1 = SIMLock( fakeATExecutor );
    EXPECT_FALSE( simLock1.isPinCodeRequired() );
}

TEST( SIMLockTestSuite, isLockedTest ) /*1.20*/ {
    FakeATExecutor::CommandReplyMap commandReply = {
        { "at+clck=\"SC\",2\r", "+CLCK: 1" },
    };

    auto fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock = SIMLock( fakeATExecutor );
    EXPECT_TRUE( simLock.isLocked() );

    commandReply = FakeATExecutor::CommandReplyMap{
        { "at+clck=\"SC\",2\r", "+CLCK: 0" },
    };

    fakeATExecutor = FakeATExecutor( commandReply );
    auto simLock1 = SIMLock( fakeATExecutor );
    EXPECT_FALSE( simLock1.isLocked() );
}

}  // namespace UT
}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
