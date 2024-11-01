#include "core/packets/packets.h"
#include "core/pipeline/transmission_handler.h"

#include "2gig/device_descriptor.h"

#include "fixtures/pipe_test.h"
#include "fakes/fake_devices_repository.h"
#include "mocks/2gig_client_mock.h"

namespace TwoGig {
namespace Test {

/*

Test plan

Done    1. Repeater updated

*/

struct TransmissionHandlerTest : public testing::Test {
    TransmissionHandlerTest()
    : m_transmissionProducer( m_logger )
    , m_pinCodesProducer( m_logger )
    , m_pipe( m_logger, m_client, m_devices ) {
        m_transmissionProducer.pipe( m_pipe );
        m_pinCodesProducer.pipe( m_pipe );
    }

    bool addDevice( uint32_t txId ) {
        return m_devices.addDevice( { txId } );
    }

    bool addDevice( uint32_t txId, DeviceType::Enum deviceType ) {
        return m_devices.addDevice( { txId }, deviceType );
    }

    RepeaterStatus getRepeaterStatus( uint8_t lowBattery,
                                      uint8_t jamDetected,
                                      uint8_t tamperDetected,
                                      uint8_t acFailed ) {
        RepeaterStatus status;
        status.lowBattery = lowBattery;
        status.jamDetected = jamDetected;
        status.tamperDetected = tamperDetected;
        status.acFailed = acFailed;

        return status;
    }

protected:
    Logger m_logger;
    ClientMock m_client;
    FakeDevicesRepository m_devices;

    DummyPipeProducer< Transmission > m_transmissionProducer;
    DummyPipeProducer< PinCodeTransmission > m_pinCodesProducer;

    TransmissionHandler m_pipe;
};

MATCHER_P( RepeaterStatusMatcher, expected_value, "" ) {
    return arg.acFailed == expected_value.acFailed && arg.jamDetected == expected_value.jamDetected &&
           arg.lowBattery == expected_value.lowBattery && arg.tamperDetected == expected_value.tamperDetected;
}

TEST_F( TransmissionHandlerTest, repeaterUpdated ) {
    addDevice( 1, DeviceType::Repeater );

    Transmission packet = {};
    packet.serial = 1;
    packet.status = 0;

    EXPECT_CALL( m_client, onRepeaterUpdated( 1, RepeaterStatusMatcher( getRepeaterStatus( 0, 0, 0, 0 ) ) ) )
        .Times( testing::Exactly( 1 ) );
    m_transmissionProducer.dispatchPayload( packet );

    packet.status = 128;
    EXPECT_CALL( m_client, onRepeaterUpdated( 1, RepeaterStatusMatcher( getRepeaterStatus( 0, 0, 0, 1 ) ) ) )
        .Times( testing::Exactly( 1 ) );
    m_transmissionProducer.dispatchPayload( packet );

    packet.status = 64;
    EXPECT_CALL( m_client, onRepeaterUpdated( 1, RepeaterStatusMatcher( getRepeaterStatus( 0, 0, 1, 0 ) ) ) )
        .Times( testing::Exactly( 1 ) );
    m_transmissionProducer.dispatchPayload( packet );

    packet.status = 32;
    EXPECT_CALL( m_client, onRepeaterUpdated( 1, RepeaterStatusMatcher( getRepeaterStatus( 0, 1, 0, 0 ) ) ) )
        .Times( testing::Exactly( 1 ) );
    m_transmissionProducer.dispatchPayload( packet );

    packet.status = 8;
    EXPECT_CALL( m_client, onRepeaterUpdated( 1, RepeaterStatusMatcher( getRepeaterStatus( 1, 0, 0, 0 ) ) ) )
        .Times( testing::Exactly( 1 ) );
    m_transmissionProducer.dispatchPayload( packet );

    packet.status = 232;
    EXPECT_CALL( m_client, onRepeaterUpdated( 1, RepeaterStatusMatcher( getRepeaterStatus( 1, 1, 1, 1 ) ) ) )
        .Times( testing::Exactly( 1 ) );
    m_transmissionProducer.dispatchPayload( packet );
}

}  // namespace Test
}  // namespace TwoGig
