#pragma once

#include "uid/server/server_peer.h"
#include "jsonrpc/jsonrpc.h"
#include <tiny/tiny_ws.h>
#include <atomic>

namespace HubPlatform {
namespace HttpBridge {
class ApiBridge;
}
}  // namespace HubPlatform

typedef tiny::ws::server::server server_listener_base;
using server_vhost_settings = tiny::ws::nlws::server_vhost_settings;
using lws_context_configuration = tiny::ws::lws_context_configuration;

class server_listener : public tiny::ws::server::server {
protected:
    std::thread m_serverThread;
    std::atomic_bool m_bStopFlag{ false };

public:
    static bool g_bAcceptConnectionsFromLocalNetworkOnly;
    server_listener( server_peer::on_message_t onMessage,
                     const tiny::ws::security_args& sa,
                     const std::string& uri = "ws",
                     int port = 17000,
                     server_listener_base::onBeforeCreateContext_t onBeforeCreateContext = nullptr );
    server_listener( server_peer::on_message_t onMessage,
                     const server_vhost_settings& secureHost,
                     const server_vhost_settings& insecureHost,
                     server_listener_base::onBeforeCreateContext_t onBeforeCreateContext = nullptr );
    ~server_listener() override;
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
        return server_listener_base::toJSON( bSkipEmptyStats );
    }

    int getPort() const;
    int getInsecurePort() const;
    bool getServerOpenComplete() const;

    static HubPlatform::HttpBridge::ApiBridge& getApiBridge();

protected:
    void setUp();

private:
    server_peer::on_message_t m_onMessage;
    server_listener_base::onBeforeCreateContext_t m_onBeforeCreateContext;
    server_vhost_settings m_defaultHost;
    server_vhost_settings m_insecureHost;

    std::atomic_bool m_serverOpenComplete{ false };

};  /// class server_listener
