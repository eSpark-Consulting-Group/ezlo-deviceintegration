#include <tiny/tiny_consolecolors.h>

#include <utility>
#include "luad/modules/network/network_scan.h"
#include "luad/modules/network/network_utils.h"
#include "log_utils/ezlo_log.h"
#include "service_interfaces/postponed_task.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace LuaNetwork {

namespace ScanEvents {
const char Started[] = "started";
const char Finished[] = "finished";
const char Progress[] = "progress";
}  // namespace ScanEvents

NetworkScan::NetworkScan( Services::Service& service,
                          INetworkScan::Addresses&& addresses,
                          INetworkScan::Ports&& ports,
                          int tries,
                          ResultCallback callback )
: m_isRunning( false )
, m_addresses( std::move( addresses ) )
, m_ports( std::move( ports ) )
, m_tries( tries )
, m_callback( std::move( callback ) )
, m_service( service )
, m_socket( makeNetworkSocket() ) {}

NetworkScan::~NetworkScan() {
    stop();
}

void NetworkScan::scan() {
    if( !m_isRunning ) {
        m_callback( ScanEvents::Started, nullptr );
        m_isRunning = true;
        scanning();
    }
}

void NetworkScan::stop() {
    m_isRunning = false;
}

bool NetworkScan::isScanning() const {
    return m_isRunning;
}

void NetworkScan::scanning() {
    LOGGER_TRACE( "scan" );
    std::function< void( IAddress& ) > pingAddress;
    if( m_ports.empty() ) {
        pingAddress = [this]( IAddress& address ) {
            if( m_tries <= 1 ) {
                auto pingResult = NetworkPing( address ).sendPing();
                if( pingResult == NetworkPing::Result::OK ) {
                    sendResult( address, {}, 1, 0 );
                }
                else {
                    LOGGER_TRACE_RAW( cc::trace( "scanning: scan result: " ) +
                                      cc::num10( static_cast< int >( pingResult ) ) );
                }
            }
            else {
                NetworkPing addressScanner( address );

                unsigned countErrors = 0;
                unsigned index = 0;
                for( ; index < m_tries; index++ ) {
                    auto pingResult = addressScanner.sendPing();
                    if( pingResult != NetworkPing::Result::OK ) {
                        countErrors++;
                        if( countErrors == 1 || countErrors % 5 == 0 ) {
                            LOGGER_TRACE_RAW( cc::trace( "scanning: ping #" ) + cc::num10( index ) +
                                              cc::trace( ", sequence: " ) +
                                              cc::num10( addressScanner.getSequenceCounter() ) +
                                              cc::trace( ", scanning: scan result: " ) +
                                              cc::num10( static_cast< int >( pingResult ) ) +
                                              cc::trace( ", errors: " ) + cc::num10( countErrors ) );
                        }
                    }

                    if( index < m_tries )
                        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                }

                if( countErrors > 0 ) {
                    LOGGER_DEBUG_RAW( cc::debug( "scanning: errors: " ) + cc::num10( countErrors ) +
                                      cc::debug( ", tries: " ) + cc::num10( index ) );
                }
                sendResult( address, {}, index, countErrors );
            }
        };
    }
    else {
        pingAddress = [this]( IAddress& address ) {
            Ports ports;
            for( const auto& port : m_ports ) {
                address.setPort( port );
                auto socketAddress = address.getSocketAddress();
                m_socket->createSocket( AF_INET, SOCK_STREAM, IPPROTO_IP );
                if( m_socket->connect( &socketAddress.address, socketAddress.size ) == 0 ) {
                    ports.push_back( port );
                }
                m_socket->close();
            }
            if( !ports.empty() ) {
                sendResult( address, ports, 1, 0 );
            }
        };
    }
    for( ; m_isRunning == true && !m_addresses->isFinished(); m_addresses->next() ) {
        if( auto currentAddress = m_addresses->getCurrent() ) {
            pingAddress( *currentAddress );
        }
    }
    m_addresses->setFirst();
    m_callback( ScanEvents::Finished, nullptr );
    m_isRunning = false;
}

std::string NetworkScan::getMacAddress( const IAddress& address ) {
    auto interfaceNames = m_networkInterfaces.getInterfaceNames();
    for( const auto& interface : interfaceNames ) {
        LOGGER_TRACE( interface );
        auto mac = LuaNetwork::getMacAddress( address, interface );
        if( !mac.empty() ) {
            return mac;
        }
    }
    return {};
}

void NetworkScan::sendResult( const IAddress& address, const Ports& ports, unsigned tries, unsigned errors ) {
    auto hostName = getHostName( address );
    auto macAddress = getMacAddress( address );
    auto result = std::make_unique< Result >(
        Result( { address.getAddressString(), macAddress, hostName, ports, tries, errors } ) );
    m_callback( ScanEvents::Progress, std::move( result ) );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
