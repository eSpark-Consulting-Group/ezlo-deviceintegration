#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Types {

struct TemplateBinding {
    std::string bindingId;
    std::string templateUUID;
    std::string templateVersion;
    std::string sceneId;
    std::string triggerDeviceId;
    std::string triggerItemId;

    bool operator==( const TemplateBinding& other ) const;
    bool operator!=( const TemplateBinding& other ) const;
};

}  // namespace Types
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
