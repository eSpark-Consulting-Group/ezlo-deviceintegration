#pragma once

#include "http_bridge/endpoint.h"
#include "tiny/tiny_utils.h"
#include <map>

namespace HubPlatform {
namespace Uid {
namespace Extensions {

class HttpSession {
public:
    struct UserData {
        virtual ~UserData() = default;
    };

    virtual ~HttpSession() = default;

    void traceId( std::string value ) {
        m_traceId = std::move( value );
    }
    std::string const& traceId() const {
        return m_traceId;
    }
    void spanId( std::string value ) {
        m_spanId = std::move( value );
    }
    std::string const& spanId() const {
        return m_spanId;
    }

public:
    std::string addon;

    std::string url;
    std::string method;
    std::map< std::string, std::string > headers;
    std::unique_ptr< UserData > userData;

    using UriParam = std::pair< std::string, std::string >;
    using UriParams = std::vector< UriParam >;
    UriParams uriParams;
    bool responseHeadersWritten = false;

public:
    bool hasHeader( const std::string& header ) const {
        return headers.count( tiny::tools::to_lower( header ) ) != 0;
    }

    bool hasHeader( const std::string& header, const std::string& value ) const {
        auto it = headers.find( tiny::tools::to_lower( header ) );
        if( it == headers.end() )
            return false;

        return tiny::tools::to_lower( it->second ) == tiny::tools::to_lower( value );
    }
    std::tuple< bool, std::string > findUriParam( const std::string& uriParam ) const {
        auto foundIt = std::find_if(
            begin( uriParams ), end( uriParams ), [&]( const std::pair< std::string, std::string >& record ) -> bool {
                return record.first == uriParam;
            } );
        if( foundIt != end( uriParams ) ) {
            return std::make_tuple( true, foundIt->second );
        }
        return std::make_tuple( false, std::string{} );
    }
    void addBytesRx( size_t size ) {
        m_bytesRx += size;
    }
    size_t bytesRx() const {
        return m_bytesRx;
    }
    void addBytesTx( size_t size ) {
        m_bytesTx += size;
    }
    size_t bytesTx() const {
        return m_bytesTx;
    }
    void closeSession() {
        m_sessionClosed = true;
    }
    bool isClosed() const {
        return m_sessionClosed;
    }

private:
    size_t m_bytesRx{ 0 };
    size_t m_bytesTx{ 0 };
    bool m_sessionClosed{ false };

    std::string m_traceId;
    std::string m_spanId;
};

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
