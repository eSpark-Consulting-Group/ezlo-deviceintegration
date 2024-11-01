#pragma once

#include "scenes_manager/scenes_processor/meshbot_templates/types/meshbot_template.h"
#include "scenes_manager/scenes_processor/meshbot_templates/types/meshbot_trigger.h"
#include "scenes_manager/scenes_processor/meshbot_templates/types/template_binding.h"

#include <vector>

class IDbClient;

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Utils {

std::vector< MeshbotTemplates::Types::MeshbotTrigger > findMeshbotTriggers(
    const std::vector< Json >& devices,
    const std::vector< Json >& items,
    const MeshbotTemplates::Types::MeshbotTemplate& meshbotTemplate );

std::vector< Types::TemplateBinding > readAllTemplateBindings( IDbClient& dbClient );
void storeTemplateBinding( IDbClient& dbClient, const Types::TemplateBinding& binding );
void deleteTemplateBinding( IDbClient& dbClient, const std::string& bindingId );
std::vector< Types::TemplateBinding > queryBindings( IDbClient& dbClient, const Types::TemplateBinding& query );

std::string getDeviceId( const Json& device );
std::string getItemId( const Json& item );
std::string getRoomId( const Json& room );

}  // namespace Utils
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
