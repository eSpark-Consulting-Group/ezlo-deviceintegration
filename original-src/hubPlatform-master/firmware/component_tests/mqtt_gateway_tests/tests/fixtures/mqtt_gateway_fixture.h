#include "test_utils/component_tests/common_nma_test_fixture.h"

#include "mqtt_client/mqtt_client.h"
#include "json_utils/json_wrapper.h"

#include <memory>

namespace HubPlatform {
namespace Tests {

class MQTTGatewayTestsApplication;

class MQTTGatewayFixtureError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class MQTTGatewayFixture : public testing::Test {
public:
    using MQTTClient = HubPlatform::Common::MQTTClient;
    using MQTTMessageCallback = std::function< void( JsonObject&& ) >;

    void SetUp() override;
    void TearDown() override;

    MQTTGatewayTestsApplication& app();
    MQTTClient& mqttClient();

    void sendMQTTMessage( const std::string& topic, const JsonObject& msg );
    bool waitForMQTTMessage();
    JsonObject popMQTTMessage();
    void clearMQTTMessageQueue();

private:
    void setupMQTTClient();

private:
    std::mutex m_mutex;
    std::unique_ptr< MQTTClient > m_mqttClient;
    std::deque< JsonObject > m_mqttMessages;
};

}  // namespace Tests
}  // namespace HubPlatform
