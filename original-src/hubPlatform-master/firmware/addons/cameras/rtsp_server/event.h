#pragma once

#include <string>

namespace HubPlatform {
namespace Cameras {

enum class EventType { STREAM_CREATED, STREAM_DELETED };

class EventBase {
public:
    using StreamId = std::string;

    EventBase( StreamId streamId, EventType type, std::string metaData = "" );
    StreamId getStreamId() const;
    EventType getEventType() const;
    std::string getMetaData() const;

    void setMetaData( const std::string& data );
    void setMetaData( std::string&& data );

private:
    StreamId m_streamId;
    EventType m_eventType;
    std::string m_data;
};

bool operator==( const EventBase& a, const EventBase& b );

}  // namespace Cameras
}  // namespace HubPlatform
