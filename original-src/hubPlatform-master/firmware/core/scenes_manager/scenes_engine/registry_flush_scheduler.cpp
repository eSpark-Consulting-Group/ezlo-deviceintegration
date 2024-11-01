#include "scenes_manager/scenes_engine/registry_flush_scheduler.h"
#include "scenes_manager/scenes_engine/states_registry.h"

#include "service_interfaces/task_loop.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace SceneEngine {

RegistryFlushScheduler::RegistryFlushScheduler( HubPlatform::Services::TaskLoop& taskLoop, StatesRegistry& registry )
: m_registry( registry )
, m_timer( taskLoop, [this] { flushNow(); } ) {}

void RegistryFlushScheduler::schedule( std::chrono::seconds interval ) {
    m_interval = interval;
    m_timer.start( m_interval, true );
}

void RegistryFlushScheduler::flushNow() {
    bool expected = false;
    if( !m_isFlushing.compare_exchange_strong( expected, true ) ) {
        LOGGER_DEBUG( cc::debug( "States registry is already flushing, nothing to do" ) );
        return;
    }

    struct Guard {
        using ValueType = decltype( m_isFlushing );

        Guard( ValueType& val )
        : val( val ) {}

        ~Guard() {
            val = false;
        }
        ValueType& val;
    };
    Guard guard( m_isFlushing );

    m_registry.flushToPersistentMemory();
    m_timer.start( m_interval, true );
}

}  // namespace SceneEngine
