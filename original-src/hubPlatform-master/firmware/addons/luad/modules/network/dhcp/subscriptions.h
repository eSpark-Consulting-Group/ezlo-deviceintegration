#pragma once

#include "json_utils/json_fwd.h"
#include "luad/modules/network/dhcp/subscription.h"
#include "luad/modules/network/dhcp/dhcp.h"
#include "luad/modules/network/network_scan.h"
#include <memory>

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

using NotifyCallback = std::function< void( const std::string&, const std::string&, const Json& ) >;
class SharedMutex;
using SharedMutexPtr = std::unique_ptr< SharedMutex >;

class Subscriptions {
public:
    using AllHeaders = Dhcp::DhcpPacketUnpack::AllHeaders;
    using SessionId = std::string;

public:
    explicit Subscriptions( NotifyCallback&& );
    ~Subscriptions();

    Subscriptions( const Subscriptions& ) = delete;
    Subscriptions( Subscriptions&& ) = delete;

    SessionId subscribe( const Subscription& );
    bool unsubscribe( SessionId sessionId );
    size_t size() const;

    size_t notify( const AllHeaders& headers ) const;

    static SessionId nextSessionId();
    void clear();

private:
    bool buildResponse( const AllHeaders& headers, Json& response, std::string& eventName ) const;

private:
    mutable SharedMutexPtr m_mutexPtr;
    using MapSubscriptions = std::unordered_map< SessionId, Subscription >;
    MapSubscriptions m_sessions;
    NotifyCallback m_notifyCallback;
};

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform