#include "core/pipeline/packet_processing_pipeline.h"
#include "core/pipeline/pipe_factory.h"

namespace TwoGig {

PacketProcessingPipeline::PacketProcessingPipeline( const PipeFactory& pipeFactory )
: m_portReader( pipeFactory.createSerialPortReader() )
, m_packetCollector( pipeFactory.createPacketCollector() )
, m_packetValidator( pipeFactory.createPacketValidator() )
, m_packetSorter( pipeFactory.createPacketSorter() )
, m_packetToTransmissionConverter( pipeFactory.createPacketConverter< Transmission >() )
, m_packetDuplicationFilter( pipeFactory.createPacketDuplicationFilter() )
, m_deviceFilter( pipeFactory.createDeviceFilter() )
, m_deviceCollector( pipeFactory.createDeviceCollector() )
, m_pinCodeFilter( pipeFactory.createPinCodeFilter() )
, m_pinCodeController( pipeFactory.createPinCodeController() )
, m_transmissionHandler( pipeFactory.createTransmissionHandler() )
, m_reachabilityHandler( pipeFactory.createReachabilityHandler() )
, m_mode( PipelineMode::PacketProcessing ) {
    initializePipeline();
}

PipelineMode PacketProcessingPipeline::getMode() const {
    return m_mode;
}

void PacketProcessingPipeline::setMode( PipelineMode mode ) {
    m_mode = mode;

    switch( m_mode ) {
    case PipelineMode::PacketProcessing:
        m_deviceFilter->resetPipe( DeviceFilter::PipeChannel::UnknownDevice );
        break;

    case PipelineMode::DeviceDiscovering:
        m_deviceFilter->pipe( *m_deviceCollector, DeviceFilter::PipeChannel::UnknownDevice );
        break;

    default:
        break;
    }
}

void PacketProcessingPipeline::start() {
    reset();

    m_portReader->startReading();
}

void PacketProcessingPipeline::stop() {
    m_portReader->stopReading();
}

void PacketProcessingPipeline::initializePipeline() {
    ( *m_portReader )
        .pipe( *m_packetCollector )
        .pipe( *m_packetValidator )
        .pipe( *m_packetSorter )
        .pipe( *m_packetToTransmissionConverter, PacketSorter::PipeChannel::Transmission )
        .pipe( *m_packetDuplicationFilter )
        .pipe( *m_deviceFilter )
        .pipe( *m_reachabilityHandler, DeviceFilter::PipeChannel::KnownDevice )
        .pipe( *m_pinCodeFilter )
        .pipe( *m_transmissionHandler, PinCodeFilter::PipeChannel::StatusUpdate );

    ( *m_pinCodeFilter )
        .pipe( *m_pinCodeController, PinCodeFilter::PipeChannel::PinCodeInput )
        .pipe( *m_transmissionHandler );
}

void PacketProcessingPipeline::reset() {
    m_packetCollector->reset();
    m_packetDuplicationFilter->reset();
    m_deviceCollector->reset();
    m_pinCodeController->reset();
}

}  // namespace TwoGig
