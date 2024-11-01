#pragma once

#include "test_utils/component_tests/ws_server/test_server_peer.h"

#include "jsonrpc/jsonrpc.h"
#include "tiny/tiny_ws.h"

#include <atomic>

typedef tiny::ws::server::server TestServer_base;
using lws_context_configuration = tiny::ws::lws_context_configuration;

namespace HubPlatform {
namespace Tests {

class TestServer : public tiny::ws::server::server {
    using OnBeforeCreateContext = TestServer_base::onBeforeCreateContext_t;

public:
    TestServer( TestServerPeer::OnMessageFunction onMessage,
                const tiny::ws::security_args& sa,
                std::string name,
                OnBeforeCreateContext beforeCreateContextHandler = nullptr );

    ~TestServer() override;

public:
    bool startAutofindPort( const std::string& uri,
                            int& port,
                            const lws_context_configuration& lwsContextConfig = lws_context_configuration() );

    bool isStarted() const;

    void onOpen( tiny::ws::server::hdl_t hdl ) override;

    void onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) override;

    //
    size_t sendMessageToAllLoggedInPeers( const std::string& msg,
                                          tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    size_t postMessageToAllLoggedInPeers( const std::string& msg,
                                          tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    size_t sendMessageToAllLoggedInPeers( const JsonObject& jo ) {
        return sendMessageToAllLoggedInPeers( jo.dump() );
    }

    size_t postMessageToAllLoggedInPeers( const JsonObject& joMsg ) {
        return postMessageToAllLoggedInPeers( joMsg.dump() );
    }

    //
    size_t sendMessageToAllLoggedInPeersNoLog( const std::string& msg,
                                               tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    size_t postMessageToAllLoggedInPeersNoLog( const std::string& msg,
                                               tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    size_t sendMessageToAllLoggedInPeersNoLog( const JsonObject& jo ) {
        return sendMessageToAllLoggedInPeersNoLog( jo.dump() );
    }

    size_t postMessageToAllLoggedInPeersNoLog( const JsonObject& joMsg ) {
        return postMessageToAllLoggedInPeersNoLog( joMsg.dump() );
    }

    nlohmann::json toJSON( bool bSkipEmptyStats ) const override {
        return TestServer_base::toJSON( bSkipEmptyStats );
    }

    bool stat_peer_register( TestServerPeer* pPeer );

    bool stat_peer_unregister( TestServerPeer* pPeer );

    typedef std::function< bool( TestServerPeer* pPeer ) > fn_for_each_peer_t;

    size_t stat_locked_for_each_peer( fn_for_each_peer_t fn );

private:
    TestServerPeer::OnMessageFunction m_onMessage;
    std::string m_name;

    std::atomic_bool m_serverStarted{ false };

    std::thread m_serverThread;
    std::atomic_bool m_isClosing;

    typedef std::set< TestServerPeer* > set_peers_t;
    set_peers_t m_set_all_peers;

};  /// class TestServer

}  // namespace Tests
}  // namespace HubPlatform
