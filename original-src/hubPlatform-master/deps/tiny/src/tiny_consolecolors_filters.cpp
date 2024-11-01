#include "tiny/include/tiny/tiny_consolecolors_filters.h"

namespace cc {

size_t RpcMessageFiltersBySize::sizeLimit() const {
    return static_cast< size_t >( Limits::SIZE_LIMIT );
}

bool RpcMessageFiltersBySize::needFiltering( const std::string& data ) const {
    return ( data.size() > sizeLimit() );
}

bool RpcMessageFiltersBySize::updateJsonDump( const std::string& data, std::string& truncated ) {
    if( !needFiltering( data ) )
        return false;
    auto size = std::to_string( data.size() );
    truncated = data.substr( 0, sizeLimit() ) + " (truncated, size: " + size + ")";
    return true;
}

RpcMessageFiltersBySize::RpcMessageFiltersBySize()
: m_limit( static_cast< size_t >( Limits::SIZE_LIMIT ) ) {}

RpcMessageFiltersSingleton& RpcMessageFiltersSingleton::instance() {
    static RpcMessageFiltersSingleton filters;
    return filters;
}

void RpcMessageFiltersSingleton::pushBack( std::shared_ptr< IRpcMessageFilters > filters ) {
    Locker lock( m_mutex );
    m_filters.push_back( std::move( filters ) );
}

void RpcMessageFiltersSingleton::pushFront( std::shared_ptr< IRpcMessageFilters > filters ) {
    Locker lock( m_mutex );
    m_filters.push_front( std::move( filters ) );
}

void RpcMessageFiltersSingleton::reset() {
    Locker lock( m_mutex );
    m_filters.clear();
}

bool RpcMessageFiltersSingleton::processFilters( const std::string& data, std::string& filtered ) const {
    Locker lock( m_mutex );
    for( auto& filter : m_filters ) {
        if( !filter->needFiltering( data ) )
            continue;
        if( filter->updateJsonDump( data, filtered ) ) {
            return true;
        }
    }
    return false;
}

}  // namespace cc