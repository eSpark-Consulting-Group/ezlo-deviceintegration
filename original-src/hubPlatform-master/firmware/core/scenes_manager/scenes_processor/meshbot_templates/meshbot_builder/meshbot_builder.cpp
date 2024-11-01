#include "scenes_manager/scenes_processor/meshbot_templates/meshbot_builder/meshbot_builder.h"
#include "scenes_manager/scenes_processor/meshbot_templates/utils/utils.h"
#include "scenes_manager/scenes_processor/meshbot_templates/exceptions.h"

#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_utils.h"
#include "common_utils/exception_utils.h"
#include "base_utils/string_utils.h"
#include "database_client/dbqueryparser.h"

#include <algorithm>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {

MeshbotBuilder::MeshbotBuilder( Data data )
: m_tcdsGeneralDataVersion( data.tcdsGeneralDataVersion )
, m_tcdsMeshbotsDataVersion( data.tcdsMeshbotsDataVersion )
, m_controllerSerial( std::move( data.controllerSerial ) ) {
    for( auto& device : data.devices ) {
        m_devices.emplace( Utils::getDeviceId( device ), std::move( device ) );
    }

    for( auto& item : data.items ) {
        m_items.emplace( Utils::getItemId( item ), std::move( item ) );
    }

    for( auto& room : data.rooms ) {
        m_rooms.emplace( Utils::getRoomId( room ), std::move( room ) );
    }
}

Json MeshbotBuilder::buildMeshbot( std::string meshbotString,
                                   const std::string& sceneId,
                                   const std::string& deviceId,
                                   const std::string& itemId,
                                   const std::string& meshbotTemplateVersion ) const {
    auto deviceIt = m_devices.find( deviceId );
    if( deviceIt == m_devices.end() ) {
        LOGGER_ERROR( cc::error( "Unable to find device with id: " ) + cc::note( deviceId ) );
        return Json::object();
    }
    auto& device = deviceIt->second;

    auto itemIt = m_items.find( itemId );
    if( itemIt == m_items.end() ) {
        LOGGER_ERROR( cc::error( "Unable to find item with id: " ) + cc::note( itemId ) );
        return Json::object();
    }
    auto& item = itemIt->second;

    auto parentDeviceIt = m_devices.end();
    {
        namespace DeviceFields = CoreD::Model::Resources::DeviceFields;
        std::string parentDeviceId = json_get_str( device, DeviceFields::ParentDeviceIdFieldName );
        if( !parentDeviceId.empty() ) {
            parentDeviceIt = m_devices.find( parentDeviceId );
        }
    }

    replaceItemFields( meshbotString, item );
    replaceDeviceFields( meshbotString, device );

    if( parentDeviceIt != m_devices.end() ) {
        replaceParentDeviceFields( meshbotString, parentDeviceIt->second );
    }
    else {
        // Replace tokens to empty strings
        replaceParentDeviceFields( meshbotString, Json::object() );
    }

    replaceTcdsFields( meshbotString );

    auto meshbot = Json::parse( meshbotString );

    meshbot[ db::ID_FIELD_NAME ] = sceneId;

    meshbot[ "generator" ] = JSON( "name"
                                   << "tcds"
                                   << "version" << meshbotTemplateVersion << "role"
                                   << "system" );
    return meshbot;
}

void MeshbotBuilder::replaceItemFields( std::string& meshbot, const Json& item ) const {
    namespace ItemFields = CoreD::Model::Resources::ItemFields;

    std::string itemId = json_get_str( item, ItemFields::IdFieldName );
    StringUtils::replaceTokens( meshbot, "{item.id}", itemId );

    std::string itemName = json_get_str( item, ItemFields::NameFieldName );
    StringUtils::replaceTokens( meshbot, "{item.name}", itemName );
}

void MeshbotBuilder::replaceDeviceFields( std::string& meshbot, const Json& device ) const {
    namespace DeviceFields = CoreD::Model::Resources::DeviceFields;

    std::string deviceId = json_get_str( device, DeviceFields::IdFieldName );
    StringUtils::replaceTokens( meshbot, "{device.id}", deviceId );

    std::string deviceName = json_get_str( device, DeviceFields::NameFieldName );
    StringUtils::replaceTokens( meshbot, "{device.name}", deviceName );

    std::string roomId = json_get_str( device, DeviceFields::RoomIdFieldName );

    auto roomIt = m_rooms.find( roomId );
    if( roomIt != m_rooms.cend() ) {
        namespace RoomFields = CoreD::Model::Resources::RoomFields;
        auto roomName = json_get_str( roomIt->second, RoomFields::NameFieldName );
        StringUtils::replaceTokens( meshbot, "{device.roomName}", roomName );
    }

    StringUtils::replaceTokens( meshbot, "{device.controllerId}", m_controllerSerial );
}

void MeshbotBuilder::replaceParentDeviceFields( std::string& meshbot, const Json& device ) const {
    namespace DeviceFields = CoreD::Model::Resources::DeviceFields;

    std::string deviceId = json_get_str( device, DeviceFields::IdFieldName );
    StringUtils::replaceTokens( meshbot, "{parentDevice.id}", deviceId );

    std::string deviceName = json_get_str( device, DeviceFields::NameFieldName );
    StringUtils::replaceTokens( meshbot, "{parentDevice.name}", deviceName );

    std::string roomId = json_get_str( device, DeviceFields::RoomIdFieldName );

    auto roomIt = m_rooms.find( roomId );
    if( roomIt != m_rooms.cend() ) {
        namespace RoomFields = CoreD::Model::Resources::RoomFields;
        auto roomName = json_get_str( roomIt->second, RoomFields::NameFieldName );
        StringUtils::replaceTokens( meshbot, "{parentDevice.roomName}", roomName );
    }

    StringUtils::replaceTokens( meshbot, "{parentDevice.controllerId}", m_controllerSerial );
}

void MeshbotBuilder::replaceTcdsFields( std::string& meshbot ) const {
    StringUtils::replaceTokens( meshbot, "{tcdsCloudData.version}", std::to_string( m_tcdsGeneralDataVersion ) );
    StringUtils::replaceTokens( meshbot, "{tcdsTemplates.version}", std::to_string( m_tcdsMeshbotsDataVersion ) );
}

}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
