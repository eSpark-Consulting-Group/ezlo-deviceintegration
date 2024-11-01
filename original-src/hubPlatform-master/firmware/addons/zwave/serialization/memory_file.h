#pragma once

#include <vector>

#include <znet/IFile.hpp>

namespace HubPlatform {
namespace Zwave {

class MemoryFile : public IFile {
public:
    MemoryFile();
    MemoryFile( const char* rawData, size_t len );

    bool read( void* dst, size_t len ) override;
    bool write( const void* src, size_t len ) override;

    const std::vector< char >& getData() const;

private:
    std::size_t m_readPos;
    std::vector< char > m_data;
};

}  // namespace Zwave
}  // namespace HubPlatform
