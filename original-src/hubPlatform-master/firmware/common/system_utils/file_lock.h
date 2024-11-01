#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace filesystem {

class FileLockError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit FileLockError( const std::string& msg )
    : runtime_error( msg ){};
};

class FileLock {
public:
    FileLock( std::string lockPath );
    ~FileLock();

    void lock();
    bool tryLock();
    void unlock();

private:
    const std::string m_lockPath;
    int m_fd = -1;
    bool m_isOpened = false;
    bool m_isLocked = false;
};

}  // namespace filesystem
}  // namespace HubPlatform
