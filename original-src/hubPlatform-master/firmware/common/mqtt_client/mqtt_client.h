#pragma once

#include "mqtt_client/mqtt_client_interface.h"

#include <memory>
#include <set>
#include <atomic>
#include <mutex>
#include <thread>

namespace mqtt {
class async_client;
}  // namespace mqtt

namespace HubPlatform {
namespace Common {

namespace MQTTCallback {
class MQTTCallback;
}  // namespace MQTTCallback

class MQTTClient : public MQTTClientInterface {
public:
    MQTTClient();
    ~MQTTClient();
    virtual void connect( std::string clientID,
                          std::string serverURI,
                          std::string username,
                          std::string password,
                          MQTTMessageCallback messageCallback ) override;
    virtual void disconnect() override;
    virtual void subscribe( const std::string& topic ) override;
    virtual void unsubscribe( const std::string& topic ) override;
    virtual void publish( const MQTTMessage& message ) override;
    virtual void publish( const std::string& topic, const std::string& payload ) override;
    virtual bool isConnected() override;

private:
    void onConnected( const std::string& reason );
    void onConnectionLost( const std::string& reason );
    void resubscribe();

private:
    std::unique_ptr< mqtt::async_client > m_client;
    std::unique_ptr< MQTTCallback::MQTTCallback > m_callback;
    MQTTMessageCallback m_messageCallback;

    std::atomic_bool m_isConnected{ false };

    std::mutex m_subscriptionsMutex;
    std::set< std::string > m_subscriptions;
    std::thread m_subscribeThread;
};

}  // namespace Common
}  // namespace HubPlatform
