#pragma once

#include "2gig/device_status.h"
#include "2gig/devices_repository.h"

#include "core/packets/pin_code_transmission.h"
#include "utils/pipeline/pipe_consumer.h"

namespace TwoGig {

struct Transmission;
struct Client;
class Logger;

class TransmissionHandler : public PipeConsumer< Transmission >, public PipeConsumer< PinCodeTransmission > {
public:
    TransmissionHandler( const Logger& logger, Client& client, const DevicesRepository& devices );

    void processPayload( Transmission packet ) override;

    void processPayload( PinCodeTransmission packet ) override;

private:
    DeviceType::Enum getDeviceType( Transmission packet ) const;

    template < typename TransmissionT >
    void printTransmissionInfo( TransmissionT&& transmission, const char* message );

private:
    const Logger& m_logger;
    Client& m_client;
    const DevicesRepository& m_devices;
};

}  // namespace TwoGig
