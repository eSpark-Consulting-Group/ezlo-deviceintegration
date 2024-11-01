#pragma once
#include <string>
#include <utility>
#include <deque>

namespace HubPlatform {
namespace HttpLinks {

struct ConnectionHandle final {
public:
    using KeyValue = std::pair< std::string, std::string >;
    using Headers = std::vector< KeyValue >;

public:
    ConnectionHandle() = default;
    ConnectionHandle( std::string linkId, const void* wsiPtr )
    : m_linkId( std::move( linkId ) )
    , m_wsiPtr( wsiPtr ) {}
    const std::string& linkId() const {
        return m_linkId;
    }
    const void* lws() const {
        return m_wsiPtr;
    }
    void forceClose( bool value ) {
        m_forceClose = value;
    }
    bool forceClose() const {
        return m_forceClose;
    }

    void exitCode( int code, std::string codeStatus ) {
        m_code = code;
        m_codeStatus = std::move( codeStatus );
    }
    int code() const {
        return m_code;
    }
    const std::string& codeStatus() const {
        return m_codeStatus;
    }
    void appendData( std::string data, bool last ) {
        m_dataPortions.push_back( std::move( data ) );
        m_last = last;
        m_first = true;
    }
    bool last() const {
        return m_last;
    }
    bool first() const {
        return m_first;
    }
    size_t portions() const {
        return m_dataPortions.size();
    }
    /** @throws std::out_of_range */
    const std::string& portionAt( size_t index ) const {
        return m_dataPortions.at( index );
    }
    void popFrontPortion() {
        return m_dataPortions.pop_front();
    }
    std::string& frontPortion() {
        return m_dataPortions.front();
    }

    const Headers& headers() const {
        return m_headers;
    }
    void headers( Headers value ) {
        m_headers = std::move( value );
    }

    void emitNextDataSend( bool value ) {
        m_nextDataSend = value;
    }
    bool emitNextDataSend() const {
        return m_nextDataSend;
    }

private:
    std::string m_linkId;
    const void* m_wsiPtr{ nullptr };
    bool m_forceClose{ false };
    int m_code{ 0 };
    std::string m_codeStatus;
    Headers m_headers;
    std::deque< std::string > m_dataPortions;
    bool m_first{ false };
    bool m_last{ false };
    bool m_nextDataSend{ false };
};

}  // namespace HttpLinks
}  // namespace HubPlatform