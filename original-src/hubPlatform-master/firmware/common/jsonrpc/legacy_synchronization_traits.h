#pragma once

#include <tiny/tiny_multithreading.h>

namespace rpc {

template < typename T >
class LegacySynchronizationTraits {
public:
    typedef tiny::multithreading::recursive_mutex_type mutex_type;
    typedef std::lock_guard< mutex_type > lock_type;
    typedef std::shared_ptr< T > stored_t;

    static mutex_type& mtx() {
        static mutex_type g_mutex( "RMTX-ATOMIC-SHARED-LEGACY-SYNC-TRAITS" );
        return g_mutex;
    }
};

}  // namespace rpc