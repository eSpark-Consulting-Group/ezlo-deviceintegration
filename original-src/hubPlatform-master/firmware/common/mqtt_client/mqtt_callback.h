#pragma once

#include "mqtt/callback.h"

#include <functional>

namespace HubPlatform {
namespace Common {
namespace MQTTCallback {

class MQTTCallback : public mqtt::callback {
public:
    using Callback = std::function< void( const std::string& ) >;

    MQTTCallback( Callback onConnected, Callback onConnectionLost );

    void connected( const std::string& reason );
    void connection_lost( const std::string& reason );

private:
    Callback m_onConnectedCb;
    Callback m_onConnectionLostCb;
};

}  // namespace MQTTCallback
}  // namespace Common
}  // namespace HubPlatform
