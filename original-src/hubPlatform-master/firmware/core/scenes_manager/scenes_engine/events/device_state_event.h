#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

class DeviceStateEvent : public EventBase {
public:
    using DeviceStateParams = std::map< std::string, JsonObject >;

    DeviceStateEvent( DeviceStateParams params );
    std::string getMethodName() const override;
    bool compare( const JsonObject& deviceInfo );
    void setKeepState( bool value );

private:
    bool checkDeviceInfo( const JsonObject& deviceInfo );

private:
    bool m_triggered = { false };
    DeviceStateParams m_params;
    JsonObject m_cachedDeviceInfo;
    bool m_keepState{ true };
};

using DeviceStateEventPtr = std::shared_ptr< DeviceStateEvent >;

}  // namespace SceneEngine
