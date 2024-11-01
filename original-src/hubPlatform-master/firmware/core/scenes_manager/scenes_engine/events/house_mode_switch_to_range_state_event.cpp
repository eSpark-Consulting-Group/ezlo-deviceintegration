#include "scenes_manager/scenes_engine/events/house_mode_switch_to_range_state_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "cored/model/resources/cored_resources.h"

#include "log_utils/ezlo_log.h"

namespace SceneEngine {

using namespace HubPlatform::CoreD::Model::Resources;

std::string HouseModeSwitchToRangeEvent::getMethodName() const {
    return Model::Resources::MethodNames::IsHouseModeAlarmSwitchToRange;
}

bool HouseModeSwitchToRangeEvent::compare( const std::string& status ) {
    bool isCompared = false;

    if( status == HouseModeAlarmStatus::Begin && m_savedPhase == false ) {
        isCompared = true;
    }

    m_savedPhase = status == HouseModeAlarmStatus::Begin;

    return isCompared;
}

}  // namespace SceneEngine
