#pragma once

#include "http/plugin_connection_queue.h"
#include "http/connection.h"
#include "http/http_plugin_connections.h"
#include "http/http_multi_client.h"
#include "http/event_emitter.h"
#include <atomic>
#include <thread>
#include <future>
#include <list>

namespace HubPlatform {
namespace CommonHttp {

class HttpImpl : public HttpPluginConnections {
public:
    using PluginId = std::string;
    using ConnectionMap = std::map< ConnectionId, ConnectionPtr >;
    using PluginMap = std::map< std::string, ConnectionMap >;

public:
    explicit HttpImpl( HttpEventEmitter& emitter );
    ~HttpImpl() override;

    void runMultiHandleThread( std::shared_ptr< std::promise< void > > readyToRun );
    void start();
    void stop();
    bool isRunning();
    bool isStopped() const override;

    ConnectionPtr makeConnection( const HttpOptions& );

    bool addConnection( const PluginId& pluginId, const ConnectionId& connId, ConnectionPtr pConnection );
    ConnectionPtr findConnection( const PluginId& pluginId, const ConnectionId& connId ) override;
    ConnectionPtr findByHandle( CURL* handle ) const override;
    bool closeConnection( const PluginId& pluginId,
                          const ConnectionId& connId,
                          bool removeFromMulti,
                          bool throwIfConnectionIdInvalid = true ) override;
    void closePluginConnections( const PluginId& pluginId );

    void setMaxPluginConnections( unsigned value );
    unsigned getMaxPluginConnections() const;

    bool askAddToMultiHandle( const PluginId& pluginId, const ConnectionId& connId );

    bool joinable();
    void join();

    HttpMultiClient& getClient();

protected:
    static std::string generateId();
    void clearPlugins();

    ConnectionPtr popConnection( const PluginId& pluginId, const ConnectionId& connId );

private:
    HttpEventEmitter& m_eventEmitter;

    mutable SharedMutex m_pluginsMutex;
    PluginMap m_plugins;
    unsigned m_maxPluginConnections = MAX_PLUGIN_CONNECTIONS;

    std::atomic_bool m_isRunning{ false };
    std::atomic_bool m_isStopped{ false };
    std::atomic_bool m_isJoinable{ false };
    std::thread m_thread;

    PluginConnectionQueue m_connectionQueue;
    HttpMultiClient m_http;
};

}  // namespace CommonHttp
}  // namespace HubPlatform
