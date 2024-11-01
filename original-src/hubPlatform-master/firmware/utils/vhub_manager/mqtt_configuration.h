#pragma once
#include <cstdlib>
#include <string>

namespace MqttConfiguration {

const char* getMqttIP();

int getMqttPort();

const char* getMqttUser();

const char* getMqttPassword();
}  // namespace MqttConfiguration

// ELOG_DEBUG("Mqtt port is %d",MQTT_PORT);