#include "mqtt_gateway_tests/application.h"

#include "mqtt_gateway_tests/tests/resources/mqtt_gateway_resources.h"

#include "test_utils/test_environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/process/shared_mutex.h"
#include "system_utils/find_free_local_port.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/message.h"
#include "common_utils/cpp14.h"

#include <stdlib.h>

namespace HubPlatform {
namespace Tests {

MQTTGatewayTestsApplication* MQTTGatewayTestsApplication::m_instance = nullptr;

MQTTGatewayTestsApplication::MQTTGatewayTestsApplication()
: TestApplication( Tests::getComponentTestsFolder() + "/mqtt_gateway_tests/root" ) {
    m_instance = this;
    m_mqttBrokerPort = getRandomPort();
}

void MQTTGatewayTestsApplication::setUp() {
    TestApplication::setUp();
    setenv( "RUNNING_COMPONENT_TESTS", "1", 1 );
    LOGGER_DEBUG( "Use MQTT Broker port: " + m_mqttBrokerPort );
    startMQTTBroker();

    setenv( "MQTT_BROKER_PORT", m_mqttBrokerPort.c_str(), true /*overwrite*/ );
    executeAddonService( "ha-mqtt_gatewayd", "mqtt_gateway" );
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
}

std::string MQTTGatewayTestsApplication::getRandomPort() {
    Process::SharedMutex processMutex( "uniquePortMutex" );
    std::unique_lock< Process::SharedMutex > lock( processMutex );
    auto port = system_utils::getUniqueFreeLocalPort();
    lock.unlock();
    return std::to_string( port );
}

void MQTTGatewayTestsApplication::startMQTTBroker() {
    execute( Tests::getBuildBinFolder() + "/" + "mosquitto",
             { "--port", m_mqttBrokerPort, "--verbose" },
             getLogFolderPath() + "/mosquitto.log" );
}

const std::string& MQTTGatewayTestsApplication::mqttBrokerPort() {
    return m_mqttBrokerPort;
}

MQTTGatewayTestsApplication::~MQTTGatewayTestsApplication() {
    m_instance = nullptr;
}

MQTTGatewayTestsApplication& MQTTGatewayTestsApplication::get() {
    if( m_instance != nullptr ) {
        return *m_instance;
    }
    throw TestApplicationError( "MQTTGatewayTestApplication instance is not initialized!" );
}

void MQTTGatewayTestsApplication::setRequestHandler( MessageSubClasses messageSubclass, RequestHandler handler ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_requestHandlers.emplace( messageSubclass, std::move( handler ) );
}

void MQTTGatewayTestsApplication::setEventHandler( MessageSubClasses messageSubclass, EventHandler handler ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_eventHandlers.emplace( messageSubclass, std::move( handler ) );
}

JsonObject MQTTGatewayTestsApplication::handleRequestFromBus( const JsonObject& request,
                                                              MessageSubClasses requestType ) {
    try {
        std::lock_guard< std::mutex > lock( m_mutex );
        auto it = m_requestHandlers.find( requestType );
        if( it != m_requestHandlers.cend() ) {
            return it->second( request );
        }
    }
    catch( const std::exception& ex ) {
        const std::string reason = ex.what();
        LOGGER_ERROR_RAW( cc::error( "request error: " ) + cc::attention( reason ) );
        JsonObject answer;
        setError( answer, Errors::WRONG_PARAMS, reason );
        return answer;
    }
    return TestApplication::handleRequestFromBus( request, requestType );
}

void MQTTGatewayTestsApplication::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    if( message.msgClass == EVENT ) {
        std::lock_guard< std::mutex > lock( m_mutex );
        auto it = m_eventHandlers.find( static_cast< MessageSubClasses >( message.msgSubClass ) );
        if( it != m_eventHandlers.cend() ) {
            it->second( message.data );
            return;
        }
    }
    return TestApplication::handleMessageFromBus( message, sender );
}

}  // namespace Tests
}  // namespace HubPlatform
