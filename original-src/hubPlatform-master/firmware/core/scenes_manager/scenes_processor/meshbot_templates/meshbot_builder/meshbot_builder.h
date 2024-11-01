#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <vector>
#include <map>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {

class MeshbotBuilder {
public:
    struct Data {
        std::vector< Json > devices;
        std::vector< Json > items;
        std::vector< Json > rooms;
        int tcdsGeneralDataVersion = -1;
        int tcdsMeshbotsDataVersion = -1;
        std::string controllerSerial;
    };

    MeshbotBuilder( Data data );

    Json buildMeshbot( std::string meshbotString,
                       const std::string& sceneId,
                       const std::string& deviceId,
                       const std::string& itemId,
                       const std::string& meshbotTemplateVersion ) const;

private:
    using DeviceId = std::string;
    using ItemId = std::string;
    using RoomId = std::string;

private:
    void replaceItemFields( std::string& meshbot, const Json& item ) const;
    void replaceDeviceFields( std::string& meshbot, const Json& device ) const;
    void replaceParentDeviceFields( std::string& meshbot, const Json& device ) const;
    void replaceTcdsFields( std::string& meshbot ) const;

private:
    std::map< DeviceId, Json > m_devices;
    std::map< ItemId, Json > m_items;
    std::map< RoomId, Json > m_rooms;
    const int m_tcdsGeneralDataVersion = -1;
    const int m_tcdsMeshbotsDataVersion = -1;
    const std::string m_controllerSerial;
};

}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
