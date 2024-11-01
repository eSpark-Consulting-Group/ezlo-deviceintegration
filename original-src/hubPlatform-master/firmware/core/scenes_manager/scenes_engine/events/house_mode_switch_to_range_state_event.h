#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

class HouseModeSwitchToRangeEvent : public EventBase {
public:
    std::string getMethodName() const override;
    bool compare( const std::string& status );

private:
    bool m_savedPhase = false;
};

using HouseModeSwitchToRangeEventPtr = std::shared_ptr< HouseModeSwitchToRangeEvent >;

}  // namespace SceneEngine
