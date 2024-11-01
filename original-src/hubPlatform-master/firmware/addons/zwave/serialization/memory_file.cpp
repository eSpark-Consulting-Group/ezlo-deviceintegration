#include "zwave/serialization/memory_file.h"

#include <stdexcept>
#include <cstring>

namespace HubPlatform {
namespace Zwave {

MemoryFile::MemoryFile()
: m_readPos( 0 ) {}

MemoryFile::MemoryFile( const char* rawData, size_t len )
: m_data( rawData, rawData + len )
, m_readPos( 0 ) {}

bool MemoryFile::read( void* dst, size_t len ) {
    if( m_readPos + len > m_data.size() ) {
        throw std::out_of_range( "Out of range" );
    }

    std::memcpy( dst, m_data.data() + m_readPos, len );

    m_readPos += len;

    return true;
}

bool MemoryFile::write( const void* src, size_t len ) {
    m_data.insert( m_data.end(), static_cast< const char* >( src ), static_cast< const char* >( src ) + len );
    return true;
}

const std::vector< char >& MemoryFile::getData() const {
    return m_data;
}

}  // namespace Zwave
}  // namespace HubPlatform
