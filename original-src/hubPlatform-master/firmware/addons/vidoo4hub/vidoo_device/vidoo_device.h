#pragma once

#include "vidoo4hub/detectable_types/detectable_types.h"
#include "service_interfaces/service.h"

#include <string>

namespace HubPlatform {
namespace Vidoo4Hub {

class ICoreManager;

class VidooDevice {
public:
    VidooDevice( Services::Service& service, ICoreManager& coreManager, std::string coreDGatewayId );

    std::string getDeviceId();
    std::string getDetectableItemId( DetectableTypes::Type type );

private:
    void writeToDb( IDbClient& dbClient );
    bool readFromDb( IDbClient& dbClient );

private:
    using ItemId = std::string;
    std::string m_deviceId;
    std::map< DetectableTypes::Type, ItemId > m_detectableItemsMap;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
