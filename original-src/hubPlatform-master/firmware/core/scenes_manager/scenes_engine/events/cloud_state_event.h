#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

class CloudStateEvent : public EventBase {
public:
    enum class State { Connected, Disconnected };

    explicit CloudStateEvent( State state );

    std::string getMethodName() const override;
    bool compare( bool isCloudConnected ) const;

private:
    State m_expectedState;
};

using CloudStateEventPtr = std::shared_ptr< CloudStateEvent >;

}  // namespace SceneEngine
