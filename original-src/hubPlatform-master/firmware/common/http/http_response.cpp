#include "http/http_response.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include <cstring>

namespace HubPlatform {
namespace CommonHttp {

bool HttpStatus::parse( const char* message, const std::string& pattern ) {
    if( !message || !*message || strlen( message ) <= pattern.size() ||
        strncmp( message, pattern.c_str(), pattern.size() ) != 0 )
        return false;

    std::string s{ message };
    unsigned offset = pattern.size();
    const char* ptr = s.c_str();
    auto spacePos = s.find( ' ', offset );
    while( spacePos != std::string::npos ) {
        std::string statusCode{ ptr + offset, spacePos - offset };

        offset += statusCode.size() + 1;
        std::string reason{ ptr + offset };
        reason.erase( reason.find_last_not_of( " \r\n" ) + 1 );  // rtrim

        int code = std::stoi( statusCode );
        if( code >= 100 && code < 1000 ) {
            set( code, reason );
            return true;
        }
        else {
            LOGGER_DEBUG( cc::error( "Invalid HTTP status code parsed out: " ) + cc::num10( code ) + ", from '" +
                          cc::debug( s ) + "'" );
        }

        break;
    }
    return false;
}

void HttpStatus::set( int code, std::string reason ) {
    m_code = code;
    m_reason = std::move( reason );
}

void HttpResponse::clear() {
    HttpResponse t;
    std::swap( t, *this );
}

HttpResponse& HttpResponse::setBytesSaved( size_t value ) {
    m_bytesSaved = value;
    return *this;
}
HttpResponse& HttpResponse::setContentLength( size_t value ) {
    m_contentLength = value;
    return *this;
}
size_t HttpResponse::getContentLength() const {
    return m_contentLength;
}

HttpResponse& HttpResponse::appendBytesSaved( size_t value ) {
    m_bytesSaved += value;
    return *this;
}

size_t HttpResponse::getBytesSaved() const {
    return m_bytesSaved;
}

/** Inserts pair in a headers multimap */
void HttpResponse::addHeader( const std::string& name, std::string&& value ) {
    m_headers.emplace( name, std::move( value ) );
}

HttpResponse& HttpResponse::setHttpStatus( int status, const std::string& reason ) {
    if( m_httpStatus != status ) {
        m_httpStatus = status;
        m_httpReason = reason;
    }
    return *this;
}
int HttpResponse::getHttpStatus() const {
    return m_httpStatus;
}
const std::string& HttpResponse::getHttpReason() const {
    return m_httpReason;
}

const Headers& HttpResponse::getHeaders() const {
    return m_headers;
}
Headers& HttpResponse::getHeaders() {
    return m_headers;
}
HttpResponse::Bytes& HttpResponse::getBuffer() {
    return m_buffer;
}
const HttpResponse::Bytes& HttpResponse::getBuffer() const {
    return m_buffer;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
