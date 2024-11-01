#pragma once

#include "luad/modules/timer/timer_fwd.h"

#include "binding_interfaces/common/plugin_status_events_subscription.h"
#include "json_utils/json_fwd.h"
#include "base_utils/non_copyable.h"
#include "base_utils/badge.h"

#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>

namespace HubPlatform {
namespace LuaAddon {

class EventEmitter;

}  // namespace LuaAddon
}  // namespace HubPlatform

namespace HubPlatform {
namespace Luad {

namespace UT {
class TimeManagerTestSuite;
}

class TimerManager : private common::NonCopyable {
    friend class UT::TimeManagerTestSuite;

public:
    TimerManager( LuaAddon::EventEmitter& eventEmitter,
                  Services::TaskLoop& taskLoop,
                  IPluginStatusEventsSubscription& pluginStatusEventsSubscription );
    ~TimerManager();

    TimerId setTimer( GroupName groupName,
                      TimerDuration delay,
                      std::string script,
                      Json scriptParams,
                      TimerType timerType );

    void setTimerWithId( GroupName groupName,
                         TimerId timerId,
                         TimerDuration delay,
                         std::string script,
                         Json scriptParams,
                         TimerType timerType );

    void stopTimer( const GroupName& groupName, const TimerId& timerId );

    void restartTimer( const GroupName& groupName, const TimerId& timerId, TimerDuration delay );

    bool hasTimer( const GroupName& groupName, const TimerId& timerId ) const;

    const Json* getTimerParameters( const GroupName& groupName, const TimerId& timerId ) const;

    void setTimerParameters( const GroupName& groupName, const TimerId& timerId, Json parameters );

    LuaAddon::EventEmitter& getEventEmitter();

    void registerTimer( Badge< TimerHelper >,
                        const GroupName& groupName,
                        const TimerId& id,
                        TimerHelperWPtr timerHelper );
    void unregisterTimer( Badge< TimerHelper >, const GroupName& groupName, const TimerId& id );

    void stopTimersGroup( const GroupName& groupName );

private:
    TimerId generateUniqueId() const;
    TimerHelperPtr getTimerHelper( const GroupName& groupName, const TimerId& timerId ) const;

private:
    LuaAddon::EventEmitter& m_eventEmitter;
    Services::TaskLoop& m_taskLoop;

    using Timers = std::unordered_map< TimerId, TimerHelperWPtr >;
    using TimersGroups = std::unordered_map< GroupName, Timers >;

    TimersGroups m_timers;

    mutable std::mutex m_mutex;
};

}  // namespace Luad
}  // namespace HubPlatform
