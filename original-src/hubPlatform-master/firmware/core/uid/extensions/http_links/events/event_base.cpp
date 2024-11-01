#include "uid/extensions/http_links/events/event_base.h"

namespace HubPlatform {
namespace HttpLinks {

EventBase::EventBase( std::string id, std::string linkId, Json userData )
: m_id( std::move( id ) )
, m_linkId( std::move( linkId ) )
, m_userData( std::move( userData ) ) {}

const std::string& EventBase::id() const {
    return m_id;
}

void EventBase::id( const std::string& value ) {
    m_id = value;
}

const std::string& EventBase::linkId() const {
    return m_linkId;
}

void EventBase::linkId( const std::string& value ) {
    m_linkId = value;
}

const Json& EventBase::userData() const {
    return m_userData;
}

void EventBase::userData( Json value ) {
    m_userData = std::move( value );
}

/** For large events serializes only event's metadata fields */
Json EventBase::toJson() const {
    Json result;
    result[ "id" ] = m_id;
    result[ "link_id" ] = m_linkId;
    result[ "user_data" ] = m_userData;
    return result;
}

/** For large events serializes all event's data */
Json EventBase::toJsonFull() const {
    return toJson();
}

}  // namespace HttpLinks
}  // namespace HubPlatform
