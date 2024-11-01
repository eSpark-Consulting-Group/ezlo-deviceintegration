#include "networkd/gsm/nm_gsm_connection_manager.h"
#include "networkd/gsm/nm_gsm_interface_state_manager.h"
#include "networkd/interfaces/nm_gsm_interface.h"
#include "networkd/resources/nm_resources.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "service_interfaces/task_loop.h"

namespace HubPlatform {
namespace Network {
namespace Mobile {

namespace {
const auto remoteSiteConnectionRestoreTimeout = std::chrono::minutes( 3 );
const auto reviveOperationFailedTimeout = std::chrono::minutes( 3 );
const auto handleConnectionInterruptedFailedTimeout = std::chrono::minutes( 3 );

std::shared_ptr< GSMInterface > lockInterface( const std::weak_ptr< GSMInterface >& m_weakInterface ) {
    auto interface = m_weakInterface.lock();
    if( !interface ) {
        LOGGER_WARN_RAW( cc::warn( "Mobile interface already removed before scheduleRun" ) );
        return nullptr;
    }

    return interface;
}
}  // namespace

ConnectionManager::ConnectionManager( GSMInterface& interface,
                                      InterfaceStateManager& interfaceStateManager,
                                      Services::TaskLoop& taskLoop,
                                      const NetworkConfigFileManager::MobileConfiguration& configuration )
: m_interface( interface )
, m_interfaceStateManager( interfaceStateManager )
, m_taskLoop( taskLoop )
, m_configuration( configuration )
, m_revivalOperations(
      { [this]() { return reRegisterRevivalOperation(); }, [this]() { return rebootModemRevivalOperation(); } } ) {}

void ConnectionManager::start( const std::weak_ptr< GSMInterface >& weakInterface ) {
    m_weakInterface = weakInterface;
}

void ConnectionManager::onConnectionInterrupted( bool probablyFalsePositive ) {
    LOGGER_DEBUG_RAW( cc::debug( "Mobile connection link was interrupted, interface: " ) +
                      cc::note( m_interface.getInterfaceName() ) );

    m_connectionEstablished = false;

    /* give a shot to a remote side to restore a mobile connection */
    if( probablyFalsePositive ) {
        auto weakInterface = m_weakInterface;
        m_taskLoop
            .createPostponedTask( [this, weakInterface]( Services::PostponedTask& ) {
                auto interface = lockInterface( weakInterface );
                if( !interface ) {
                    return;
                }

                handleConnectionInterrupted();
            } )
            ->start( remoteSiteConnectionRestoreTimeout, false );

        return;
    }

    handleConnectionInterrupted();
}

void ConnectionManager::onConnectionRestored() {
    LOGGER_DEBUG_RAW( cc::debug( "Mobile connection link was restored, interface: " ) +
                      cc::note( m_interface.getInterfaceName() ) );

    m_connectionEstablished = true;
    m_revivalOperationIdx = 0;
}

void ConnectionManager::handleConnectionInterrupted() {
    try {
        m_interfaceStateManager.registerToNetwork();

        if( !m_connectionEstablished ) {
            LOGGER_INFO_RAW( cc::info( "Mobile connection link was lost, signal quality: " ) +
                             cc::note( m_interface.getInstantSignalQuality().dump() ) + cc::info( ", interface: " ) +
                             cc::note( m_interface.getInterfaceName() ) );

            m_interface.onRegistrationInterrupted();

            auto weakInterface = m_weakInterface;
            m_taskLoop
                .createPostponedTask( [this, weakInterface]( Services::PostponedTask& ) {
                    auto interface = lockInterface( weakInterface );
                    if( !interface ) {
                        return;
                    }
                    auto result = tryRevive();
                    if( result.first ) {
                        m_interfaceStateManager.scheduleRun( result.second );
                    }
                } )
                ->start( m_configuration.m_connectionRevivalAttemptTimeout, false );
        }

        return;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle mobile connection interruption, interface: " ) +
                          cc::note( m_interface.getInterfaceName() ) + cc::error( ", reason: " ) +
                          cc::note( ex.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to handle mobile connection interruption, interface: " ) +
                          cc::note( m_interface.getInterfaceName() ) );
    }

    m_interfaceStateManager.scheduleRun( handleConnectionInterruptedFailedTimeout );
}

ConnectionManager::RevivalOperationResult ConnectionManager::tryRevive() {
    bool schedule = true;
    std::chrono::seconds scheduleTimeout = reviveOperationFailedTimeout;

    auto revivalOperationIdx = m_revivalOperationIdx;

    try {
        std::tie( schedule, scheduleTimeout ) = m_revivalOperations[ m_revivalOperationIdx++ ]();
        if( m_revivalOperationIdx >= m_revivalOperations.size() ) {
            m_revivalOperationIdx = 0;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to revive a mobile connection, interface: " ) +
                          cc::note( m_interface.getInterfaceName() ) + cc::error( ", revival operation idx: " ) +
                          cc::note( std::to_string( revivalOperationIdx ) ) + cc::error( ", reason: " ) +
                          cc::note( ex.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to revive a mobile connection, interface: " ) +
                          cc::note( m_interface.getInterfaceName() ) + cc::error( ", revival operation idx: " ) +
                          cc::note( std::to_string( revivalOperationIdx ) ) );
    }

    return { schedule, scheduleTimeout };
}

ConnectionManager::RevivalOperationResult ConnectionManager::reRegisterRevivalOperation() {
    LOGGER_DEBUG_RAW(
        cc::debug( "Try to revive mobile connection by performing network re-registration, interface: " ) +
        cc::note( m_interface.getInterfaceName() ) );

    m_interfaceStateManager.reRegister( true );
    return { true, std::chrono::minutes( 3 ) };
}

ConnectionManager::RevivalOperationResult ConnectionManager::rebootModemRevivalOperation() {
    LOGGER_DEBUG_RAW( cc::debug( "Try to revive mobile connection by performing modem reboot, interface: " ) +
                      cc::note( m_interface.getInterfaceName() ) );

    m_interfaceStateManager.rebootModem();
    return { false, std::chrono::seconds( 0 ) };
}

}  // namespace Mobile
}  // namespace Network
}  // namespace HubPlatform
