#pragma once

#include "service_interfaces/task_loop_timer.h"
#include <atomic>

namespace HubPlatform {

namespace Services {
class TaskLoop;
}

namespace CoreD {

class ItemDbModifier;
class DeviceDbModifier;

class PermanentDBWriteScheduler {
public:
    PermanentDBWriteScheduler( Services::TaskLoop& loop,
                               ItemDbModifier& itemDbModifier,
                               DeviceDbModifier& deviceDbModifier );
    void doNow();
    void schedule( const Services::TaskLoopTimer::Interval& interval );
    void cancelSchedule();

private:
    void worker();
    void updateItems();
    void updateDevices();
    void updateCommandTimestamps();

private:
    ItemDbModifier& m_itemDbModifier;
    DeviceDbModifier& m_deviceDbModifier;

    Services::TaskLoopTimer m_timer;

    std::atomic_bool m_isRunning{ false };
};

}  // namespace CoreD
}  // namespace HubPlatform
