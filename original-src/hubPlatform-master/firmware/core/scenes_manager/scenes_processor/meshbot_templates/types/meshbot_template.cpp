#include "scenes_manager/scenes_processor/meshbot_templates/types/meshbot_template.h"
#include "scenes_manager/scenes_processor/meshbot_templates/exceptions.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Types {

static std::string getNonEmptyString( const Json& j, const std::string& key ) {
    auto value = json_get_str( j, key );
    if( value.empty() )
        throw MeshbotTemplateParsingError( "Empty '" + key + "' field" );
    return value;
}

void moveNonEmptyString( std::string& src, std::string& dst, const std::string& key ) {
    dst = std::move( src );
    if( dst.empty() )
        throw MeshbotTemplateParsingError( "Empty '" + key + "' field" );
}

MeshbotTemplate::MeshbotTemplate( CloudDevices::MeshbotTemplateRecord record ) {
    try {
        moveNonEmptyString( record.uuid, uuid, "uuid" );
        moveNonEmptyString( record.title, title, "title" );
        moveNonEmptyString( record.description, description, "description" );
        moveNonEmptyString( record.id, id, "id" );
        moveNonEmptyString( record.oemId, oemId, "oem_id" );
        moveNonEmptyString( record.partnerUUID, partnerUUID, "partner_uuid" );
        moveNonEmptyString( record.localMeshbot, localMeshbot, "local_meshbot" );

        rules.reserve( record.rules.size() );

        for( auto& jsonRule : record.rules ) {
            if( !jsonRule.is_object() ) {
                throw MeshbotTemplateParsingError( "Rules element is not an object: " + jsonRule.dump() );
            }

            rules.emplace_back();
            auto& rule = rules.back();

            rule.field = getNonEmptyString( jsonRule, "field" );

            auto valueIt = jsonRule.find( "value" );
            if( valueIt == jsonRule.cend() )
                throw MeshbotTemplateParsingError( "MeshbotTemplate: rule.value is not provided:" + jsonRule.dump() );
            rule.value = valueIt.value();

            std::string ruleType = getNonEmptyString( jsonRule, "type" );
            if( ruleType == "device" )
                rule.type = Rule::Type::Device;
            else if( ruleType == "item" )
                rule.type = Rule::Type::Item;
            else
                throw MeshbotTemplateParsingError( "MeshbotTemplate: unknown rule type: " + ruleType );

            std::string ruleOperation = getNonEmptyString( jsonRule, "operation" );
            if( ruleOperation == "==" )
                rule.operation = Rule::Operation::Equal;
            else if( ruleOperation == "in" ) {
                rule.operation = Rule::Operation::In;
                if( !rule.value.is_array() ) {
                    throw MeshbotTemplateParsingError(
                        "MeshbotTemplate: rule operation 'in' must be used only with arrays, value: " +
                        rule.value.dump() );
                }
            }
            else
                throw MeshbotTemplateParsingError( "MeshbotTemplate: unknown rule operation: " + ruleOperation );
        }
    }
    catch( const std::exception& e ) {
        throw MeshbotTemplateParsingError( "Parsing error for template with uuid: " + uuid + ", error: " + e.what() );
    }
}

}  // namespace Types
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
