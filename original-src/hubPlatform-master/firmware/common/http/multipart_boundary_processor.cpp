#include "http/multipart_boundary_processor.h"

#include <locale>
#include <algorithm>

namespace HubPlatform {
namespace CommonHttp {

namespace {

template < typename T >
size_t ci_find( const T& begin, const T& end, const std::string& rhs ) {
    auto it = std::search( begin, end, rhs.begin(), rhs.end(), []( char lhs, char rhs ) {
        return std::tolower( lhs ) == std::tolower( rhs );
    } );
    if( it != end )
        return it - begin;
    else
        return -1;
}

size_t ci_find( const std::string& lhs, const std::string& rhs ) {
    return ci_find( lhs.begin(), lhs.end(), rhs );
}

template < typename T >
std::reverse_iterator< T > make_reverse_iterator( T iter ) {
    return std::reverse_iterator< T >( iter );
}

}  // namespace

void MultipartBoundaryProcessor::processHeader( const std::string& name, const std::string& value ) {
    const char boundaryPrefix[] = "boundary=";
    if( ci_find( name, "Content-Type" ) != -1 ) {
        auto multipartPos = ci_find( value, "multipart" );
        auto boundaryPos = ci_find( value, boundaryPrefix );
        if( multipartPos != -1 && boundaryPos != -1 && multipartPos < boundaryPos ) {
            m_boundaryMarker = "--" + value.substr( boundaryPos + sizeof( boundaryPrefix ) - 1 );
        }
    }
}

bool MultipartBoundaryProcessor::searchForBoundary( size_t bytesProcessed, const char* ptr, size_t length ) {
    bool found = false;
    // Init frame begin window
    if( m_boundaryWindow.size() > m_boundaryMarker.size() ) {
        m_boundaryWindowPosition += m_boundaryWindow.size() - m_boundaryMarker.size();
        m_boundaryWindow.erase( m_boundaryWindow.begin(), m_boundaryWindow.end() - m_boundaryMarker.size() );
    }
    m_boundaryWindow.insert( m_boundaryWindow.end(), ptr, ptr + std::min( m_boundaryMarker.size(), length ) );

    // Search in frame
    auto rbegin = make_reverse_iterator( ptr + length );
    auto rend = make_reverse_iterator( ptr );
    auto it = std::search( rbegin, rend, m_boundaryMarker.rbegin(), m_boundaryMarker.rend() );
    if( it != rend ) {
        found = true;
        m_lastBoundaryStart = bytesProcessed + length - ( it - rbegin );
        m_lastBoundaryEnd = m_lastBoundaryStart - m_boundaryMarker.size();
        m_lastHeader.clear();
        m_contentLength = -1;
    }
    else {
        // Search in window
        auto it = std::search(
            m_boundaryWindow.rbegin(), m_boundaryWindow.rend(), m_boundaryMarker.rbegin(), m_boundaryMarker.rend() );
        if( it != m_boundaryWindow.rend() ) {
            found = true;
            m_lastBoundaryStart =
                m_boundaryWindowPosition + m_boundaryWindow.size() - ( it - m_boundaryWindow.rbegin() );
            m_lastBoundaryEnd = m_lastBoundaryStart - m_boundaryMarker.size();
            m_lastHeader.clear();
            m_contentLength = -1;
        }
    }

    // Init frame end window
    if( length >= m_boundaryMarker.size() ) {
        m_boundaryWindow.assign( ptr + length - m_boundaryMarker.size(), ptr + length );
        m_boundaryWindowPosition = bytesProcessed + length - m_boundaryMarker.size();
    }

    return found;
}

bool MultipartBoundaryProcessor::searchForHeadersEnd( size_t bytesProcessed, const char* ptr, size_t length ) {
    static const std::string headerEndMarker = "\r\n";

    size_t headerStart = bytesProcessed - m_lastHeader.size();
    m_lastHeader.insert( m_lastHeader.end(), ptr, ptr + std::min( headerEndMarker.size(), length ) );

    auto it = std::search( std::max( m_lastHeader.begin(), m_lastHeader.end() - headerEndMarker.size() * 2 ),
                           m_lastHeader.end(),
                           headerEndMarker.begin(),
                           headerEndMarker.end() );
    if( it != m_lastHeader.end() ) {
        m_lastHeaderEnd = headerStart + ( it - m_lastHeader.begin() ) + headerEndMarker.size();
        m_lastHeader.resize( it - m_lastHeader.begin() + headerEndMarker.size() );
        return true;
    }

    if( length > headerEndMarker.size() ) {
        auto it = std::search( ptr, ptr + length, headerEndMarker.begin(), headerEndMarker.end() );
        if( it != ptr + length ) {
            m_lastHeaderEnd = bytesProcessed + ( it - ptr ) + headerEndMarker.size();
            m_lastHeader.insert( m_lastHeader.end(), ptr + headerEndMarker.size(), it + headerEndMarker.size() );
            return true;
        }
        else {
            m_lastHeader.insert( m_lastHeader.end(), ptr + headerEndMarker.size(), ptr + length );
        }
    }

    return false;
}

bool MultipartBoundaryProcessor::parseHeader() {
    static const std::string delimiter = ": ";

    if( m_lastHeader.size() == 2 && m_lastHeader[ 0 ] == '\r' && m_lastHeader[ 1 ] == '\n' &&
        m_lastHeaderEnd != m_lastBoundaryStart + 2 ) {
        m_lastHeadersEnd = m_lastHeaderEnd;
        m_lastHeader.clear();
        return true;
    }

    auto delimiterIt = std::search( m_lastHeader.begin(), m_lastHeader.end(), delimiter.begin(), delimiter.end() );

    if( delimiterIt != m_lastHeader.end() ) {
        const char contentLength[] = "Content-Length";
        if( ci_find( m_lastHeader.begin(), m_lastHeader.begin() + sizeof( contentLength ) - 1, contentLength ) != -1 ) {
            auto valueIt = delimiterIt + delimiter.size();
            std::string value{ valueIt, m_lastHeader.end() - 2 };
            char* pEnd = nullptr;
            auto contentLength = strtoul( value.data(), &pEnd, 10 );
            if( pEnd != value.data() ) {
                m_contentLength = contentLength;
            }
        }
    }
    m_lastHeader.clear();
    return false;
}

void MultipartBoundaryProcessor::processBytes( const char* ptr, size_t length ) {
    if( m_boundaryMarker.empty() ) {
        return;
    }

    size_t processed = 0;
    if( searchForBoundary( m_bytesProcessed, ptr, length ) ) {
        processed = m_lastBoundaryStart - m_bytesProcessed;
    }

    if( m_lastBoundaryStart > m_lastBoundaryEnd ) {
        while( processed < length ) {
            if( searchForHeadersEnd( m_bytesProcessed + processed, ptr + processed, length - processed ) ) {
                if( !parseHeader() ) {
                    processed = m_lastHeaderEnd - m_bytesProcessed;
                    continue;
                }
            }
            processed = length;
        }
    }

    if( m_contentLength != -1 && m_lastHeadersEnd != -1 && m_lastHeadersEnd + m_contentLength > m_lastBoundaryEnd ) {
        m_lastBoundaryEnd = m_lastHeadersEnd + m_contentLength;
    }

    m_bytesProcessed += length;
}

void MultipartBoundaryProcessor::clear() {
    MultipartBoundaryProcessor t;
    std::swap( t, *this );
}

size_t MultipartBoundaryProcessor::getLastBoundaryStart() const {
    return m_lastBoundaryStart;
}

size_t MultipartBoundaryProcessor::getLastBoundaryEnd() const {
    return m_lastBoundaryEnd;
}

bool MultipartBoundaryProcessor::isLastBoundaryEndHandled() const {
    return m_lastBoundaryEndHandled == m_lastBoundaryEnd;
}

void MultipartBoundaryProcessor::setLastBoundaryEndHandled() {
    m_lastBoundaryEndHandled = m_lastBoundaryEnd;
}

size_t MultipartBoundaryProcessor::getLastHeadersEnd() const {
    return m_lastHeadersEnd;
}

size_t MultipartBoundaryProcessor::getContentLength() const {
    return m_contentLength;
}

const std::string& MultipartBoundaryProcessor::getBoundaryMarker() const {
    return m_boundaryMarker;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
