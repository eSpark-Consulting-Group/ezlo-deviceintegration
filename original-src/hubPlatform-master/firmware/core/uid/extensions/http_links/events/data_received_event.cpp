#include "uid/extensions/http_links/events/data_received_event.h"
#include "uid/extensions/http_links/resources.h"

namespace HubPlatform {
namespace HttpLinks {

DataReceivedEvent::DataReceivedEvent( std::string id,
                                      std::string linkId,
                                      Json userData,
                                      DataReceivedEvent::Headers headers,
                                      DataReceivedEvent::Params params,
                                      std::string binaryData,
                                      bool last,
                                      std::string method )
: EventBase( std::move( id ), std::move( linkId ), std::move( userData ) )
, m_headers( std::move( headers ) )
, m_params( std::move( params ) )
, m_binaryData( std::move( binaryData ) )
, m_last( last )
, m_method( std::move( method ) ) {}

Json DataReceivedEvent::toJson() const {
    Json event = EventBase::toJson();
    event[ "last" ] = m_last;
    ConnectionInfo::fillArray( event, "headers", m_headers );
    ConnectionInfo::fillArray( event, "params", m_params );
    event[ "method" ] = m_method;
    if( m_binaryData.size() > 1000 ) {
        event[ "data_size" ] = m_binaryData.size();
        event[ "data" ] = "";
    }
    else {
        event[ "data" ] = std::string( m_binaryData.data(), m_binaryData.size() );
    }
    return event;
}

Json DataReceivedEvent::toJsonFull() const {
    Json event = DataReceivedEvent::toJson();
    event[ "data" ] = std::string( m_binaryData.data(), m_binaryData.size() );
    event[ "data_size" ] = m_binaryData.size();
    return event;
}

void DataReceivedEvent::headers( DataReceivedEvent::Headers headers ) {
    m_headers = std::move( headers );
}

const DataReceivedEvent::Headers& DataReceivedEvent::headers() const {
    return m_headers;
}

void DataReceivedEvent::params( DataReceivedEvent::Params params ) {
    m_params = std::move( params );
}

const DataReceivedEvent::Params& DataReceivedEvent::params() const {
    return m_params;
}

void DataReceivedEvent::binaryData( const char* ptr, unsigned int length ) {
    m_binaryData.clear();
    if( ptr && length )
        m_binaryData.assign( ptr, length );
}

const std::string& DataReceivedEvent::binaryData() const {
    return m_binaryData;
}

void DataReceivedEvent::last( bool value ) {
    m_last = value;
}

bool DataReceivedEvent::last() const {
    return m_last;
}

const std::string& DataReceivedEvent::method() const {
    return m_method;
}

void DataReceivedEvent::method( std::string value ) {
    m_method = std::move( value );
}

const char* DataReceivedEvent::name() const {
    return Resources::Events::HttpDataReceivedEvent;
}

}  // namespace HttpLinks
}  // namespace HubPlatform
