#pragma once
#include "http_bridge/session.h"

class lws;

namespace HubPlatform {
namespace HttpBridge {

class ISessionManager {
public:
    virtual ~ISessionManager() = default;
    virtual SessionPtr startSession( lws* wsi, const char* sessionType ) = 0;
    virtual void finishSession( lws* wsi ) = 0;
    virtual SessionPtr findSession( lws* wsi ) const = 0;
    using EachSessionPredicate = std::function< void( const std::string&, SessionPtr ptr ) >;
    virtual void forEachSession( const std::string& msg, EachSessionPredicate callback ) = 0;

    virtual size_t sessionCount() const = 0;
    virtual size_t eventStreamsCount() const = 0;
    virtual void incrementExternalSessions() = 0;
    virtual void decrementExternalSessions() = 0;
    virtual size_t externalSessions() const = 0;
};

}  // namespace HttpBridge
}  // namespace HubPlatform