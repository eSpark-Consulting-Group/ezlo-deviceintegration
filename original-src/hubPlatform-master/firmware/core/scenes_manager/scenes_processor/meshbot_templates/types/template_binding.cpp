#include "scenes_manager/scenes_processor/meshbot_templates/types/template_binding.h"

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Types {

bool TemplateBinding::operator==( const TemplateBinding& other ) const {
    return bindingId == other.bindingId && templateUUID == other.templateUUID &&
           templateVersion == other.templateVersion && sceneId == other.sceneId &&
           triggerDeviceId == other.triggerDeviceId && triggerItemId == other.triggerItemId;
}

bool TemplateBinding::operator!=( const TemplateBinding& other ) const {
    return !( *this == other );
}

}  // namespace Types
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
