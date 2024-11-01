#include "http/http_events.h"
#include "http/http_options.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CommonHttp {

namespace Fields {
const char* const code = "code";
const char* const text = "text";
const char* const reason = "reason";
const char* const codeStatus = "code_status";
const char* const data = "data";
const char* const last = "last";
const char* const headers = "headers";
const char* const maxDataLength = "max_data_length";
const char* const progress = "progress";
const char* const multipleHeaders = "multiple_headers";
}  // namespace Fields

int toInt( Reason value ) {
    return static_cast< int >( value );
}

const char* ConnectionClosedEvent::reasonName( Reason reason ) {
    static const char* const arr[] = { "Ok",
                                       "Cannot resolve DNS name",
                                       "Cannot connect to the server",
                                       "SSL Connection Error",
                                       "Closed by client",
                                       "Closed by server",
                                       "Timeout of response is reached",
                                       "Unknown error" };

    int index = static_cast< int >( reason );
    if( index >= 0 && index < static_cast< int >( Reason::END_OF_REASONS ) )
        return arr[ index ];
    return arr[ static_cast< int >( Reason::UNKNOWN_ERROR ) ];
}

ConnectionClosedEvent::ConnectionClosedEvent( const std::string& id,
                                              const std::string& url,
                                              const Json& userData ) noexcept
: Event( id, url, userData ) {}

Json ConnectionClosedEvent::toJson() const {
    Json result = Event::toJson();

    result[ Fields::reason ] = { { Fields::code, m_reasonCode }, { Fields::text, m_reasonText } };
    if( m_code > 0 ) {
        json_add_int( result, Fields::code, m_code );
        json_add_str( result, Fields::codeStatus, m_codeStatus );
    }
    return result;
}

void ConnectionClosedEvent::fromJson( const Json& from ) {
    Event::fromJson( from );
    Json reason;
    json_get_obj( from, Fields::reason, reason );
    m_reasonCode = json_get_int( reason, Fields::code );
    m_reasonText = json_get_str( reason, Fields::text );

    m_code = json_get_int( from, Fields::code, 0 );
    m_codeStatus = json_get_str( from, Fields::codeStatus );
}

ConnectionClosedEvent& ConnectionClosedEvent::setReason( Reason reason ) {
    m_reasonCode = static_cast< int >( reason );
    m_reasonText = reasonName( reason );
    return *this;
}

ConnectionClosedEvent& ConnectionClosedEvent::reasonText( std::string&& text ) {
    m_reasonText = std::move( text );
    return *this;
}

ConnectionClosedEvent& ConnectionClosedEvent::reasonText( const std::string& text ) {
    m_reasonText = text;
    return *this;
}
int ConnectionClosedEvent::reasonCode() const {
    return m_reasonCode;
}
const std::string& ConnectionClosedEvent::reasonText() const {
    return m_reasonText;
}

void ConnectionClosedEvent::accept( EventVisitor& visitor ) {
    visitor.visit( *this );
}

void ConnectionClosedEvent::httpStatus( int code, const std::string& status ) {
    m_code = code;
    m_codeStatus = status;
}

int ConnectionClosedEvent::httpCode() const {
    return m_code;
}

const std::string& ConnectionClosedEvent::httpCodeStatus() const {
    return m_codeStatus;
}

Json DataReceivedEvent::toJson() const {
    Json result = Event::toJson();
    json_add_int( result, Fields::code, m_code );
    json_add_str( result, Fields::data, m_data );
    json_add_bool( result, Fields::last, m_last );
    if( m_multipleHeaders )
        json_add_bool( result, Fields::multipleHeaders, m_multipleHeaders );

    if( !m_headers.empty() ) {
        Json& headers = result[ Fields::headers ];
        serializeHeaders( headers );
    }

    return result;
}

void DataReceivedEvent::fromJson( const Json& from ) {
    Event::fromJson( from );
    m_code = json_get_int( from, Fields::code, 0 );
    auto headersIt = from.find( Fields::headers );

    if( headersIt != from.end() ) {
        from_json( *headersIt, m_headers );
    }

    m_data = json_get_str( from, Fields::data );
    m_last = json_get_bool( from, Fields::last, true );
    m_multipleHeaders = json_get_bool( from, Fields::multipleHeaders, false );
}

DataReceivedEvent::DataReceivedEvent( const std::string& id,
                                      const std::string& url,
                                      const Json& userData,
                                      bool multiHeaders ) noexcept
: Event( id, url, userData )
, m_multipleHeaders( multiHeaders ) {}

DataReceivedEvent& DataReceivedEvent::code( int code ) {
    m_code = code;
    return *this;
}
DataReceivedEvent& DataReceivedEvent::headers( Headers&& headers ) {
    m_headers = std::move( headers );
    return *this;
}
DataReceivedEvent& DataReceivedEvent::headers( const Headers& headers ) {
    m_headers = headers;
    return *this;
}
DataReceivedEvent& DataReceivedEvent::data( std::string&& data ) {
    m_data = std::move( data );
    return *this;
}
DataReceivedEvent& DataReceivedEvent::data( const std::string& data ) {
    m_data = data;
    return *this;
}
DataReceivedEvent& DataReceivedEvent::last( bool last ) {
    m_last = last;
    return *this;
}
int DataReceivedEvent::code() const {
    return m_code;
}
const Headers& DataReceivedEvent::headers() const {
    return m_headers;
}
const std::string& DataReceivedEvent::data() const {
    return m_data;
}
bool DataReceivedEvent::last() const {
    return m_last;
}

void DataReceivedEvent::accept( EventVisitor& visitor ) {
    visitor.visit( *this );
}

bool DataReceivedEvent::multipleHeaders() const {
    return m_multipleHeaders;
}

DataReceivedEvent& DataReceivedEvent::multipleHeaders( bool value ) {
    m_multipleHeaders = value;
    return *this;
}

void DataReceivedEvent::serializeHeaders( Json& response ) const {
    if( !m_headers.empty() ) {
        to_json( response, m_headers, m_multipleHeaders );
    }
}

Json DataSendEvent::toJson() const {
    Json result = Event::toJson();
    result[ Fields::maxDataLength ] = m_maxDataLength;
    return result;
}

void DataSendEvent::fromJson( const Json& from ) {
    Event::fromJson( from );
    m_maxDataLength = json_get_int( from, Fields::maxDataLength, 0 );
}

DataSendEvent::DataSendEvent( const std::string& id, const std::string& url, const Json& userData ) noexcept
: Event( id, url, userData ) {}

int DataSendEvent::maxDataLength() const {
    return m_maxDataLength;
}

DataSendEvent& DataSendEvent::maxDataLength( int ) {
    return *this;
}

void DataSendEvent::accept( EventVisitor& visitor ) {
    visitor.visit( *this );
}

ProgressEvent::ProgressEvent( const std::string& id, int progress ) noexcept
: Event( id, "", "" )
, m_progress( progress ) {}

Json ProgressEvent::toJson() const {
    Json result = Event::toJson();
    result[ Fields::progress ] = m_progress;
    return result;
}

void ProgressEvent::fromJson( const Json& from ) {
    Event::fromJson( from );
    m_progress = json_get_int( from, Fields::progress, 0 );
}

void ProgressEvent::accept( EventVisitor& visitor ) {
    visitor.visit( *this );
}

int ProgressEvent::progress() const {
    return m_progress;
}

ProgressEvent& ProgressEvent::setProgress( int value ) {
    m_progress = value;
    return *this;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
