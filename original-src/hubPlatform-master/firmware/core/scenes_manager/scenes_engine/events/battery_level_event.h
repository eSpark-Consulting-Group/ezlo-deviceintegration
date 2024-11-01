#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/events/item_value_wrapper.h"

namespace SceneEngine {
class BatteryLevelEvent : public EventBase {
public:
    using Comparator = std::function< bool( const ItemValueWrapper&, const ItemValueWrapper& ) >;
    BatteryLevelEvent( Comparator comparator, const ItemValueWrapper& itemValue );

    std::string getMethodName() const override;

    bool compare( const int level ) const;

private:
    ItemValueWrapper m_batteryLevel;
    Comparator m_comparator;
};

using BatteryLevelEventPtr = std::shared_ptr< BatteryLevelEvent >;

}  // namespace SceneEngine
