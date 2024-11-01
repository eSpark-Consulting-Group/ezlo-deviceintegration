#include "device_item_groups_fixture.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

DeviceItemGroupsFixture::DeviceItemGroupsFixture()
: m_remote( makeRemote( nma() ) ) {}

void DeviceItemGroupsFixture::SetUp() {
    SceneManagerTestsFixture::SetUp();
}

void DeviceItemGroupsFixture::TearDown() {
    SceneManagerTestsFixture::TearDown();
}

JsonObject DeviceItemGroupsFixture::waitForDeviceGroupCreatedBroadcast( int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.created", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.created" ) << codeLineStr;
    return broadcast;
}

JsonObject DeviceItemGroupsFixture::waitForDeviceGroupDeletedBroadcast( int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.device.group.deleted", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.group.deleted" ) << codeLineStr;
    return broadcast;
}

std::string DeviceItemGroupsFixture::createDeviceGroup( const JsonObject& request, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    sendRequestAndCheckAnswer( request, codeLine );
    auto br = waitForDeviceGroupCreatedBroadcast( codeLine );
    std::string deviceGroupId = br[ "result" ][ "_id" ];
    EXPECT_FALSE( deviceGroupId.empty() ) << codeLineStr;
    return deviceGroupId;
}

void DeviceItemGroupsFixture::deleteDeviceGroup( const std::string& groupId, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );

    auto groupOps = makeRemote().groupOperations();

    Json answer = groupOps->deviceGroup().deleteGroup( JSON( "id" << groupId ) );
    ASSERT_FALSE( hasError( answer ) ) << answer;

    auto br = waitForDeviceGroupDeletedBroadcast( codeLine );
    std::string deviceGroupId = br[ "result" ][ "_id" ];
    EXPECT_EQ( deviceGroupId, groupId ) << codeLineStr;
}

std::string DeviceItemGroupsFixture::createItemGroup( const JsonObject& request, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );
    sendRequestAndCheckAnswer( request, codeLine );
    JsonObject broadcast = waitForBroadcast( "hub.item.group.created", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.group.created" ) << codeLineStr;
    std::string groupId = broadcast[ "result" ][ "_id" ];
    EXPECT_FALSE( groupId.empty() ) << codeLineStr;
    return groupId;
}

void DeviceItemGroupsFixture::deleteItemGroup( const std::string& groupId, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );

    Json params = JSON( "id" << groupId << "force" << false );

    auto groupOps = makeRemote().groupOperations();

    Json answer = groupOps->itemGroup().deleteGroup( params );
    ASSERT_FALSE( hasError( answer ) ) << answer;

    JsonObject broadcast = waitForBroadcast( "hub.item.group.deleted", codeLineStr );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.group.deleted" ) << codeLineStr;
    std::string deviceGroupId = broadcast[ "result" ][ "_id" ];
    EXPECT_EQ( deviceGroupId, groupId ) << codeLineStr;
}

std::string DeviceItemGroupsFixture::makeDevice( const std::string& category,
                                                 const std::string& subcategory,
                                                 int codeLine ) {
    return addDevice(
        m_remote, "Controller device-item group device", category, category, category, subcategory, false, codeLine );
}

std::string DeviceItemGroupsFixture::makeIntItem( const std::string& deviceId,
                                                  const std::string& itemName,
                                                  int value,
                                                  int codeLine ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::Int;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::Int,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );
    return addItem( m_remote, item, codeLine, nullptr );
}

std::string DeviceItemGroupsFixture::makeBooleanItem( const std::string& deviceId,
                                                      const std::string& itemName,
                                                      bool value,
                                                      int codeLine ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::Bool;
    CoreD::Model::Item item( deviceId,
                             itemName,
                             ValueType::Enum::Bool,
                             true,
                             true,
                             ValueFactory::createItemValue( valueType, Json( value ) ) );
    return addItem( m_remote, item, codeLine, nullptr );
}

std::string DeviceItemGroupsFixture::makeStringItem( const std::string& deviceId,
                                                     const std::string& itemName,
                                                     const std::string& value,
                                                     int codeLine ) {
    using namespace CoreD::Model;
    auto valueType = ValueType::Enum::String;
    CoreD::Model::Item item(
        deviceId, itemName, valueType, true, true, ValueFactory::createItemValue( valueType, Json( value ) ) );
    return addItem( m_remote, item, codeLine, nullptr );
}

}  // namespace Tests
}  // namespace HubPlatform
