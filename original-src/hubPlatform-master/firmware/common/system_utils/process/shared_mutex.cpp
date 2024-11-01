#include "system_utils/process/shared_mutex.h"
#include "system_utils/process/shared_ram_exception.h"

namespace HubPlatform {
namespace Process {

SharedMutex::SharedMutex( const std::string& id )
: m_sharedRam( id, sizeof( pthread_mutex_t ) ) {
    m_mutexPtr = static_cast< pthread_mutex_t* >( m_sharedRam.get() );

    if( m_sharedRam.isJustInitialized() ) {
        pthread_mutexattr_t attr;
        if( pthread_mutexattr_init( &attr ) ) {
            throw SharedRamException( "pthread_mutexattr_init failure" );
        }
        if( pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED ) ) {
            throw SharedRamException( "pthread_mutexattr_setpshared failure" );
        }
        if( pthread_mutex_init( m_mutexPtr, &attr ) ) {
            throw SharedRamException( "pthread_mutex_init failure" );
        }
    }
}

void SharedMutex::lock() {
    pthread_mutex_lock( m_mutexPtr );
}

bool SharedMutex::try_lock() {
    return pthread_mutex_trylock( m_mutexPtr ) == 0;
}

void SharedMutex::unlock() {
    pthread_mutex_unlock( m_mutexPtr );
}

void SharedMutex::release() {
    if( pthread_mutex_destroy( m_mutexPtr ) ) {
        throw SharedRamException( "pthread_mutex_destroy failure" );
    }
    m_sharedRam.release();
}

}  // namespace Process
}  // namespace HubPlatform
