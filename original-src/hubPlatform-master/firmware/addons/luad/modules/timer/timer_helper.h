#pragma once

#include "luad/modules/timer/timer_fwd.h"

#include "json_utils/json_fwd.h"
#include "base_utils/badge.h"
#include "base_utils/non_copyable.h"

#include <string>
#include <memory>

namespace HubPlatform {
namespace Luad {

class TimerHelper : private common::NonCopyable {
public:
    using PrivateTag = Badge< TimerHelper >;
    using GroupName = std::string;

    TimerHelper( TimerManager& timerManager,
                 GroupName groupName,
                 TimerId&& timerId,
                 Services::PostponedTaskWPtr task,
                 std::string&& script,
                 Json&& scriptParams,
                 PrivateTag );

    static TimerHelperPtr create( TimerManager& timerManager,
                                  GroupName groupName,
                                  TimerId timerId,
                                  Services::PostponedTaskWPtr task,
                                  std::string script,
                                  Json scriptParams );

    ~TimerHelper() noexcept;

    void operator()();

    const Json& getScriptParams() const;

    void setScriptParams( Json params );

    Services::PostponedTaskWPtr getTask();

private:
    TimerManager& m_timerManager;
    const GroupName m_groupName;
    const TimerId m_timerId;
    Services::PostponedTaskWPtr m_task;
    const std::string m_script;
    Json m_scriptParams;
};

}  // namespace Luad
}  // namespace HubPlatform
