#pragma once

#include "provision_data_utils/provision_data.h"
#include "provision_data_utils/provision_data_manager.h"

#include "service_interfaces/task_loop.h"
#include "json_utils/json_utils.h"
#include "mqtt_client/mqtt_client.h"

#include <memory>

namespace HubPlatform {
namespace VhubManager {

class VhubManager {
    enum class Status { Unknown, FirmwareStarted, FirmwareStopped, ProvisionDataNotFound };

public:
    void run();

private:
    void mqttConnect();
    void handleMQTTMessage( const Common::MQTTMessage& msg );

    void handleProvisionInfoGet( Json& msg );
    void handleProvisionDataNew( Json& msg );
    void handleProvisionDataDelete( Json& msg );
    void handleVhubFirmwareStop( Json& msg );
    void handleVhubFirmwareStart( Json& msg );
    void handleVhubStatusGet( Json& msg );

    void setMsgResponse( Json& msg, Json response );
    void setMsgError( Json& msg, std::string errorMsg, int code = -1 );

private:
    void init();
    void startFirmware();
    void stopFirmware();

    Json getProvisionDataInfo();
    void prepareProvisioning();
    void processNewProvisioning( Json provisioningJson );
    ProvisionDataUtils::ProvisionData loadProvisionData();

    void clearVhubData();

    void changeStatus( Status status );
    void postCurrentStatus();
    std::string statusToString( Status status );

private:
    Status m_status = Status::Unknown;
    std::unique_ptr< ProvisionDataUtils::ProvisionData > m_provisionData;
    Services::TaskLoop m_taskLoop;
    Common::MQTTClient m_mqttClient;
};

}  // namespace VhubManager
}  // namespace HubPlatform
