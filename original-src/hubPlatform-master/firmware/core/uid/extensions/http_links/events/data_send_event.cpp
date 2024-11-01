#include "uid/extensions/http_links/events/data_send_event.h"
#include "uid/extensions/http_links/resources.h"

namespace HubPlatform {
namespace HttpLinks {

DataSendEvent::DataSendEvent( std::string id, std::string linkId, Json userData, int maxDataLength )
: EventBase( std::move( id ), std::move( linkId ), std::move( userData ) )
, m_maxDataLength( maxDataLength ) {}

Json DataSendEvent::toJson() const {
    auto result = EventBase::toJson();
    result[ "max_data_length" ] = m_maxDataLength;
    return result;
}

void DataSendEvent::maxDataLength( bool value ) {
    m_maxDataLength = value;
}

bool DataSendEvent::maxDataLength() const {
    return m_maxDataLength;
}

const char* DataSendEvent::name() const {
    return Resources::Events::HttpDataSendEvent;
}

}  // namespace HttpLinks
}  // namespace HubPlatform
