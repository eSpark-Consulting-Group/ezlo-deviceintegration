#pragma once

#include "core/pipeline/packet_collector.h"
#include "core/pipeline/packet_validator.h"
#include "core/pipeline/packet_sorter.h"
#include "core/pipeline/packet_converter.h"
#include "core/pipeline/packet_duplication_filter.h"
#include "core/pipeline/device_filter.h"
#include "core/pipeline/device_collector.h"
#include "core/pipeline/pin_code_filter.h"
#include "core/pipeline/pin_code_controller.h"
#include "core/pipeline/transmission_handler.h"
#include "core/pipeline/reachability_handler.h"

#include "utils/serial_port/serial_port_reader.h"

namespace TwoGig {

class PipeFactory;
struct Transmission;

enum class PipelineMode { PacketProcessing, DeviceDiscovering };

class PacketProcessingPipeline {
public:
    PacketProcessingPipeline( const PipeFactory& pipeFactory );

    PipelineMode getMode() const;
    void setMode( PipelineMode mode );

    void start();
    void stop();

private:
    void initializePipeline();

    void reset();

private:
    std::unique_ptr< SerialPortReader > m_portReader;
    std::unique_ptr< PacketCollector > m_packetCollector;
    std::unique_ptr< PacketValidator > m_packetValidator;
    std::unique_ptr< PacketSorter > m_packetSorter;
    std::unique_ptr< PacketConverter< Transmission > > m_packetToTransmissionConverter;
    std::unique_ptr< PacketDuplicationFilter > m_packetDuplicationFilter;
    std::unique_ptr< DeviceFilter > m_deviceFilter;
    std::unique_ptr< DeviceCollector > m_deviceCollector;
    std::unique_ptr< PinCodeFilter > m_pinCodeFilter;
    std::unique_ptr< PinCodeController > m_pinCodeController;
    std::unique_ptr< TransmissionHandler > m_transmissionHandler;
    std::unique_ptr< ReachabilityHandler > m_reachabilityHandler;

    PipelineMode m_mode;
};

}  // namespace TwoGig
