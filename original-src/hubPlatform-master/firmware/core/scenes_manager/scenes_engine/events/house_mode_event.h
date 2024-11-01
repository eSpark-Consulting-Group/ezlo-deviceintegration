#pragma once

#include "json_utils/json_fwd.h"
#include "scenes_manager/scenes_engine/events/event_base.h"

#include <string>
#include <set>

namespace SceneEngine {
class HouseModeEvent : public EventBase {
public:
    using Modes = std::set< std::string >;

    explicit HouseModeEvent( Modes modes );

    std::string getMethodName() const override;

    bool compare( const std::string& mode ) const;

private:
    Modes m_modes;
};

using HouseModeEventPtr = std::shared_ptr< HouseModeEvent >;

}  // namespace SceneEngine
