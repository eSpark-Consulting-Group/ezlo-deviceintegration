#pragma once

#include "json_utils/json_fwd.h"
#include "uid/extensions/ui_extension.h"
#include "uid/extensions/ui_bus_extension.h"
#include "uid/extensions/ui_http_extension.h"
#include "uid/extensions/http_links/resources.h"
#include "bus/messagesclasses.h"
#include <string>
#include <utility>
#include <ctime>

namespace HubPlatform {
namespace HttpLinks {

/** Holds parameters of an http link */
class HttpLink final {
public:
    using ScriptPath = std::string;
    using LinkId = std::string;
    using PluginId = std::string;

public:
    HttpLink() = default;
    HttpLink( PluginId pluginId, LinkId urlId, ScriptPath handler, Json userData, int timeout, bool answer );

    const std::string& secureLink() const {
        return m_secureLink;
    }
    void secureLink( PluginId value ) {
        m_secureLink = std::move( value );
    }
    const std::string& insecureLink() const {
        return m_insecureLink;
    }
    void insecureLink( PluginId value ) {
        m_insecureLink = std::move( value );
    }

    const PluginId& pluginId() const {
        return m_pluginId;
    }
    void pluginId( PluginId value ) {
        m_pluginId = std::move( value );
    }
    void linkId( LinkId newValue ) {
        m_urlId = std::move( newValue );
    }
    const LinkId& linkId() const {
        return m_urlId;
    }
    const Json& userData() const {
        return m_userData;
    }
    void userData( Json value ) {
        m_userData = std::move( value );
    }
    int timeout() const {
        return m_timeout;
    }
    void timeout( int value ) {
        m_timeout = value;
    }
    bool answer() const {
        return m_answer;
    }
    void answer( bool value ) {
        m_answer = value;
    }
    const ScriptPath& handler() const {
        return m_handler;
    }
    void handler( ScriptPath path ) {
        m_handler = std::move( path );
    }

    void deleted( time_t value ) {
        m_deleted = value;
    }
    time_t deleted() const {
        return m_deleted;
    }

private:
    PluginId m_pluginId;
    LinkId m_urlId;
    ScriptPath m_handler;
    Json m_userData;
    int m_timeout{ Resources::Limits::RESPONSE_TIMEOUT_DEFAULT };
    bool m_answer{ false };

    std::string m_secureLink;
    std::string m_insecureLink;

    time_t m_deleted{ 0 };
};

inline HttpLink::HttpLink( HttpLink::PluginId pluginId,
                           HttpLink::LinkId urlId,
                           HttpLink::ScriptPath handler,
                           Json userData,
                           int timeout,
                           bool answer )
: m_pluginId( std::move( pluginId ) )
, m_urlId( std::move( urlId ) )
, m_handler( std::move( handler ) )
, m_userData( std::move( userData ) )
, m_timeout( timeout )
, m_answer( answer ) {}

}  // namespace HttpLinks
}  // namespace HubPlatform
