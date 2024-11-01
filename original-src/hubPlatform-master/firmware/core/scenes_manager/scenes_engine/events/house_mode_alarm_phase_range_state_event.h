#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

class HouseModeAlarmedPhaseRangeEvent : public EventBase {
public:
    HouseModeAlarmedPhaseRangeEvent( const std::string& phase );
    std::string getMethodName() const override;
    bool compare( const std::string& phase, const std::string& status );

private:
    std::string m_phase = "";
    std::string m_savedPhase = "";
};

using HouseModeAlarmPhaseRangeEventPtr = std::shared_ptr< HouseModeAlarmedPhaseRangeEvent >;

}  // namespace SceneEngine
