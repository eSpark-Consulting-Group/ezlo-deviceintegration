#pragma once

#include "cloud_devices/local_notifications/types/meshbot_template_record.h"
#include "cloud_devices/local_notifications/types/local_notifiction_data_record.h"
#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

#include <vector>

namespace HubPlatform {
namespace CloudDevices {

class BaseRemoteInterface {
public:
    virtual ~BaseRemoteInterface() = default;

    virtual JsonObject createCloudSubscription( const JsonObject& params ) = 0;
    virtual JsonObject deleteCloudSubscription( const JsonObject& params ) = 0;
    virtual JsonObject updateCloudSubscription( const JsonObject& params ) = 0;
    virtual JsonObject dispatchCloudSubscription( const JsonObject& params ) = 0;
    virtual JsonObject getCloudSubscriptionsList() = 0;
    virtual std::vector< MeshbotTemplateRecord > getMeshbotTemplatesList() = 0;
    virtual std::vector< MeshbotTemplateRecord > getMeshbotTemplatesList( int meshbotsVersion, int fileVersion ) = 0;
    virtual Json getLocalNotificationDataFiles() = 0;

    virtual bool switchCloudDataVersion( int cloudDataVersion, int fileVersion ) = 0;
    virtual bool switchMeshbotsVersion( int meshbotsVersion, int fileVersion ) = 0;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
