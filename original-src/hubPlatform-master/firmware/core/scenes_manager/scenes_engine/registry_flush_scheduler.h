#include "service_interfaces/task_loop_timer.h"

#include <chrono>
#include <atomic>

namespace HubPlatform {
namespace Services {
class TaskLoop;
}  // namespace Services
}  // namespace HubPlatform

namespace SceneEngine {

class StatesRegistry;

class RegistryFlushScheduler {
public:
    RegistryFlushScheduler( HubPlatform::Services::TaskLoop& taskLoop, StatesRegistry& registry );

    void schedule( std::chrono::seconds interval );
    void flushNow();

private:
    HubPlatform::Services::TaskLoopTimer m_timer;
    std::chrono::seconds m_interval{ 0 };
    std::atomic_bool m_isFlushing{ false };

    StatesRegistry& m_registry;
};

}  // namespace SceneEngine
