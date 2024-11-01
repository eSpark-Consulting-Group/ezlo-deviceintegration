#include "uid/extensions/http_links/events/link_updated_event.h"
#include "uid/extensions/http_links/resources.h"

namespace HubPlatform {
namespace HttpLinks {

LinkUpdatedEvent::LinkUpdatedEvent( std::string linkId, Json userData, std::string url, std::string secureUrl )
: EventBase( "", std::move( linkId ), std::move( userData ) )
, m_url( std::move( url ) )
, m_secureUrl( std::move( secureUrl ) ) {}

Json LinkUpdatedEvent::toJson() const {
    auto result = EventBase::toJson();
    json_remove( result, "id" );
    result[ "url" ] = m_url;
    result[ "secure_url" ] = m_secureUrl;
    return result;
}

const char* LinkUpdatedEvent::name() const {
    return Resources::Events::HttpLinkUpdatedEvent;
}

}  // namespace HttpLinks
}  // namespace HubPlatform
