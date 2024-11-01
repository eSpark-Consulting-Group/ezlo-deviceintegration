#include "cameras/rtsp_server/event.h"

namespace HubPlatform {
namespace Cameras {

EventBase::EventBase( StreamId streamId, EventType type, std::string metaData )
: m_streamId( streamId )
, m_eventType( type )
, m_data( std::move( metaData ) ) {}

EventBase::StreamId EventBase::getStreamId() const {
    return m_streamId;
}

EventType EventBase::getEventType() const {
    return m_eventType;
}

void EventBase::setMetaData( const std::string& data ) {
    m_data = data;
}

void EventBase::setMetaData( std::string&& data ) {
    m_data = std::move( data );
}

bool operator==( const EventBase& a, const EventBase& b ) {
    return a.getEventType() == b.getEventType() && a.getStreamId() == b.getStreamId();
}

}  // namespace Cameras
}  // namespace HubPlatform
