#pragma once

#include <string>
#include <memory>
#include <stdexcept>

namespace HubPlatform {
namespace filesystem {

class MountPointError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit MountPointError( const std::string& msg )
    : runtime_error( msg ){};
};

// NOTE: mountpoint path sould point to temporary directory that will be automatically created/removed
class MountPoint {
public:
    static std::unique_ptr< MountPoint > mount( const std::string device, std::string mountPoint );
    ~MountPoint();

private:
    MountPoint( std::string mountPoint );

private:
    const std::string m_mountPoint;
};

}  // namespace filesystem
}  // namespace HubPlatform
