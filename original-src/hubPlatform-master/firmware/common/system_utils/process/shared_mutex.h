#pragma once

#include "system_utils/process/shared_ram.h"

//#define _BSD_SOURCE // for ftruncate
#include <pthread.h>  // pthread_mutex_t, pthread_mutexattr_t,
                      // pthread_mutexattr_init, pthread_mutexattr_setpshared,
                      // pthread_mutex_init, pthread_mutex_destroy

namespace HubPlatform {
namespace Process {

/// @brief The shared mutex with given id across different processes
/// In case of error of shared mutex creating, the exception throws.
/// The destructor unmaps shared mutex but shared mutex lives even after process overs
/// For shared mutex release, there should not be any other mutexes in other processes overwise crashes could be
/// In case when the mutex appears to be uncreated,
/// this function becomes *non-thread-safe*. If multiple threads
/// call it at one moment, there occur several race conditions,
/// in which one call might recreate another's shared memory
/// object or rewrite another's pthread mutex in the shared memory.
/// There is no workaround currently, except to run first
/// initialization only before multi-threaded or multi-process
/// functionality.
///
/// Useful links:
/// https://opensource.com/article/19/4/interprocess-communication-linux-storage
/// https://gist.github.com/yamnikov-oleg/abf61cf96b4867cbf72d
/// https://gist.github.com/walchko/6d211a2dc8b2f5158642fb2fd65cf295
class SharedMutex final {
public:
    /// @brief create a new shared mutex. In case of error, the exception throws

    /// @param id of shared mutex
    SharedMutex( const std::string& id );

    /// @brief lock shared mutex across the processes
    void lock();

    /// @brief lock shared mutex across the processes
    /// @return true in case of succesful lock of mutex
    bool try_lock();

    /// @brief unlock shared mutex across the processes
    void unlock();

    /// @brief release shared mutex.
    /// There should not be any other mutexes in other processes overwise crashes could be
    void release();

private:
    SharedRam m_sharedRam;
    pthread_mutex_t* m_mutexPtr;
    std::size_t* m_counterPtr;
};

}  // namespace Process
}  // namespace HubPlatform
