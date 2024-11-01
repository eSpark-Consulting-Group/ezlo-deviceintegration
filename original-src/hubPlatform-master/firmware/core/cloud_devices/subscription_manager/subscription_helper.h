#pragma once

#include "cloud_devices/subscription_manager/subscription_helper_interface.h"

#include "service_interfaces/postponed_task.h"

#include <mutex>
#include <list>

namespace HubPlatform {

namespace Services {
class Service;
}  // namespace Services

namespace CoreDRemoteInterface {
class CoreDInterface;
}  // namespace CoreDRemoteInterface

namespace CloudDevices {

class SubscriptionHelper : public ISubscriptionHelper {
public:
    SubscriptionHelper( Services::Service& hostService, CoreDRemoteInterface::CoreDInterface& core );
    ~SubscriptionHelper();

    Result addDevice( const CoreD::Model::Device& device ) override;
    Result addItem( const CoreD::Model::Item& item ) override;

    void modifyItem( const std::string& itemId, const CoreD::Model::Item& item ) override;

    void removeDevice( const Id& deviceId ) override;
    void removeItem( const Id& itemId ) override;

    Json getItem( const Id& itemId ) override;
    Json getDevice( const Id& deviceId ) override;
    std::string getGatewayId() override;
    Json getGatewayByName( const std::string& gatewayName ) override;
    Json getGatewayById( const std::string& gatewayId ) override;
    bool itemBelongsToGateway( const std::string& itemId, const std::string& gatewayName ) override;

    void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) override;

    void sendSubscriptionList( Json message ) override;

    void tearDown() override;

private:
    void sendSubscriptionList();

private:
    Services::Service& m_service;
    CoreDRemoteInterface::CoreDInterface& m_core;
    std::string m_gatewayId;

    std::recursive_mutex m_heapMutex;
    std::list< Json > m_messageHeap;
    Services::PostponedTaskPtr m_resendSubscriptionListTask;
    size_t m_retries = 0;
    bool m_waitingResponse = false;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
