#include "json_utils/json_fwd.h"
#include "luad/modules/network/dhcp/subscriptions.h"
#include "luad/modules/network/dhcp/dhcp.h"
#include "luad/modules/network/dhcp/response_builder.h"
#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "tiny/include/yamc/alternate_shared_mutex.hpp"
#include "tiny/tiny_consolecolors.h"
#include <memory>
#include "log_utils/ezlo_log.h"
#include "database_client/dbid.h"

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

using SharedLock = yamc::shared_lock< SharedMutex >;
using UniqueLock = std::unique_lock< SharedMutex >;

class SharedMutex : public yamc::alternate::shared_mutex {};

Subscriptions::Subscriptions( NotifyCallback&& callback )
: m_notifyCallback( std::move( callback ) )
, m_mutexPtr( new SharedMutex() ) {}

Subscriptions::SessionId Subscriptions::subscribe( const Subscription& subscription ) {
    // write-lock here (m_sessions)
    assert( m_mutexPtr != nullptr );
    UniqueLock lock( *m_mutexPtr );

    auto id = Subscriptions::nextSessionId();
    m_sessions.emplace( id, subscription );

    return id;
}

/** Tries to erase a subscription by its Id.
 * @param sessionId - session identifier.
 * @return true - subscription erased, false - subscription with the id wasn't found.
 */
bool Subscriptions::unsubscribe( Subscriptions::SessionId sessionId ) {
    // write-lock here (m_sessions)
    assert( m_mutexPtr != nullptr );
    UniqueLock lock( *m_mutexPtr );

    return m_sessions.erase( sessionId ) > 0;
}

size_t Subscriptions::size() const {
    // read-lock (m_sessions)
    assert( m_mutexPtr != nullptr );
    SharedLock lock( *m_mutexPtr );

    return m_sessions.size();
}

static const char* commandName( Dhcp::ECommand command ) {
    static const char* arr[] = { "",     "discovery", "offer",      "request", "decline", "acknowledge",
                                 "nack", "release",   "information" };

    if( command > 0 && command < sizeof( arr ) / sizeof( arr[ 0 ] ) ) {
        return arr[ command ];
    }
    return arr[ 0 ];  // empty string if not found
}

bool Subscriptions::buildResponse( const AllHeaders& headers, Json& response, std::string& eventName ) const {
    using namespace HubPlatform::LuaNetwork::Dhcp;

    ethhdr eth = {};
    iphdr ip = {};
    DhcpPacket rawPacket = {};

    std::tie( eth, ip, std::ignore, rawPacket ) = headers;
    DhcpResponseBuilder builder( &eth, &ip, rawPacket );

    try {
        response = builder.buildResponse();
    }
    catch( const DhcpPacketError& e ) {
        Log::error( cc::error( "Dhcp ResponseBuilder runtime error: " ) + e.what() );
        return false;
    }

    // operationName
    Dhcp::ECommand command = builder.messageType();  // call it after builder.buildResponse()
    eventName = commandName( command );
    return true;
}

size_t Subscriptions::notify( const AllHeaders& headers ) const {
    if( !m_notifyCallback )
        return 0;

    std::string eventName;
    Json response;

    if( !buildResponse( headers, response, eventName ) )
        return 0;

    // read-lock here (m_sessions)
    assert( m_mutexPtr != nullptr );
    SharedLock lock( *m_mutexPtr );

    size_t count = 0;
    // iterate all subscribers and call their notification scripts
    for( const auto& session : m_sessions ) {
        SessionId sessionId = session.first;
        const Subscription& subscription = session.second;

        bool isOk = !subscription.hasOperations() || subscription.contains( eventName );
        if( !isOk )
            continue;

        // Add session id
        response[ "id" ] = sessionId;

        m_notifyCallback( subscription.scriptPath(), "dhcp_" + eventName, response );
        count++;
    }

    // return number of notified clients.
    return count;
}

Subscriptions::SessionId Subscriptions::nextSessionId() {
    return db::DbId::genOID();
}

void Subscriptions::clear() {
    assert( m_mutexPtr != nullptr );
    UniqueLock lock( *m_mutexPtr );

    m_sessions.clear();
}

Subscriptions::~Subscriptions() {
    clear();
}

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform
