#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {
class FirmwareUpdateStateEvent : public EventBase {
public:
    using State = std::string;
    explicit FirmwareUpdateStateEvent( State state );

    std::string getMethodName() const override;

    bool compare( const State& state ) const;

private:
    State m_state;
};

using FirmwareUpdateStateEventPtr = std::shared_ptr< FirmwareUpdateStateEvent >;

}  // namespace SceneEngine
