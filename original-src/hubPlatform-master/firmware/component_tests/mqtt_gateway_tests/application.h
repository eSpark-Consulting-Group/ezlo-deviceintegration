#pragma once

#include "test_utils/component_tests/test_application.h"

#include <functional>

namespace HubPlatform {
namespace Tests {

class MQTTGatewayTestsApplication : public TestApplication {
public:
    using RequestHandler = std::function< JsonObject( const JsonObject& ) >;
    using EventHandler = std::function< void( const JsonObject& ) >;
    MQTTGatewayTestsApplication();
    ~MQTTGatewayTestsApplication();

    static MQTTGatewayTestsApplication& get();

    void setRequestHandler( MessageSubClasses messageSubclass, RequestHandler handler );
    void setEventHandler( MessageSubClasses messageSubclass, EventHandler handler );

    const std::string& mqttBrokerPort();

protected:
    void setUp() override;
    void startMQTTBroker();
    std::string getRandomPort();

    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleMessageFromBus( const Services::Message& message, const std::string& sender ) override;

private:
    static MQTTGatewayTestsApplication* m_instance;

    std::string m_mqttBrokerPort;

    std::mutex m_mutex;
    std::map< MessageSubClasses, RequestHandler > m_requestHandlers;
    std::map< MessageSubClasses, EventHandler > m_eventHandlers;
};

}  // namespace Tests
}  // namespace HubPlatform
