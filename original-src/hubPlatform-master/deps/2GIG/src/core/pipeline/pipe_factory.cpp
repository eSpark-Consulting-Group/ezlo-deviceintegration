#include "core/pipeline/pipe_factory.h"

#include "core/pipeline/packet_collector.h"
#include "core/pipeline/packet_validator.h"
#include "core/pipeline/packet_sorter.h"
#include "core/pipeline/packet_converter.h"
#include "core/pipeline/device_filter.h"
#include "core/pipeline/device_collector.h"
#include "core/pipeline/packet_duplication_filter.h"
#include "core/pipeline/pin_code_filter.h"
#include "core/pipeline/pin_code_controller.h"
#include "core/pipeline/transmission_handler.h"
#include "core/pipeline/reachability_handler.h"

#include "utils/serial_port/serial_port_reader.h"
#include "utils/cpp14.h"

namespace TwoGig {

PipeFactory::PipeFactory( const Logger& logger,
                          const Timer& timer,
                          const DevicesRepository& devices,
                          SerialPort& port,
                          Client& client,
                          DevicesReachabilityManager& reachabilityManager )
: m_logger( logger )
, m_timer( timer )
, m_devices( devices )
, m_port( port )
, m_client( client )
, m_reachabilityManager( reachabilityManager ) {}

std::unique_ptr< SerialPortReader > PipeFactory::createSerialPortReader() const {
    return std::make_unique< SerialPortReader >( m_logger, m_port );
}

std::unique_ptr< PacketCollector > PipeFactory::createPacketCollector() const {
    return std::make_unique< PacketCollector >( m_logger );
}

std::unique_ptr< PacketValidator > PipeFactory::createPacketValidator() const {
    return std::make_unique< PacketValidator >( m_logger );
}

std::unique_ptr< PacketSorter > PipeFactory::createPacketSorter() const {
    return std::make_unique< PacketSorter >( m_logger );
}

template < typename To >
std::unique_ptr< PacketConverter< To > > PipeFactory::createPacketConverter() const {
    return std::make_unique< PacketConverter< Transmission > >( m_logger );
}

std::unique_ptr< DeviceFilter > PipeFactory::createDeviceFilter() const {
    return std::make_unique< DeviceFilter >( m_logger, m_devices );
}

std::unique_ptr< DeviceCollector > PipeFactory::createDeviceCollector() const {
    return std::make_unique< DeviceCollector >( m_logger, m_client );
}

std::unique_ptr< PacketDuplicationFilter > PipeFactory::createPacketDuplicationFilter() const {
    return std::make_unique< PacketDuplicationFilter >( m_logger, m_timer );
}

std::unique_ptr< PinCodeFilter > PipeFactory::createPinCodeFilter() const {
    return std::make_unique< PinCodeFilter >( m_logger );
}

std::unique_ptr< PinCodeController > PipeFactory::createPinCodeController() const {
    return std::make_unique< PinCodeController >( m_logger, m_devices );
}

std::unique_ptr< TransmissionHandler > PipeFactory::createTransmissionHandler() const {
    return std::make_unique< TransmissionHandler >( m_logger, m_client, m_devices );
}

std::unique_ptr< ReachabilityHandler > PipeFactory::createReachabilityHandler() const {
    return std::make_unique< ReachabilityHandler >( m_logger, m_reachabilityManager );
}

template std::unique_ptr< PacketConverter< Transmission > > PipeFactory::createPacketConverter< Transmission >() const;

}  // namespace TwoGig
