#include "uid/extensions/http_links/events/connection_closed_event.h"
#include "uid/extensions/http_links/resources.h"
#include <utility>

namespace HubPlatform {
namespace HttpLinks {

ConnectionClosedEvent::ConnectionClosedEvent( std::string id,
                                              std::string linkId,
                                              Json userData,
                                              ConnectionClosedReason code,
                                              std::string reason )
: EventBase( std::move( id ), std::move( linkId ), std::move( userData ) )
, m_reasonCode( code )
, m_reasonText( std::move( reason ) ) {}

void ConnectionClosedEvent::reason( ConnectionClosedReason code, std::string message ) {
    m_reasonCode = code;
    m_reasonText = std::move( message );
}

Json ConnectionClosedEvent::toJson() const {
    Json result = EventBase::toJson();
    result[ "reason" ] = { { "code", toInt( m_reasonCode ) }, { "text", m_reasonText } };
    return result;
}

const char* ConnectionClosedEvent::name() const {
    return Resources::Events::HttpConnectionClosedEvent;
}

}  // namespace HttpLinks
}  // namespace HubPlatform
