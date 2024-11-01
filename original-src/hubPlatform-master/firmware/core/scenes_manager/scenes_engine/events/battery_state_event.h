#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {
class BatteryStateEvent : public EventBase {
public:
    using BatteryState = std::string;
    explicit BatteryStateEvent( BatteryState state );

    std::string getMethodName() const override;

    bool compare( const std::string& state ) const;

private:
    BatteryState m_batteryState;
};

using BatteryStateEventPtr = std::shared_ptr< BatteryStateEvent >;

}  // namespace SceneEngine
