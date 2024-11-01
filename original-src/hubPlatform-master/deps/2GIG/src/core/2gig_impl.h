#pragma once

#include "2gig/2gig.h"
#include "2gig/devices_repository.h"

#include "core/pipeline/packet_processing_pipeline.h"

#include "utils/thread/thread_safe_wrapper.h"

#include <memory>

namespace TwoGig {

struct ModuleInterface;
class SerialPort;
class PacketProcessingPipeline;
class PipeFactory;
class Logger;
struct Timer;
class DevicesReachabilityManager;

class TwoGigImpl : public TwoGig {
public:
    TwoGigImpl( const std::string& devicePath,
                const std::string& resetPinGpio,
                Client& client,
                DevicesRepository& devicesRepository,
                DevicesReachabilityManager& reachabilityManager,
                InstrumentationClient* instrumentationClient );

    ~TwoGigImpl() override;

    void start() override;
    void stop() override;

    void reset() override;

    void startDeviceDiscovering() override;
    void stopDeviceDiscovering() override;

    void addDevice( const DeviceDescriptor& descriptor ) override;
    void removeDevice( const std::vector< uint32_t >& txIds ) override;

private:
    void switchPipelineMode( PipelineMode mode );

private:
    Client& m_client;
    InstrumentationClient* m_instrumentationClient;
    DevicesRepository& m_devices;
    DevicesReachabilityManager& m_reachabilityManager;

    std::unique_ptr< Timer > m_timer;
    std::unique_ptr< Logger > m_logger;
    std::unique_ptr< ModuleInterface > m_moduleInterface;
    std::unique_ptr< SerialPort > m_port;
    std::unique_ptr< PipeFactory > m_pipeFactory;
    std::unique_ptr< PacketProcessingPipeline > m_pipeline;
};

}  // namespace TwoGig
