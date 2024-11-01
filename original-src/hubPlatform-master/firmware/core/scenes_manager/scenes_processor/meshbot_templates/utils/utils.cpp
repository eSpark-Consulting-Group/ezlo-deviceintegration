#include "scenes_manager/scenes_processor/meshbot_templates/utils/utils.h"

#include "scenes_manager/scenes_processor/meshbot_templates/exceptions.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "base_utils/string_utils.h"
#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <string>
#include <map>
#include <algorithm>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {
namespace Utils {

namespace {

bool matchRule( const Json& element, const Types::MeshbotTemplate::Rule& rule ) {
    using Rule = Types::MeshbotTemplate::Rule;

    auto valueIt = element.find( rule.field );
    if( valueIt == element.cend() ) {
        LOGGER_WARN( cc::warn( "Provided rule field not found in the element object, field: " ) +
                     cc::note( rule.field ) );
        return false;
    }

    if( rule.operation == Rule::Operation::Equal ) {
        return valueIt.value() == rule.value;
    }
    else if( rule.operation == Rule::Operation::In ) {
        auto it = std::find_if( rule.value.cbegin(), rule.value.cend(), [&valueIt]( const Json& value ) {
            return value == valueIt.value();
        } );
        return it != rule.value.cend();
    }
    else {
        throw MeshbotTemplatesError( "matchRule: unsupported rule operation" );
    }
}

}  // namespace

std::vector< MeshbotTemplates::Types::MeshbotTrigger > findMeshbotTriggers(
    const std::vector< Json >& devices,
    const std::vector< Json >& items,
    const MeshbotTemplates::Types::MeshbotTemplate& meshbotTemplate ) {
    using Rule = MeshbotTemplates::Types::MeshbotTemplate::Rule;

    std::vector< MeshbotTemplates::Types::MeshbotTrigger > triggers;

    auto& rules = meshbotTemplate.rules;

    std::vector< const Json* > matchedDevices;
    std::vector< const Json* > matchedItems;

    // If device rules not provided, match all devices
    matchedDevices.reserve( devices.size() );
    for( auto& device : devices ) {
        matchedDevices.push_back( &device );
    }

    // If item rules not provided, match all items
    matchedItems.reserve( items.size() );
    for( auto& item : items ) {
        matchedItems.push_back( &item );
    }

    for( auto& rule : rules ) {
        std::vector< const Json* > matches;

        if( rule.type == Rule::Type::Device ) {
            for( auto* device : matchedDevices ) {
                if( matchRule( *device, rule ) )
                    matches.push_back( device );
            }

            if( matches.empty() )
                return {};
            matchedDevices = matches;
        }
        else if( rule.type == Rule::Type::Item ) {
            for( auto* item : matchedItems ) {
                if( matchRule( *item, rule ) )
                    matches.push_back( item );
            }

            if( matches.empty() )
                return {};
            matchedItems = matches;
        }
        else {
            throw MeshbotTemplatesError( "findMeshbotTriggers: unsupported rule type" );
        }
    }

    std::map< std::string, const Json* > devicesMap;
    for( auto* device : matchedDevices ) {
        devicesMap.emplace( getDeviceId( *device ), device );
    }

    for( auto* item : matchedItems ) {
        namespace ItemFields = CoreD::Model::Resources::ItemFields;
        auto itemId = getItemId( *item );
        auto deviceId = json_get_str( *item, ItemFields::DeviceIdFieldName );

        auto it = devicesMap.find( deviceId );
        if( it == devicesMap.cend() )
            continue;
        auto& device = *( it->second );

        triggers.emplace_back(
            Types::MeshbotTrigger{ .deviceId = deviceId, .device = device, .itemId = itemId, .item = *item } );
    }

    return triggers;
}

std::vector< Types::TemplateBinding > readAllTemplateBindings( IDbClient& dbClient ) {
    std::vector< Types::TemplateBinding > bindings;

    auto bindingsJson = dbClient.findAll( HaTcdsSystemMeshbotsDBName, JSON_EMPTY_OBJ, nullptr );
    bindings.reserve( bindingsJson.size() );

    for( auto& j : bindingsJson ) {
        bindings.emplace_back();
        auto& binding = bindings.back();
        binding.bindingId = json_get_str( j, db::ID_FIELD_NAME );
        binding.templateUUID = json_get_str( j, "template_uuid" );
        binding.templateVersion = json_get_str( j, "template_version" );
        binding.sceneId = json_get_str( j, "scene_id" );
        binding.triggerDeviceId = json_get_str( j, "trigger_device_id" );
        binding.triggerItemId = json_get_str( j, "trigger_item_id" );
    }
    return bindings;
}

void storeTemplateBinding( IDbClient& dbClient, const Types::TemplateBinding& binding ) {
    Json j =
        JSON( db::ID_FIELD_NAME << binding.bindingId << "template_uuid" << binding.templateUUID << "template_version"
                                << binding.templateVersion << "scene_id" << binding.sceneId << "trigger_device_id"
                                << binding.triggerDeviceId << "trigger_item_id" << binding.triggerItemId );

    dbClient.update( HaTcdsSystemMeshbotsDBName, JSON_EMPTY_OBJ, j, true, false );
}

void deleteTemplateBinding( IDbClient& dbClient, const std::string& bindingId ) {
    dbClient.remove( HaTcdsSystemMeshbotsDBName, JSON( db::ID_FIELD_NAME << bindingId ) );
}

std::vector< Types::TemplateBinding > queryBindings( IDbClient& dbClient, const Types::TemplateBinding& query ) {
    std::vector< Types::TemplateBinding > result;
    auto bindings = readAllTemplateBindings( dbClient );
    for( auto& binding : bindings ) {
        if( !query.bindingId.empty() && binding.bindingId != query.bindingId )
            continue;
        if( !query.templateUUID.empty() && binding.templateUUID != query.templateUUID )
            continue;
        if( !query.templateVersion.empty() && binding.templateVersion != query.templateVersion )
            continue;
        if( !query.sceneId.empty() && binding.sceneId != query.sceneId )
            continue;
        if( !query.triggerDeviceId.empty() && binding.triggerDeviceId != query.triggerDeviceId )
            continue;
        if( !query.triggerItemId.empty() && binding.triggerItemId != query.triggerItemId )
            continue;

        result.emplace_back( std::move( binding ) );
    }
    return result;
}

std::string getDeviceId( const Json& device ) {
    namespace DeviceFields = CoreD::Model::Resources::DeviceFields;
    return json_get_str( device, DeviceFields::IdFieldName );
}

std::string getItemId( const Json& item ) {
    namespace ItemFields = CoreD::Model::Resources::ItemFields;
    return json_get_str( item, ItemFields::IdFieldName );
}

std::string getRoomId( const Json& room ) {
    namespace RoomFields = CoreD::Model::Resources::RoomFields;
    return json_get_str( room, RoomFields::IdFieldName );
}

}  // namespace Utils
}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
