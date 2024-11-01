#include "mqtt_client/mqtt_callback.h"

namespace HubPlatform {
namespace Common {
namespace MQTTCallback {

MQTTCallback::MQTTCallback( Callback onConnected, Callback onConnectionLost )
: m_onConnectedCb( std::move( onConnected ) )
, m_onConnectionLostCb( std::move( onConnectionLost ) ) {}

void MQTTCallback::connected( const std::string& reason ) {
    m_onConnectedCb( reason );
};

void MQTTCallback::connection_lost( const std::string& reason ) {
    m_onConnectionLostCb( reason );
};

}  // namespace MQTTCallback
}  // namespace Common
}  // namespace HubPlatform
