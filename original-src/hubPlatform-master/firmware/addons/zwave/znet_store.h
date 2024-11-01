#pragma once

#include <string>

namespace HubPlatform {
namespace Zwave {

class ZNetStore {
public:
    ZNetStore( const std::string& storePath );

    bool save( std::size_t offset, const void* data, std::size_t size ) const;
    bool load( std::size_t offset, void* data, std::size_t size ) const;
    bool reset() const;
    std::string file();
    std::string path();

private:
    const std::string m_storePath;
    const std::string m_storeFile;
};

}  // namespace Zwave
}  // namespace HubPlatform
