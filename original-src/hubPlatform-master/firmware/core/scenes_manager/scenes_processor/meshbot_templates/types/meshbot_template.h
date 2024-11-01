#pragma once

#include "cloud_devices/local_notifications/types/meshbot_template_record.h"

#include <string>
#include <vector>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Types {

struct MeshbotTemplate {
    struct Rule {
        enum Type { Device = 0, Item };
        Type type = Type::Device;
        std::string field;
        Json value;

        enum Operation { Equal = 0, In };
        Operation operation = Operation::Equal;
    };

    MeshbotTemplate( CloudDevices::MeshbotTemplateRecord record );
    MeshbotTemplate() = default;

    std::string uuid;
    std::string version;
    std::string title;
    std::string description;
    std::string id;
    std::string oemId;
    std::string partnerUUID;
    std::string localMeshbot;
    std::vector< Rule > rules;
};

}  // namespace Types
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
