#pragma once

#include "http/http_options.h"
#include <deque>

namespace HubPlatform {
namespace CommonHttp {

class MultipartBoundaryProcessor {
public:
    using Bytes = std::deque< char >;

public:
    void processHeader( const std::string& name, const std::string& value );
    void processBytes( const char* ptr, size_t length );

    size_t getLastBoundaryEnd() const;
    bool isLastBoundaryEndHandled() const;
    void setLastBoundaryEndHandled();

    void clear();

public:
    size_t getLastBoundaryStart() const;
    size_t getLastHeadersEnd() const;
    size_t getContentLength() const;

    const std::string& getBoundaryMarker() const;

private:
    bool searchForBoundary( size_t bytesProcessed, const char* ptr, size_t length );
    bool searchForHeadersEnd( size_t bytesProcessed, const char* ptr, size_t length );
    bool parseHeader();

private:
    size_t m_bytesProcessed = 0;
    std::string m_boundaryMarker;

    size_t m_boundaryWindowPosition = 0;
    Bytes m_boundaryWindow;
    size_t m_lastBoundaryStart = -1;
    size_t m_lastBoundaryEnd = -1;
    size_t m_lastBoundaryEndHandled = -1;

    Bytes m_lastHeader;
    size_t m_lastHeaderEnd = -1;
    size_t m_lastHeadersEnd = -1;

    size_t m_contentLength = -1;
};

}  // namespace CommonHttp
}  // namespace HubPlatform