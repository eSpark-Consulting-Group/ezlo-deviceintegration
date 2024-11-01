#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

class NetworkStateEvent : public EventBase {
public:
    using InterfaceId = std::string;
    NetworkStateEvent( InterfaceId interfaceId );
    std::string getMethodName() const override;

protected:
    const InterfaceId m_interfaceId;
};

class NetworkFailedStateEvent : public NetworkStateEvent {
public:
    using GetMainInterface = std::function< InterfaceId() >;
    NetworkFailedStateEvent( InterfaceId interfaceId );
    bool compare( const InterfaceId& interfaceId, bool isConnected );
    void setMainInterfaceCallback( GetMainInterface callback );

private:
    GetMainInterface m_getMainInterface;
};

class NetworkMainStateEvent : public NetworkStateEvent {
public:
    NetworkMainStateEvent( InterfaceId interfaceId );
    bool compare( const InterfaceId& interfaceId );
};

using NetworkFailedStateEventPtr = std::shared_ptr< NetworkFailedStateEvent >;
using NetworkMainStateEventPtr = std::shared_ptr< NetworkMainStateEvent >;

}  // namespace SceneEngine
