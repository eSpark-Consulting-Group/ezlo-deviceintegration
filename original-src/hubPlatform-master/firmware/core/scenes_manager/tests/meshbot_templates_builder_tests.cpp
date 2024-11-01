#include "scenes_manager/scenes_processor/meshbot_templates/meshbot_builder/meshbot_builder.h"

#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN
DONE    1 MeshbotTemplate meshbot builder tests
DONE        1.1 Build meshbot

*/

class MeshbotTemplatesBuilderTests : public ::testing::Test {
public:
    struct DeviceData {
        std::string id;
        std::string name;
        std::string category;
        std::string subcategory;
        std::string roomId;
        std::string parentDeviceId;
    };

    struct ItemData {
        std::string id;
        std::string name;
    };

    struct RoomData {
        std::string id;
        std::string name;
    };

    DeviceData deviceDataWithSuffix( const std::string suffix ) {
        DeviceData data;
        data.id = "deviceId_" + suffix;
        data.name = "deviceName_" + suffix;
        data.category = "deviceCategory_" + suffix;
        data.subcategory = "deviceSubcategory_" + suffix;
        data.roomId = "roomId_" + suffix;
        data.parentDeviceId = "parentDeviceId_" + suffix;
        return data;
    }

    ItemData itemDataWithSuffix( const std::string suffix ) {
        ItemData data;
        data.id = "itemId_" + suffix;
        data.name = "itemName_" + suffix;
        return data;
    }

    RoomData roomDataWithSuffix( const std::string suffix ) {
        RoomData data;
        data.id = "roomId_" + suffix;
        data.name = "roomName_" + suffix;
        return data;
    }

    Json deviceFromDeviceData( const DeviceData& data ) {
        namespace DeviceFields = CoreD::Model::Resources::DeviceFields;
        Json j;
        j[ DeviceFields::IdFieldName ] = data.id;
        j[ DeviceFields::NameFieldName ] = data.name;
        j[ DeviceFields::CategoryFieldName ] = data.category;
        j[ DeviceFields::SubcategoryFieldName ] = data.subcategory;
        j[ DeviceFields::RoomIdFieldName ] = data.roomId;
        j[ DeviceFields::ParentDeviceIdFieldName ] = data.parentDeviceId;
        return j;
    }

    Json itemFromItemData( const ItemData& data ) {
        namespace ItemFields = CoreD::Model::Resources::ItemFields;
        Json j;
        j[ ItemFields::IdFieldName ] = data.id;
        j[ ItemFields::NameFieldName ] = data.name;
        return j;
    }

    Json roomFromRoomData( const RoomData& data ) {
        namespace RoomFields = CoreD::Model::Resources::RoomFields;
        Json j;
        j[ RoomFields::IdFieldName ] = data.id;
        j[ RoomFields::NameFieldName ] = data.name;
        return j;
    }
};

TEST_F( MeshbotTemplatesBuilderTests, buildMeshbot ) {  // 1.1
    const std::string sceneId = "sceneIdA";
    const std::string templateVersion = "templateVersionA";
    const std::string controllerSerial = "controllerSerial";
    const int tcdsGeneralDataVersion = 10;
    const int tcdsMeshbotsDataVersion = 20;

    auto deviceA = deviceDataWithSuffix( "A" );
    auto parentDeviceA = deviceDataWithSuffix( "parent-of-A" );
    auto itemA = itemDataWithSuffix( "A" );
    auto roomA = roomDataWithSuffix( "A" );

    deviceA.parentDeviceId = parentDeviceA.id;
    deviceA.roomId = roomA.id;
    parentDeviceA.roomId = roomA.id;

    std::vector< Json > devices;
    std::vector< Json > items;
    std::vector< Json > rooms;

    // Populate random data
    for( size_t i = 0; i < 2; ++i ) {
        devices.emplace_back( deviceFromDeviceData( deviceDataWithSuffix( std::to_string( i ) ) ) );
        items.emplace_back( itemFromItemData( itemDataWithSuffix( std::to_string( i ) ) ) );
        rooms.emplace_back( roomFromRoomData( roomDataWithSuffix( std::to_string( i ) ) ) );
    }

    devices.emplace_back( deviceFromDeviceData( deviceA ) );
    devices.emplace_back( deviceFromDeviceData( parentDeviceA ) );
    items.emplace_back( itemFromItemData( itemA ) );
    rooms.emplace_back( roomFromRoomData( roomA ) );

    ScenesManager::MeshbotTemplates::MeshbotBuilder::Data builderData;
    builderData.devices = devices;
    builderData.items = items;
    builderData.rooms = rooms;
    builderData.tcdsGeneralDataVersion = 10;
    builderData.tcdsMeshbotsDataVersion = 20;
    builderData.controllerSerial = controllerSerial;

    ScenesManager::MeshbotTemplates::MeshbotBuilder builder( std::move( builderData ) );

    const std::string meshbotString = ( R"#({
        "data" : {
            "deviceId" : "{device.id}",
            "deviceName" : "{device.name}",
            "deviceRoomName" : "{device.roomName}",
            "deviceControllerId" : "{device.controllerId}",

            "parentDeviceId" : "{parentDevice.id}",
            "parentDeviceName" : "{parentDevice.name}",
            "parentDeviceRoomName" : "{parentDevice.roomName}",
            "parentDeviceControllerId" : "{parentDevice.controllerId}",

            "itemId" : "{item.id}",
            "itemName" : "{item.name}",

            "tcdsGeneralDataVersion" : "{tcdsCloudData.version}",
            "tcdsMeshbotsDataVersion" : "{tcdsTemplates.version}"
        }
        })#"_json )
                                          .dump();

    auto meshbot = builder.buildMeshbot( meshbotString, sceneId, deviceA.id, itemA.id, templateVersion );

    EXPECT_EQ( json_get_str( meshbot, db::ID_FIELD_NAME ), sceneId );

    {
        Json expectedGenerator = JSON( "name"
                                       << "tcds"
                                       << "version" << templateVersion << "role"
                                       << "system" );
        EXPECT_EQ( meshbot[ "generator" ].dump(), expectedGenerator.dump() );
    }

    auto meshbotData = meshbot.at( "data" );

    EXPECT_EQ( json_get_str( meshbotData, "deviceId" ), deviceA.id );
    EXPECT_EQ( json_get_str( meshbotData, "deviceName" ), deviceA.name );
    EXPECT_EQ( json_get_str( meshbotData, "deviceRoomName" ), roomA.name );
    EXPECT_EQ( json_get_str( meshbotData, "deviceControllerId" ), controllerSerial );

    EXPECT_EQ( json_get_str( meshbotData, "parentDeviceId" ), parentDeviceA.id );
    EXPECT_EQ( json_get_str( meshbotData, "parentDeviceName" ), parentDeviceA.name );
    EXPECT_EQ( json_get_str( meshbotData, "parentDeviceRoomName" ), roomA.name );
    EXPECT_EQ( json_get_str( meshbotData, "parentDeviceControllerId" ), controllerSerial );

    EXPECT_EQ( json_get_str( meshbotData, "itemId" ), itemA.id );
    EXPECT_EQ( json_get_str( meshbotData, "itemName" ), itemA.name );

    EXPECT_EQ( json_get_str( meshbotData, "tcdsGeneralDataVersion" ), std::to_string( tcdsGeneralDataVersion ) );
    EXPECT_EQ( json_get_str( meshbotData, "tcdsMeshbotsDataVersion" ), std::to_string( tcdsMeshbotsDataVersion ) );
}

}  // namespace UT
}  // namespace HubPlatform
