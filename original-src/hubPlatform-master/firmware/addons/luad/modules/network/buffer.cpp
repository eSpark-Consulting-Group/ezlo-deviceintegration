#include "luad/modules/network/buffer.h"

#include "common_utils/cpp14.h"

#include <cassert>

namespace HubPlatform {
namespace LuaNetwork {

IBuffer::~IBuffer() = default;
IDataSourceManager::~IDataSourceManager() = default;

Buffer::Buffer( IDataSourceManager& dataSourceManager )
: m_dataSourceManager( dataSourceManager )
, m_dataSourcePaused( false ) {
    reset();
}

char* Buffer::front() {
    return m_buffer.data() + m_writeIndex;
}

std::size_t Buffer::size() const {
    return m_spaceToWrite;
}

void Buffer::commit( std::size_t providedDataAmount ) {
    assert( providedDataAmount <= m_spaceToWrite );

    m_writeIndex += providedDataAmount;
    m_spaceToWrite -= providedDataAmount;
    m_spaceToRead += providedDataAmount;

    if( !m_dataSourcePaused && m_spaceToWrite == 0 ) {
        m_dataSourceManager.pauseDataSource();
        m_dataSourcePaused = true;
    }
}

std::string Buffer::fetchData( std::size_t amount ) {
    const auto realAmount = amount < m_spaceToRead ? amount : m_spaceToRead;
    auto firstIt = m_buffer.data() + m_readIndex;
    auto lastIt = firstIt + realAmount;

    const std::string data( firstIt, lastIt );

    m_readIndex += realAmount;
    m_spaceToRead -= realAmount;

    if( m_readIndex == m_writeIndex )
        reset();

    if( m_dataSourcePaused && m_spaceToWrite != 0 ) {
        m_dataSourceManager.resumeDataSource();
        m_dataSourcePaused = false;
    }

    return data;
}

std::string Buffer::inspectData( std::size_t amount ) const {
    auto firstIt = m_buffer.data() + m_readIndex;
    auto lastIt = firstIt + ( amount < m_spaceToRead ? amount : m_spaceToRead );

    return std::string{ firstIt, lastIt };
}

std::size_t Buffer::inspectDataSize() const {
    return m_spaceToRead;
}

void Buffer::reset() {
    m_readIndex = m_writeIndex = 0;
    m_spaceToRead = 0;
    m_spaceToWrite = MaxCapacity;
}

std::unique_ptr< IBuffer > makeCycleBuffer( IDataSourceManager& dataSourceManager ) {
    return std::make_unique< Buffer >( dataSourceManager );
}

}  // namespace LuaNetwork
}  // namespace HubPlatform
