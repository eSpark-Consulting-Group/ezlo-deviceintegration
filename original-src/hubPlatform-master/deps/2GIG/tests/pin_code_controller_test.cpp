#include "core/pipeline/pin_code_controller.h"
#include "core/packets/packets.h"

#include "fixtures/pipe_test.h"
#include "fakes/fake_devices_repository.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Single device
Done    2. Multiple devices
Done    3. Custom pin length
Done    4. Length decreased in the middle of input
Done    5. Length increased in the middle of input

*/

struct PinCodeControllerTest : public PipeTest< PinCodeController > {
    PinCodeControllerTest()
    : PipeTest< PinCodeController >( m_devices ) {}

    bool isPinCodeReady( uint8_t txId, PadButton::Enum input ) {
        Transmission packet = { 0 };
        packet.serial = txId;
        new( &packet.status )
            PadStatus{ .keypad = true, .supervisory = false, .lowBattery = 0, .buttonPressed = input };

        pushPayload( packet );

        auto payload = pullPayload();
        if( payload->pinState == PinCodeState::Valid ) {
            m_pinCodes[ txId ] = payload->pin;
            return true;
        }

        return false;
    }

    std::string getPinCode( uint8_t txId ) const {
        auto it = m_pinCodes.find( txId );
        return it != m_pinCodes.end() ? it->second : std::string{};
    }

    void updatePinCodeLength( uint32_t txId, uint8_t length ) {
        m_devices.setPinCodeLength( txId, length );
    }

private:
    FakeDevicesRepository m_devices;

    std::map< uint8_t, std::string > m_pinCodes;
};

TEST_F( PinCodeControllerTest, singleDeviceTest ) {
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Asterisk ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_0 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Pound ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_1 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_2 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_3 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_4 ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Num_5 ) );

    ASSERT_EQ( getPinCode( 1 ), "*0#12345" );
}

TEST_F( PinCodeControllerTest, multipleDevicesTest ) {
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Asterisk ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_0 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Pound ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_1 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_2 ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_3 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_4 ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_5 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_6 ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_7 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_8 ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_9 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Asterisk ) );
    ASSERT_FALSE( isPinCodeReady( 2, PadButton::Num_0 ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Pound ) );
    ASSERT_TRUE( isPinCodeReady( 2, PadButton::Num_1 ) );

    ASSERT_EQ( getPinCode( 1 ), "*#2468*#" );
    ASSERT_EQ( getPinCode( 2 ), "01357901" );
}

TEST_F( PinCodeControllerTest, customPinLengthTest ) {
    updatePinCodeLength( 1, 4 );

    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Asterisk ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_0 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Pound ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Num_1 ) );

    ASSERT_EQ( getPinCode( 1 ), "*0#1" );
}

TEST_F( PinCodeControllerTest, customPinLengthLoweredInTheMiddleTest ) {
    updatePinCodeLength( 1, 4 );

    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Asterisk ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_0 ) );
    updatePinCodeLength( 1, 2 );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Pound ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Num_1 ) );
    ASSERT_EQ( getPinCode( 1 ), "*0#1" );

    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_2 ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Num_3 ) );
    ASSERT_EQ( getPinCode( 1 ), "23" );
}

TEST_F( PinCodeControllerTest, customPinLengthRaisedInTheMiddleTest ) {
    updatePinCodeLength( 1, 4 );

    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Asterisk ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_0 ) );
    updatePinCodeLength( 1, 6 );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Pound ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Num_1 ) );
    ASSERT_EQ( getPinCode( 1 ), "*0#1" );

    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_2 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_3 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_4 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_5 ) );
    ASSERT_FALSE( isPinCodeReady( 1, PadButton::Num_6 ) );
    ASSERT_TRUE( isPinCodeReady( 1, PadButton::Num_7 ) );
    ASSERT_EQ( getPinCode( 1 ), "234567" );
}

}  // namespace Test
}  // namespace TwoGig
