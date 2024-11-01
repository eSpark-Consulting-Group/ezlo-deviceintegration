#include "cloud_devices_tests/tests/fixtures/test_controller_server.h"

#include "system_utils/environment.h"
#include "database_client/dbid.h"

#include <cassert>

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

TestControllerServer::TestControllerServer()
: m_port( 0 )
, m_connected( false ) {}

std::string TestControllerServer::getAddress() const {
    return "127.0.0.1:" + std::to_string( m_port );
}

void TestControllerServer::start( const std::string& crt, const std::string& key, const std::string& ca ) {
    tiny::ws::security_args securityArgs;
    if( !crt.empty() ) {
        securityArgs.strCertificateFile_ = Environment::getFirmwareRootFolder() + "/certs/" + crt;
    }
    if( !key.empty() ) {
        securityArgs.strPrivateKeyFile_ = Environment::getFirmwareRootFolder() + "/certs/" + key;
    }
    if( !ca.empty() ) {
        securityArgs.strCertificationChainFile_ = Environment::getFirmwareRootFolder() + "/certs/" + ca;
    }

    static std::string serverName = "Test controller server";
    for( std::size_t idx = 1; idx <= 3; ++idx ) {
        Log::info( cc::debug( "Set up " ) + cc::info( serverName ) + cc::debug( " server, attempt " ) +
                   cc::num10( idx ) );
        m_server = std::make_unique< TestServer >(
            [this]( rpc::Message& peerMessage ) {
                auto jsonMessage = peerMessage.getMessage();

                std::string messageId;
                if( !json_get( jsonMessage, "id", messageId ) ) {
                    throw std::runtime_error( std::string( "Unknown message format: " ) + cc::j( jsonMessage ) );
                }

                if( messageId == "ui_broadcast" ) {
                    std::unique_lock< std::mutex > lock( m_mutex );
                    m_broadcastMessages.push_back( jsonMessage );
                    lock.unlock();
                    m_condVar.notify_all();
                }
                else {
                    if( jsonMessage.count( "result" ) != 0 ) {
                        std::unique_lock< std::mutex > lock( m_mutex );
                        m_answers.emplace( std::move( messageId ), std::move( jsonMessage ) );
                        lock.unlock();
                        m_condVar.notify_all();
                    }
                    else {
                        std::unique_lock< std::mutex > lock( m_mutex );
                        m_requestMessages.push_back( peerMessage.getMessage() );
                        lock.unlock();
                        m_condVar.notify_all();
                    }
                }
            },
            securityArgs,
            serverName );
        if( m_server->startAutofindPort( "wss", m_port ) ) {
            assert( m_port > 0 && m_port < std::numeric_limits< std::uint16_t >::max() );
            break;
        }
        m_server.reset();
    }

    if( !m_server || !m_server->isStarted() ) {
        throw std::runtime_error( "Cannot run " + serverName + " WebSocket server!" );
    }

    m_server->onPeerRegister_ = [this]( tiny::ws::server::peer_ptr_t peer ) {
        setConnected( true );
        dynamic_cast< TestServerPeer* >( peer )->onHubRegistered( "any_serial" );
    };

    m_server->onPeerUnregister_ = [this]( tiny::ws::server::peer_ptr_t peer ) { setConnected( false ); };
}

bool TestControllerServer::isConnected() const {
    return m_connected;
}

void TestControllerServer::waitForConnection( const char* file,
                                              int line,
                                              bool connected,
                                              std::chrono::seconds timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_connected != connected ) {
        if( !m_condVar.wait_for( lock, timeout, [&] { return m_connected == connected; } ) ) {
            ADD_FAILURE() << "Expected connected to be " << connected << " at: " << file << ":" << line;
            throw std::runtime_error( "TestControllerServer::waitForConnection timeout" );
        }
    }
}

JsonObject TestControllerServer::waitForRequest( const char* file, int line, std::chrono::seconds timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_requestMessages.empty() ) {
        if( !m_condVar.wait_for( lock, timeout, [this] { return !m_requestMessages.empty(); } ) ) {
            ADD_FAILURE_AT( file, line ) << "No requests received";
            throw std::exception();
        }
    }
    auto tmp = m_requestMessages.front();
    m_requestMessages.pop_front();
    return tmp;
}

void TestControllerServer::sendResponse( const JsonObject& request, Json result, Json error, Json sender ) {
    Json response;
    response[ "id" ] = request[ "id" ];
    response[ "method" ] = request[ "method" ];
    if( request.count( "api" ) != 0 ) {
        response[ "api" ] = request[ "api" ];
    }
    response[ "result" ] = std::move( result );
    if( !error.empty() ) {
        response[ "error" ] = std::move( error );
    }
    if( !sender.empty() ) {
        response[ "sender" ] = std::move( sender );
    }

    m_server->sendMessageToAllLoggedInPeers( response );
}

JsonObject TestControllerServer::sendtoHubAndGetAnswer( const char* file,
                                                        int line,
                                                        JsonObject request,
                                                        std::chrono::seconds timeout ) {
    std::string messageId;
    if( !json_get( request, "id", messageId ) ) {
        messageId = db::DbId::genOID();
        request[ "id" ] = messageId;
    }

    m_server->sendMessageToAllLoggedInPeers( request );

    std::unique_lock< std::mutex > lock( m_mutex );
    if( m_answers.count( messageId ) == 0 ) {
        if( !m_condVar.wait_for( lock, timeout, [&] { return m_answers.count( messageId ) != 0; } ) ) {
            ADD_FAILURE_AT( file, line ) << "No Responses received";
            throw std::exception();
        }
    }

    auto tmp = m_answers[ messageId ];
    m_answers.erase( messageId );
    return tmp;
}

void TestControllerServer::sendBroadcast( const std::string& msgSubclass, JsonObject result ) {
    Json broadcast;
    broadcast[ "id" ] = "ui_broadcast";
    broadcast[ "msg_subclass" ] = msgSubclass;
    broadcast[ "result" ] = std::move( result );
    m_server->sendMessageToAllLoggedInPeers( broadcast );
}

JsonObject TestControllerServer::waitForBroadcast( const std::string& msgSubclass,
                                                   const char* file,
                                                   int line,
                                                   std::chrono::milliseconds timeout ) {
    std::unique_lock< std::mutex > lock( m_mutex );

    while( timeout >= std::chrono::seconds( 0 ) ) {
        for( auto it = m_broadcastMessages.begin(); it != m_broadcastMessages.end(); ++it ) {
            if( json_get_str( *it, "msg_subclass" ) == msgSubclass ) {
                auto broadcast = std::move( *it );
                m_broadcastMessages.erase( it );
                return broadcast;
            }
        }

        if( timeout == std::chrono::seconds( 0 ) )
            break;

        size_t previousSize = m_broadcastMessages.size();
        auto beforeWaiting = std::chrono::steady_clock::now();
        if( !m_condVar.wait_for(
                lock, timeout, [this, &previousSize] { return m_broadcastMessages.size() > previousSize; } ) ) {
            break;
        }
        auto elapsedTime =
            std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::steady_clock::now() - beforeWaiting );

        timeout = timeout - elapsedTime;
    }

    LOGGER_ERROR( "No " + msgSubclass + " broadcasts received!" );
    ADD_FAILURE_AT( file, line ) << "No " + msgSubclass + " broadcasts received";
    throw std::exception();
}

void TestControllerServer::setConnected( bool connected ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_connected = connected;
    lock.unlock();
    m_condVar.notify_all();
}

}  // namespace Tests
}  // namespace HubPlatform
