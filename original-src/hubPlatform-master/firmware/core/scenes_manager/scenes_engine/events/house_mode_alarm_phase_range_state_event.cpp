#include "scenes_manager/scenes_engine/events/house_mode_alarm_phase_range_state_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "cored/model/resources/cored_resources.h"

#include "log_utils/ezlo_log.h"

namespace SceneEngine {

using namespace HubPlatform::CoreD::Model::Resources;
HouseModeAlarmedPhaseRangeEvent::HouseModeAlarmedPhaseRangeEvent( const std::string& phase )
: m_phase( phase ) {}

std::string HouseModeAlarmedPhaseRangeEvent::getMethodName() const {
    return Model::Resources::MethodNames::IsHouseModeAlarmPhaseRange;
}

bool HouseModeAlarmedPhaseRangeEvent::compare( const std::string& phase, const std::string& status ) {
    std::string comparedPhase = phase;
    if( phase == HouseModeAlarmStatus::EntryDelay ) {
        if( status == HouseModeAlarmStatus::Begin ) {
            comparedPhase = HouseModeAlarmPhase::EntryDelay;
        }
        else if( status == HouseModeAlarmStatus::Canceled ) {
            comparedPhase = HouseModeAlarmPhase::Idle;
        }
    }
    else if( phase == HouseModeAlarmStatus::Main ) {
        comparedPhase = HouseModeAlarmPhase::Main;
    }

    bool isCompared = false;

    if( !comparedPhase.empty() ) {
        isCompared = !phase.empty() && comparedPhase != m_savedPhase && comparedPhase == m_phase;
        m_savedPhase = comparedPhase;
    }
    return isCompared;
}

}  // namespace SceneEngine
