#pragma once
#include "http/http_options.h"
#include "http/event_emitter_base.h"
#include "http/http_error.h"
#include <json_utils/json_fwd.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <mutex>

namespace HubPlatform {
namespace CommonHttp {

enum { MAX_PLUGIN_CONNECTIONS = 50 };

class Http : public HttpEventEmitterBase {
public:
    using PluginId = std::string;
    using RequestId = std::string;
    class Impl;

public:
    Http();
    ~Http() override;

    Http( const Http& ) = delete;
    Http& operator=( const Http& ) = delete;

    void stop();

public:
    RequestId request( const PluginId& pluginId, const HttpOptions& options );
    void repeatRequest( const PluginId& pluginId, const RequestId& requestId, const HttpOptions& options );
    bool closeConnection( const PluginId& pluginId, const RequestId& requestId );
    void closePluginConnections( const PluginId& pluginId );

    int sendData( const PluginId& pluginId, const RequestId& requestId, std::string data, bool last = true );
    ConnectionInfo getConnectionInfo( const PluginId& pluginId, const RequestId& requestId );

    void setMaxPluginConnections( unsigned );
    unsigned getMaxPluginConnections() const;

    bool joinable();
    void join();

    int exitDelayInterval() const;
    void exitDelayInterval( int value );

private:
    void checkImpl();

private:
    std::unique_ptr< Impl > m_pimpl;
};

}  // namespace CommonHttp
}  // namespace HubPlatform
