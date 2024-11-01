#pragma once

#include "mqtt_gateway/mqtt_client/mqtt_client_interface.h"
#include "mqtt_client/mqtt_client.h"
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace MQTTGateway {

class MQTTClient : public MQTTClientInterface {
public:
    MQTTClient( Services::Service& service, MessageCallback callback );

    void setUp() override;
    void tearDown() override;
    void sendMessage( const std::string& topic, const JsonObject& msg ) override;

private:
    void connect();
    const std::string& getBrokerURI();

private:
    Services::Service& m_service;
    MessageCallback m_callback;
    Common::MQTTClient m_mqttClient;
};

}  // namespace MQTTGateway
}  // namespace HubPlatform
