#pragma once

#include "cloud_devices/remote_interface/cloud_devices_base_remote_interface.h"
#include "service_interfaces/service.h"

#include <memory>

namespace HubPlatform {
namespace Services {
class Service;
}
namespace CloudDevices {

class RemoteInterface final : public BaseRemoteInterface {
public:
    RemoteInterface( Services::Service& hostService );

    JsonObject createCloudSubscription( const JsonObject& params ) override;
    JsonObject updateCloudSubscription( const JsonObject& params ) override;
    JsonObject deleteCloudSubscription( const JsonObject& params ) override;
    JsonObject dispatchCloudSubscription( const JsonObject& params ) override;

    JsonObject getCloudSubscriptionsList() override;

    std::vector< MeshbotTemplateRecord > getMeshbotTemplatesList() override;
    std::vector< MeshbotTemplateRecord > getMeshbotTemplatesList( int meshbotsVersion, int fileVersion ) override;
    Json getLocalNotificationDataFiles() override;

    bool switchCloudDataVersion( int cloudDataVersion, int fileVersion ) override;
    bool switchMeshbotsVersion( int meshbotsVersion, int fileVersion ) override;

private:
    LocalNotificationDataRecord getCloudDataInfo();
    LocalNotificationDataRecord getMeshbotDataInfo();

private:
    Services::Service& m_hostService;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
