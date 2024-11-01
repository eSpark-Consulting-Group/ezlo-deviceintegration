#include "networkd/connection_service/nm_wifi_connection_service.h"
#include "networkd/connection_service/nm_connection_defs.h"
#include "networkd/resources/nm_resources.h"

#include "jsonrpc/jsonrpc.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"

#include <string.h>
#include <cassert>
#include <mongoose.h>
#include <unistd.h>
#include <fcntl.h>

namespace HubPlatform {
namespace Network {

namespace {
constexpr const char getListURI[] = "/wifi/networks";
constexpr const char getStatusURI[] = "/wifi/status";
constexpr const char postConnectURI[] = "/wifi/network/connect";
constexpr const char postCloseURI[] = "/wifi/network/close";
constexpr const char postDisconnectURI[] = "/wifi/disconnect";
constexpr const char postRestartURI[] = "/restart";

template < typename... Args >
std::string stringWithFormat( const std::string& format, Args... args ) {
    size_t size = snprintf( nullptr, 0, format.c_str(), args... ) + 1;
    std::unique_ptr< char[] > buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args... );
    return std::string( buf.get(), buf.get() + size - 1 );
}
}  // namespace

WifiConnectionService::WifiConnectionService( ConnectionDelegate& connectionDelegate,
                                              std::string webRoot,
                                              std::string ports )
: m_connectionDelegate( connectionDelegate )
, m_webRoot( std::move( webRoot ) )
, m_ports( std::move( ports ) )
, m_status( ConnectionStatus::None )
, m_lastConnectionResult( 0 ) {}

WifiConnectionService::~WifiConnectionService() {
    stopImpl();
}

bool WifiConnectionService::isStarted() const noexcept {
    return m_context != nullptr && m_enabled;
}

void WifiConnectionService::stop() noexcept {
    if( !m_enabled ) {
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Stopping HTTP server" ) );

    m_enabled = false;
#ifdef __HUB_TYPE_G150__
    return;
#endif
    stopImpl();
}

void WifiConnectionService::start() noexcept {
    if( m_enabled ) {
        return;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Starting HTTP server" ) );

    m_enabled = true;
#ifdef __HUB_TYPE_G150__
    if( m_context != nullptr ) {
        return;
    }
#endif
    startImpl();
}

void WifiConnectionService::stopImpl() noexcept {
    if( !m_context ) {
        return;
    }

    // NOTE: Stopping mongoose messes up stderr (we use it in logger),
    //       so need to backup stderr and restore it after stopping mongoose
    // NOTE: For some reason this workaround works only on H2
    fflush( stderr );
    auto stderrBackup = dup( STDERR_FILENO );
    int devnull = open( "/dev/null", O_WRONLY );
    dup2( devnull, STDERR_FILENO );
    close( devnull );

    mg_stop( m_context );

    fflush( stderr );
    dup2( stderrBackup, STDERR_FILENO );
    close( stderrBackup );

    m_context = nullptr;
}

void WifiConnectionService::startImpl() noexcept {
    if( m_context ) {
        return;
    }

    m_context = mg_start();

    if( mg_set_option( m_context, "root", m_webRoot.c_str() ) != 1 ) {
        Log::warn( "MG: Failed to set web root " + m_webRoot );
        stop();
        return;
    }

    if( mg_set_option( m_context, "ports", m_ports.c_str() ) != 1 ) {
        Log::warn( "MG: Failed to set port " + m_ports );
        stop();
        return;
    }

    mg_set_uri_callback( m_context, "/*", WifiConnectionService::handleDataRequest, this );
}

void WifiConnectionService::send200JsonAnswer( mg_connection* connection, int contentLength ) {
    LOGGER_TRACE_RAW( cc::trace( "WifiConnectionService::send200JsonAnswer" ) );
    mg_printf( connection, JsonResponseHeader, "200 Ok" );
    mg_printf( connection, "Content-Type: application/json\r\nContent-Length: %d\r\n\r\n", contentLength );
}

void WifiConnectionService::send400BadRequestAnswer( mg_connection* connection ) {
    LOGGER_TRACE_RAW( cc::trace( "WifiConnectionService::send400BadRequestAnswer" ) );
    mg_printf( connection, JsonResponseHeader, BadRequest );
    mg_printf( connection, "Connection: close\r\n\r\n" );
}

void WifiConnectionService::send404NotFoundAnswer( mg_connection* connection ) {
    LOGGER_TRACE_RAW( cc::trace( "WifiConnectionService::send404NotFoundAnswer" ) );
    mg_printf( connection, JsonResponseHeader, NotFound );
    mg_printf( connection, "Connection: close\r\n\r\n" );
}

void WifiConnectionService::send500InternalErrorAnswer( mg_connection* connection ) {
    LOGGER_TRACE_RAW( cc::trace( "WifiConnectionService::send500InternalErrorAnswer" ) );
    mg_printf( connection, JsonResponseHeader, InternalError );
    mg_printf( connection, "Connection: close\r\n\r\n" );
}

void WifiConnectionService::handleGetListRequest( mg_connection* connection ) {
    LOGGER_TRACE_RAW( cc::trace( "Got request to scan for wireless networks via REST API" ) );
    std::string response;
    if( m_connectionDelegate.scanWifi( response ) ) {
        response += "\r\n\r\n";
        send200JsonAnswer( connection, response.size() );
        mg_write( connection, response.c_str(), response.size() );
    }
    else {
        send500InternalErrorAnswer( connection );
    }
}

void WifiConnectionService::handleGetStatusRequest( mg_connection* connection ) {
    LOGGER_TRACE_RAW( cc::trace( "Got request for wireless network status via REST API" ) );

    std::string response =
        stringWithFormat( JsonAnswerSuccessStatusPattern,
                          m_connectionDelegate.isWifiInternetAvailable() ? "connected" : "not connected",
                          HubInfoHelper::getSerial().c_str(),
                          FirmwareInfoHelper::getFirmwareVersionString().c_str(),
                          HubInfoHelper::getMacAddress().c_str(),
                          HubInfoHelper::getControllerUUID().c_str() );

    send200JsonAnswer( connection, response.size() );
    mg_write( connection, response.c_str(), response.size() );
}

void WifiConnectionService::handleConnectionRequest( mg_connection* connection, const std::string& postData ) {
    JsonObject object;
    if( !parseJson( postData, object ) ) {
        send400BadRequestAnswer( connection );
        return;
    }

    auto encryption = json_get_str( object, EncrypyionKey );
    if( encryption.empty() ) {
        encryption = Resources::DefaultApEncryption;
    }
    auto password = json_get_str( object, PassKey );
    auto ssid = json_get_str( object, SsidKey );
    auto bssid = json_get_str( object, BssidKey );

    m_status = ConnectionStatus::InProgress;

    Log::info( cc::info( "Got request to connect to " ) + cc::note( ssid ) +
               cc::info( " wireless network via REST API" ) );

    m_connectionDelegate.connect(
        std::move( ssid ), std::move( bssid ), std::move( encryption ), std::move( password ) );

    send200JsonAnswer( connection, strlen( JsonConnectSuccessAnswer ) );
    mg_write( connection, JsonConnectSuccessAnswer, strlen( JsonConnectSuccessAnswer ) );
}

void WifiConnectionService::handleUndefinedRequest( mg_connection* connection ) {
    send400BadRequestAnswer( connection );
}

void WifiConnectionService::handleCloseRequest( mg_connection* connection ) {
    Log::info( cc::info( "Got request to temporary shutdown AP via REST API" ) );

    m_connectionDelegate.temporaryShutDownAP();

    std::string response = "{}\r\n";
    send200JsonAnswer( connection, response.size() );
    mg_write( connection, response.c_str(), response.size() );

    stop();
}

void WifiConnectionService::handleRestartRequest( mg_connection* connection ) {
    Log::info( cc::info( "Got request to reset wireless network settings via REST API" ) );
    m_connectionDelegate.restart();

    std::string response = "{}\r\n";
    send200JsonAnswer( connection, response.size() );
    mg_write( connection, response.c_str(), response.size() );
}

void WifiConnectionService::handleDataRequest( mg_connection* connection,
                                               const mg_request_info* requestInfo,
                                               void* data ) {
    auto service = reinterpret_cast< WifiConnectionService* >( data );
    if( !service->m_enabled ) {
        return;
    }

    LOGGER_TRACE_RAW( cc::trace( "WifiConnectionService::handleDataRequest URI: " ) + cc::c( requestInfo->uri ) );

    if( const auto uriString = requestInfo->uri ) {
        if( strcmp( requestInfo->request_method, "GET" ) == 0 ) {
            if( strstr( uriString, getListURI ) != nullptr ) {
                return service->handleGetListRequest( connection );
            }
            else if( strstr( uriString, getStatusURI ) != nullptr ) {
                return service->handleGetStatusRequest( connection );
            }
        }
        else if( strcmp( requestInfo->request_method, "POST" ) == 0 ) {
            if( strstr( uriString, postConnectURI ) != nullptr ) {
                const auto postData = requestInfo->post_data;
                const auto postDataLength = requestInfo->post_data_len;
                return service->handleConnectionRequest( connection,
                                                         std::string( postData, postData + postDataLength ) );
            }
            else if( strstr( uriString, postCloseURI ) != nullptr ) {
                return service->handleCloseRequest( connection );
            }
            else if( strstr( uriString, postDisconnectURI ) != nullptr ) {
                return service->handleRestartRequest( connection );
            }
            else if( strstr( uriString, postRestartURI ) != nullptr ) {
                return service->handleRestartRequest( connection );
            }
        }

        return service->handleUndefinedRequest( connection );
    }
}

}  // namespace Network
}  // namespace HubPlatform
