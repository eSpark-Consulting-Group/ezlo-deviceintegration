#include "mqtt_configuration.h"

namespace MqttConfiguration {

namespace EnvironmentKeys {
const char* const MQTT_IP = "MQTT_IP";
const char* const MQTT_PORT = "MQTT_PORT";
const char* const MQTT_USER = "MQTT_USER";
const char* const MQTT_PASSWORD = "MQTT_PASSWORD";
}  // namespace EnvironmentKeys

namespace DefaultValues {
const char* const MQTT_IP = "tcp://127.0.0.1";
const char* const MQTT_PORT = "51883";
const char* const MQTT_USER = "";
const char* const MQTT_PASSWORD = "";
}  // namespace DefaultValues

const char* getMqttIP() {
    const char* buff = getenv( EnvironmentKeys::MQTT_IP );
    return buff ? buff : DefaultValues::MQTT_IP;
}

int getMqttPort() {
    const char* buff = getenv( EnvironmentKeys::MQTT_PORT );
    const char* value = buff ? buff : DefaultValues::MQTT_PORT;
    return std::stoi( value );
}

const char* getMqttUser() {
    const char* buff = getenv( EnvironmentKeys::MQTT_USER );
    return buff ? buff : DefaultValues::MQTT_USER;
}

const char* getMqttPassword() {
    const char* buff = getenv( EnvironmentKeys::MQTT_PASSWORD );
    return buff ? buff : DefaultValues::MQTT_PASSWORD;
}
}  // namespace MqttConfiguration

// ELOG_DEBUG("Mqtt port is %d",MQTT_PORT);
