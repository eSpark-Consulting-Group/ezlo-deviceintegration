#pragma once

#if __HUB_TYPE_VHUB__

#include "vidoo4hub/vidoo_client/vidoo_client_interface.h"
#include "service_interfaces/service.h"
#include "mqtt_client/mqtt_client.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class VidooMQTTClient : public IVidooClient {
public:
    VidooMQTTClient( Services::Service& service, MessageCallback callback );

    void setUp() override;
    void tearDown() override;
    void sendMessage( const JsonObject& msg ) override;

private:
    void connect();

private:
    Services::Service& m_service;
    MessageCallback m_callback;
    Common::MQTTClient m_mqttClient;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform

#endif  // __HUB_TYPE_VHUB__
