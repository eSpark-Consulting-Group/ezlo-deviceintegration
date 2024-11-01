#pragma once

#include "http/http_plugin_connections.h"
#include "http/plugin_connection_queue.h"
#include <curl/curl.h>
#include <atomic>
#include <thread>
#include <future>
#include <list>

namespace HubPlatform {
namespace CommonHttp {

constexpr int threadExitDelayDefault = 1;

class HttpMultiClient final {
public:
    HttpMultiClient( HttpPluginConnections& impl, PluginConnectionQueue& connectionQueue );
    ~HttpMultiClient();

    void reset();
    /** Not synchronized call */
    bool addMultiHandle( Connection& connection );
    /** Not synchronized call */
    bool removeMultiHandle( Connection& connection );

    unsigned clearQueue();

    void runMultiHandleThread( std::shared_ptr< std::promise< void > > readyToRun );

    int exitDelayInterval() const;
    void exitDelayInterval( int value );

    static Reason getReason( const CURLMsg* msg, Connection* connection );
    static bool isSSLConnectionError( CURLcode code );

private:
    CURLMcode getCurlMultiTimeout( timeval& timeout );
    int waitForDataReady( timeval& timeout, int& stillAlive );
    void handleMessages();

private:
    HttpPluginConnections& m_connections;
    PluginConnectionQueue& m_connectionQueue;
    std::atomic_int m_exitDelayInterval{ threadExitDelayDefault };

    CurlMultiPtr m_multi;
};

}  // namespace CommonHttp
}  // namespace HubPlatform