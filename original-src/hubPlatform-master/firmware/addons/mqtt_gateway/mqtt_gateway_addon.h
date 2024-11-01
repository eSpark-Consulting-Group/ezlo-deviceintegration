#pragma once
#include "addon_interfaces/addon.h"

#include "mqtt_gateway/mqtt_client/mqtt_client_interface.h"
#include "service_interfaces/bus_wrapper.h"

#include <string>
#include <set>

namespace HubPlatform {

class MQTTGatewayAddon final : public Addons::Addon {
public:
    MQTTGatewayAddon();
    ~MQTTGatewayAddon() override;

    CommonUtils::AddonVersion getVersion() const override;

private:
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;

private:
    void handleMQTTMessage( JsonObject&& msg );
    Json handleMQTTRequest( const std::string& sender,
                            const std::string& messageId,
                            const std::string& messageSubclass,
                            Json& params );
    Json handleClientRegister( const Json& params );
    Json handleClientUnregister( const Json& params );
    Json handleHubDeviceSettingsList( const Json& params );

    void forwardToSpread( Json mqttMessage );
    bool forwardToMQTT( const bus_rx_data_t* rxData );

private:
    void setUp() override;
    void tearDown() override;

private:
    std::unique_ptr< MQTTGateway::MQTTClientInterface > m_mqttClient;
    std::set< std::string > m_registeredClients;
};

}  // namespace HubPlatform
