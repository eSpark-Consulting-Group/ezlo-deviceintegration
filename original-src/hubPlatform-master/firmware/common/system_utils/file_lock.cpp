#include "system_utils/file_lock.h"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

extern "C" {
#include <sys/file.h>
}

namespace HubPlatform {
namespace filesystem {

FileLock::FileLock( std::string lockPath )
: m_lockPath( std::move( lockPath ) ) {
    if( m_lockPath.empty() ) {
        throw FileLockError( "Empty lock path" );
    }

    m_fd = open( m_lockPath.c_str(), O_RDWR | O_CREAT, 0666 );
    if( m_fd <= 0 )
        throw FileLockError( "Fail to open lock file: " + m_lockPath +
                             " , error: " + std::string( std::strerror( errno ) ) );

    m_isOpened = true;
}

FileLock::~FileLock() {
    if( m_isLocked ) {
        try {
            unlock();
        }
        catch( const FileLockError& e ) {
            std::cerr << e.what() << std::endl;
        }
    }

    if( m_isOpened )
        close( m_fd );
}

void FileLock::lock() {
    if( m_isLocked )
        throw FileLockError( "Fail to lock file: " + m_lockPath + " , lock is already acquired" );

    if( flock( m_fd, LOCK_EX ) == -1 ) {
        throw FileLockError( "Fail to lock file: " + m_lockPath + " , flock() failed" );
    }
    m_isLocked = true;
}

bool FileLock::tryLock() {
    if( m_isLocked )
        return false;

    if( flock( m_fd, LOCK_EX | LOCK_NB ) == -1 )
        return false;

    m_isLocked = true;
    return m_isLocked;
}

void FileLock::unlock() {
    if( !m_isLocked )
        return;

    if( flock( m_fd, LOCK_UN ) == -1 )
        throw FileLockError( "Fail to unlock file: " + m_lockPath +
                             " , error: " + std::string( std::strerror( errno ) ) );
    m_isLocked = false;
}

}  // namespace filesystem
}  // namespace HubPlatform
