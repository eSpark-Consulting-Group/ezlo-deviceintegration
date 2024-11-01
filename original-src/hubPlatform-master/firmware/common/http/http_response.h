#pragma once

#include "http/http_options.h"
#include <vector>
#include <string>

namespace HubPlatform {
namespace CommonHttp {

class HttpStatus {
public:
    HttpStatus() = default;

    void set( int code, std::string reason );
    int code() const {
        return m_code;
    }
    const std::string& reason() const {
        return m_reason;
    }

    bool empty() const {
        return m_code == 0 && m_reason.empty();
    }
    bool parse( const char* message, const std::string& pattern );

private:
    int m_code = 0;
    std::string m_reason;
};

class HttpResponse {
public:
    using Bytes = std::vector< unsigned char >;

public:
    void clear();
    HttpResponse& setHttpStatus( int status, const std::string& reason = std::string() );
    int getHttpStatus() const;
    const std::string& getHttpReason() const;
    HttpResponse& setBytesSaved( size_t value );
    HttpResponse& appendBytesSaved( size_t value );
    size_t getBytesSaved() const;
    HttpResponse& setContentLength( size_t value );
    size_t getContentLength() const;
    const Headers& getHeaders() const;
    Headers& getHeaders();

    void addHeader( const std::string& name, std::string&& value );

    Bytes& getBuffer();
    const Bytes& getBuffer() const;

private:
    Bytes m_buffer;
    size_t m_contentLength = 0;  //!< number of bytes to read from HTTP response, defined by HTTP headers
    size_t m_bytesSaved = 0;     //!< number of bytes read from HTTP response stream
    int m_httpStatus = 0;
    std::string m_httpReason;
    Headers m_headers;
};

}  // namespace CommonHttp
}  // namespace HubPlatform