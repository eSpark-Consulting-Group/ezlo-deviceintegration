#include "core/2gig_impl.h"
#include "core/module_interface.h"
#include "core/pipeline/pipe_factory.h"

#include "2gig/client.h"
#include "2gig/device_descriptor.h"
#include "2gig/devices_reachability_manager.h"

#include "utils/logger/logger.h"
#include "utils/timer/timer_impl.h"
#include "utils/cpp14.h"

namespace TwoGig {

std::unique_ptr< TwoGig > createInstance( const std::string& devicePath,
                                          const std::string& resetPinGpio,
                                          Client& client,
                                          DevicesRepository& devicesRepository,
                                          DevicesReachabilityManager& reachabilityManager,
                                          InstrumentationClient* instrumentationClient ) {
    return std::make_unique< TwoGigImpl >(
        devicePath, resetPinGpio, client, devicesRepository, reachabilityManager, instrumentationClient );
}

TwoGigImpl::TwoGigImpl( const std::string& devicePath,
                        const std::string& resetPinGpio,
                        Client& client,
                        DevicesRepository& devicesRepository,
                        DevicesReachabilityManager& reachabilityManager,
                        InstrumentationClient* instrumentationClient )
: m_client( client )
, m_instrumentationClient( instrumentationClient )
, m_devices( devicesRepository )
, m_reachabilityManager( reachabilityManager )
, m_timer( std::make_unique< TimerImpl >() )
, m_logger( std::make_unique< Logger >( m_instrumentationClient ) )
, m_moduleInterface( std::make_unique< ModuleInterface >( devicePath, resetPinGpio ) )
, m_port( m_moduleInterface->createSerialPort() )
, m_pipeFactory(
      std::make_unique< PipeFactory >( *m_logger, *m_timer, m_devices, *m_port, m_client, m_reachabilityManager ) )
, m_pipeline( std::make_unique< PacketProcessingPipeline >( *m_pipeFactory ) ) {}

TwoGigImpl::~TwoGigImpl() {
    m_moduleInterface->disable();
}

void TwoGigImpl::start() {
    m_moduleInterface->reenable();

    m_pipeline->start();
}

void TwoGigImpl::stop() {
    m_pipeline->stop();

    m_moduleInterface->disable();
}

void TwoGigImpl::reset() {
    m_devices.reset();
    m_client.onReset();
}

void TwoGigImpl::startDeviceDiscovering() {
    switchPipelineMode( PipelineMode::DeviceDiscovering );
}

void TwoGigImpl::stopDeviceDiscovering() {
    switchPipelineMode( PipelineMode::PacketProcessing );
}

void TwoGigImpl::addDevice( const DeviceDescriptor& descriptor ) {
    const bool result = m_devices.addDevice( descriptor.getTxIds(), descriptor.getDeviceType() );
    if( result ) {
        m_reachabilityManager.onDeviceAdded( descriptor.getTxIds() );
        m_client.onDeviceAdded( descriptor );
    }
}

void TwoGigImpl::removeDevice( const std::vector< uint32_t >& txIds ) {
    const bool result = m_devices.removeDevice( txIds );
    if( result ) {
        m_reachabilityManager.onDeviceRemoved( txIds );
        m_client.onDeviceRemoved( txIds );
    }
}

void TwoGigImpl::switchPipelineMode( PipelineMode mode ) {
    if( m_pipeline->getMode() != mode ) {
        m_pipeline->stop();
        m_pipeline->setMode( mode );
        m_pipeline->start();
    }
}

}  // namespace TwoGig
