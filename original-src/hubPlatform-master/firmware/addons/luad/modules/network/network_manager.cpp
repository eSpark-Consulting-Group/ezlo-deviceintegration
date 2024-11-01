#include "luad/modules/network/network_manager.h"
#include "luad/modules/network/async_socket.h"
#include "luad/modules/network/events_emitter.h"
#include "luad/modules/network/dhcp/dhcp_listener.h"
#include "luad/modules/network/dhcp/subscriptions.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace LuaNetwork {

NetworkManager::NetworkManager( Services::Service& service, std::unique_ptr< EventsEmitter >&& eventsEmitter )
: NetworkManager( service, std::move( eventsEmitter ), makeEpollEventsLoop() ) {}

NetworkManager::NetworkManager( Services::Service& service,
                                std::unique_ptr< EventsEmitter >&& eventsEmitter,
                                std::unique_ptr< IEventsLoop > eventsLoop )
: m_service( service )
, m_eventsEmitter( std::move( eventsEmitter ) )
, m_eventsLoop( std::move( eventsLoop ) ) {}

NetworkManager::~NetworkManager() = default;

int NetworkManager::connect( const std::string& groupName, const RemoteAddress& remoteAddress, int options ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto asyncSocketHandle = asyncSocketsClient.makeAsyncSocket( *m_eventsLoop, *m_eventsEmitter, remoteAddress );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( asyncSocketHandle );

    asyncSocket->connect( options, asyncSocket );
    asyncSocket->connected( asyncSocket );

    return asyncSocketHandle;
}

std::size_t NetworkManager::send( const std::string& groupName, int connectionHandle, const std::string& data ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->send( data );
}

std::size_t NetworkManager::sendto( const std::string& groupName,
                                    int connectionHandle,
                                    const std::string& data,
                                    const Host& address,
                                    const Port& port ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->sendto( data, address, port );
}

std::string NetworkManager::receive( const std::string& groupName, int connectionHandle, std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->receive( amount );
}

DataAddressPort NetworkManager::receiveFrom( const std::string& groupName, int connectionHandle, std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->receiveFrom( amount );
}

AddressPort NetworkManager::getsockname( const std::string& groupName, int connectionHandle ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->getsockname();
}

void NetworkManager::processOpenedSockets( const std::string& groupName, SocketsProcessCall call ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    asyncSocketsClient.processOpenedSockets( std::move( call ) );
}

void NetworkManager::request( const std::string& groupName,
                              int connectionHandle,
                              std::string callback,
                              std::string userData,
                              std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    asyncSocket->request( std::move( callback ), std::move( userData ), amount );
}

void NetworkManager::requestFrom( const std::string& groupName,
                                  int connectionHandle,
                                  std::string callback,
                                  std::string userData,
                                  std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    asyncSocket->requestFrom( std::move( callback ), std::move( userData ), amount );
}

std::string NetworkManager::inspectData( const std::string& groupName, int connectionHandle, std::size_t amount ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->inspectData( amount );
}

std::size_t NetworkManager::inspectDataSize( const std::string& groupName, int connectionHandle ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    return asyncSocket->inspectDataSize();
}

void NetworkManager::close( const std::string& groupName, int connectionHandle ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );

    asyncSocketsClient.removeAsyncSocket( connectionHandle );
}

void NetworkManager::setEventsHandler( const std::string& groupName, const std::string& eventHandler ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    asyncSocketsClient.setEventsHandler( eventHandler );
}

void NetworkManager::setEventsHandler( const std::string& groupName,
                                       const std::string& eventHandler,
                                       int connectionHandle ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );

    asyncSocket->setEventsHandler( eventHandler );
}

NetworkScanClient::ScanHandle NetworkManager::startScanning( const std::string& groupName,
                                                             NetworkScanClient::AddressVersion addressFormat,
                                                             INetworkScan::Addresses&& addresses,
                                                             INetworkScan::Ports&& ports,
                                                             const std::string& resultHandler,
                                                             int tries ) {
    std::lock_guard< std::mutex > lock( m_networkScanManagerMtx );

    NetworkScanClient::ResultCallback resultCallback = [this,
                                                        resultHandler]( const NetworkScanClient::Result& result ) {
        EventsEmitter::ScanningOptionResult optionResult;
        if( result.scanResult ) {
            NetworkScan::Result& scanResult = *result.scanResult;
            optionResult = std::make_unique< EventsEmitter::ScanningResult >(
                EventsEmitter::ScanningResult( { scanResult.ipAddress,
                                                 scanResult.macAddress,
                                                 scanResult.hostName,
                                                 scanResult.ports,
                                                 scanResult.tries,
                                                 scanResult.errors } ) );
        }
        m_eventsEmitter->emitScanningResult(
            resultHandler, result.scanHandle, result.status, std::move( optionResult ) );
    };

    auto& networkScanClient = m_networkScanManager.makeScannerClient( groupName );
    auto scanHandle = networkScanClient.makeScanner(
        m_service, addressFormat, std::move( addresses ), std::move( ports ), tries, resultCallback );
    networkScanClient.startScanner( scanHandle );
    return scanHandle;
}

void NetworkManager::stopScanning( const std::string& groupName, NetworkScanClient::ScanHandle handle ) {
    std::lock_guard< std::mutex > lock( m_networkScanManagerMtx );

    auto& networkScanClient = m_networkScanManager.makeScannerClient( groupName );
    networkScanClient.stopScanner( handle );
}

NetworkManager::DhcpHandle NetworkManager::startListeningDhcp( const std::string& groupName,
                                                               const std::string& resultHandler,
                                                               const std::vector< std::string >& operations ) {
    using namespace Dhcp;
    using Result = DhcpPacketUnpack::AllHeadersPtr;
    using ResultCallback = std::function< void( Result && ) >;

    ResultCallback resultCallback = [this]( Result&& headersPtr ) -> void {
        if( headersPtr != nullptr && this->m_subscriptions && this->m_subscriptions->size() > 0 ) {
            const Dhcp::Subscriptions& db = *this->m_subscriptions;
            size_t clientsNotified = db.notify( *headersPtr );
            if( clientsNotified )
                LOGGER_TRACE_RAW( cc::trace( "Clients notified: " ) + cc::num10( clientsNotified ) );
        }
    };
    NotifyCallback notifyCallback =
        [this]( const std::string& path, const std::string& eventType, const Json& response ) -> void {
        LOGGER_DEBUG_RAW( cc::warn( "notifyCallback called: type: " ) + eventType + std::string( ", event: " ) +
                          cc::j( response ) );
        if( this->m_eventsEmitter ) {
            m_eventsEmitter->emitDhcpEvent( path, eventType.empty() ? "dhcp" : eventType, response );
        }
    };

    using namespace HubPlatform::LuaNetwork;
    if( m_subscriptions == nullptr ) {
        m_subscriptions = std::make_unique< Dhcp::Subscriptions >( std::move( notifyCallback ) );
    }

    if( m_dhcpListener == nullptr ) {
        m_dhcpListener = Dhcp::makeListener( std::move( resultCallback ) );
    }

    if( m_subscriptions->size() == 0 ) {
        // run listener here
        if( !m_dhcpListener->isScanning() )
            m_dhcpListener->scan();
    }
    DhcpHandle handle = m_subscriptions->subscribe( Subscription( resultHandler, operations ) );
    return handle;
}

void NetworkManager::stopListeningDhcp( const std::string& groupName, DhcpHandle subscriptionId ) {
    if( m_subscriptions == nullptr )
        return;

    m_subscriptions->unsubscribe( subscriptionId );

    if( m_subscriptions->size() == 0 ) {
        // Stop watching network events
        if( m_dhcpListener && m_dhcpListener->isScanning() ) {
            m_dhcpListener->stop();
        }
    }
}

int NetworkManager::createUdp( const std::string& groupName ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    RemoteAddress remoteAddress{ "", "", ConnectionType::UDP };
    auto asyncSocketHandle = asyncSocketsClient.makeAsyncSocket( *m_eventsLoop, *m_eventsEmitter, remoteAddress );

    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( asyncSocketHandle );
    asyncSocket->connected( asyncSocket );

    return asyncSocketHandle;
}

void NetworkManager::setPeerName( const std::string& groupName,
                                  int connectionHandle,
                                  const Host& address,
                                  const Port& port ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );

    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
    asyncSocket->setRemoteAddress( address, port );
    asyncSocket->connect( 0, asyncSocket );
}

void NetworkManager::setSockName( const std::string& groupName,
                                  int connectionHandle,
                                  const Host& address,
                                  const Port& port ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );

    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
    asyncSocket->bind( address, port, asyncSocket );
}

void NetworkManager::setOption( const std::string& groupName,
                                int connectionHandle,
                                const std::string& option,
                                const JsonObject& optionValue ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );

    if( option == "broadcast" ) {
        int value = 1;
        auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
        auto res = asyncSocket->setopt( SOL_SOCKET, SO_BROADCAST, value );
        if( res != 0 ) {
            throw AsyncSocketError( "fail to set option: " + std::to_string( errno ) );
        }
    }
    else if( option == "ip-add-membership" ) {
        if( optionValue.empty() && !optionValue.is_object() && optionValue.count( "multi_addr" ) ) {
            throw AsyncSocketError( "fail to set option: " + option + ", no multicast IP provided" );
        }

        auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
        auto res = asyncSocket->setopt( IPPROTO_IP, IP_ADD_MEMBERSHIP, optionValue );
        if( res != 0 ) {
            throw AsyncSocketError( "fail to set option: " + std::to_string( errno ) );
        }
    }
    else if( option == "ip-drop-membership" ) {
        if( optionValue.empty() && !optionValue.is_object() && optionValue.count( "multi_addr" ) ) {
            throw AsyncSocketError( "fail to set option: " + option + ", no multicast IP provided" );
        }
        auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
        auto res = asyncSocket->setopt( IPPROTO_IP, IP_DROP_MEMBERSHIP, optionValue );
        if( res != 0 ) {
            throw AsyncSocketError( "fail to set option: " + std::to_string( errno ) );
        }
    }
    else if( option == "ip-multicast-ttl" ) {
        int value;
        if( !optionValue.empty() && optionValue.is_number() ) {
            value = optionValue.get< int >();
        }
        else {
            throw AsyncSocketError( "fail to set option: " + option + ", no ttl value provided" );
        }
        auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
        auto res = asyncSocket->setopt( IPPROTO_IP, IP_MULTICAST_TTL, value );
        if( res != 0 ) {
            throw AsyncSocketError( "fail to set option: " + std::to_string( errno ) );
        }
    }
    else {
        throw AsyncSocketError( "unknown option" );
    }
}

void NetworkManager::setTimeout( const std::string& groupName,
                                 int connectionHandle,
                                 std::chrono::milliseconds timeoutMs ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );

    auto& asyncSocketsClient = m_asyncSocketsManager.makeAsyncSocketsClient( groupName );
    auto& asyncSocket = asyncSocketsClient.getAsyncSocket( connectionHandle );
    auto sndTimeoutHandler = asyncSocket->setTimeout( SOL_SOCKET, SO_SNDTIMEO, timeoutMs );
    auto recvTimeoutHandler = asyncSocket->setTimeout( SOL_SOCKET, SO_RCVTIMEO, timeoutMs );
    if( sndTimeoutHandler != 0 ) {
        throw AsyncSocketError( "fail to set socket sending timeout: " + std::to_string( errno ) );
    }
    else if( recvTimeoutHandler != 0 ) {
        throw AsyncSocketError( "fail to set socket receiving timeout: " + std::to_string( errno ) );
    }
}

void NetworkManager::reset( const std::string& groupName ) {
    std::lock_guard< std::mutex > lock( m_asyncSocketsManagerMtx );
    m_asyncSocketsManager.removeAsyncSocketsClient( groupName );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
