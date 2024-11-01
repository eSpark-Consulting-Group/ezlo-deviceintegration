#include "cored/model/values/value.h"
#include "cored/model/cored_gateway_setting.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"
#include "cored/tests/service/test_core_service.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "database_client/dbqueryparser.h"

#include "common_utils/cpp14.h"

#include <string>
#include <memory>
#include <gtest/gtest.h>

/*
TESTPLAN

DONE    1. Add gateway setting
DONE      1.1 Ensure a gateway setting is in a table
DONE      1.2 Ensure a gateway's setting field is increased
DONE      1.3 Ensure a gateway setting added broadcast is raised
DONE    2. Remove gateway setting
DONE      2.1 Ensure a gateway setting removed form a table
DONE      2.2 Ensure a gateway's setting field is decreased
DONE      2.3 Ensure a gateway setting removed broadcast was raised
DONE    3. Update gateway setting value
DONE      3.1 Ensure a gateway setting value was updated in a table
DONE      3.2 Ensure a gateway setting updated broadcast is raised
DONE    4. Update gateway setting status
DONE      4.1 Ensure a gateway setting status was updated in a table
DONE      4.2 Ensure a gateway setting updated broadcast is raised
DONE    5. GatewaySetting object
DONE      5.1 Check an object of a gateway setting serialization/deserialization
DONE      5.2 Check an object of a gateway setting serialization
DONE      5.3 Check an object of a gateway setting serialization for unsupported value type

*/

namespace HubPlatform {
namespace UT {

class GatewaySettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        Log::setLocalLogLevel( Log::FATAL );

        m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >(
            std::make_unique< Services::BusWrapper >(
                Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >() ) );

        getDBClient().clean();
        m_service->setUp();
    }

    void TearDown() override {
        getDBClient().clean();
        m_service->tearDown();
    }

    IDbClient& getDBClient() {
        return m_service->getDBClient();
    }

    const IDbClient& getDBClient() const {
        return m_service->getDBClient();
    }

    Services::UT::FakeResponseBus& getFakeResponseBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( m_service->getBus() );
    }

    using GatewaySettingId = CoreD::CoreService::GatewaySettingID;

    Json addGateway( const Json& gateway ) {
        return m_service->getDeviceManager().ensureGateway( gateway );
    }

    Json addGatewaySetting( const Json& setting ) {
        return m_service->getDeviceManager().addGatewaySetting( setting );
    }

    void removeGatewaySetting( const GatewaySettingId& settingId ) {
        m_service->removeGatewaySetting( settingId );
    }

    void updateGatewaySettingValue( const JsonObject& updateData ) {
        m_service->handleUpdateGatewaySettingValue( updateData );
    }

    void updateGatewaySettingStatus( const JsonObject& updateData ) {
        m_service->handleUpdateGatewaySettingStatus( updateData );
    }

private:
    std::unique_ptr< CoreD::Tests::TestCoreService > m_service;
};

class GatewaySettingObjectTest : public ::testing::Test {
protected:
};

TEST_F( GatewaySettingsTest, addSettingEnsureDatabaseHasEntry ) { /*1.1*/
    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();

    const auto reply = addGatewaySetting( setting );

    ASSERT_TRUE( db.has( CoreD::Model::Resources::DBNames::GatewaysSettingsDB, setting ) );
    EXPECT_EQ( json_get_str( setting, db::ID_FIELD_NAME ), json_get_str( reply, "id" ) );
    EXPECT_EQ( db.findOne( CoreD::Model::Resources::DBNames::GatewaysSettingsDB, setting, nullptr ), setting );
}

TEST_F( GatewaySettingsTest, addSettingEnsureGatewaySettingsCounterIncreased ) { /*1.2*/
    using namespace CoreD::Model::Resources;

    auto setting = R"({
	        "_id": "test_plugin.wassup",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto gateway = R"({
            "settings": 0
		})"_json;
    const auto& db = getDBClient();
    auto fieldsToReturn = JSON( GatewayFields::SettingsFieldName << GatewayFields::SettingsFieldName );

    const auto reply = addGateway( gateway );
    const auto gatewayId = json_get_str( reply, "id" );

    auto settingsJs = db.findOne(
        CoreD::Model::Resources::DBNames::GatewaysDB, JSON( db::ID_FIELD_NAME << gatewayId ), &fieldsToReturn );
    EXPECT_EQ( json_get_int( settingsJs, GatewayFields::SettingsFieldName ), 0 );

    setting[ GatewaySettingFields::GatewayId ] = gatewayId;
    addGatewaySetting( setting );

    settingsJs = db.findOne(
        CoreD::Model::Resources::DBNames::GatewaysDB, JSON( db::ID_FIELD_NAME << gatewayId ), &fieldsToReturn );
    EXPECT_EQ( json_get_int( settingsJs, GatewayFields::SettingsFieldName ), 1 );
}

TEST_F( GatewaySettingsTest, addSettingEnsureGatewaySettingAddedBroadcastRaised ) { /*1.3*/
    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();

    addGatewaySetting( setting );
    const auto message = getFakeResponseBus().getMessage();

    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_GATEWAY_SETTING_ADDED );
    EXPECT_EQ( message.data, setting );
}

TEST_F( GatewaySettingsTest, removeSettingEnsureDatabaseHasNoEntry ) { /*2.1*/
    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();

    const auto reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );
    removeGatewaySetting( settingId );

    ASSERT_FALSE( db.has( CoreD::Model::Resources::DBNames::GatewaysSettingsDB, setting ) );
}

TEST_F( GatewaySettingsTest, removeSettingEnsureGatewaySettingsCounterDecreased ) { /*2.2*/
    using namespace CoreD::Model::Resources;

    auto setting = R"({
	        "_id": "test_plugin.wassup",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto gateway = R"({
            "settings": 0
		})"_json;
    const auto& db = getDBClient();
    auto fieldsToReturn = JSON( GatewayFields::SettingsFieldName << GatewayFields::SettingsFieldName );

    auto reply = addGateway( gateway );
    const auto gatewayId = json_get_str( reply, "id" );

    setting[ GatewaySettingFields::GatewayId ] = gatewayId;
    reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );

    auto settingsJs = db.findOne(
        CoreD::Model::Resources::DBNames::GatewaysDB, JSON( db::ID_FIELD_NAME << gatewayId ), &fieldsToReturn );
    EXPECT_EQ( json_get_int( settingsJs, GatewayFields::SettingsFieldName ), 1 );

    removeGatewaySetting( settingId );

    settingsJs = db.findOne(
        CoreD::Model::Resources::DBNames::GatewaysDB, JSON( db::ID_FIELD_NAME << gatewayId ), &fieldsToReturn );
    EXPECT_EQ( json_get_int( settingsJs, GatewayFields::SettingsFieldName ), 0 );
}

TEST_F( GatewaySettingsTest, removeSettingEnsureGatewaySettingRemovedBroadcastRaised ) { /*2.3*/
    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();

    const auto reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );
    removeGatewaySetting( settingId );

    const auto message = getFakeResponseBus().getMessage();

    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_GATEWAY_SETTING_REMOVED );
    EXPECT_EQ( message.data, JSON( db::ID_FIELD_NAME << settingId ) );
}

TEST_F( GatewaySettingsTest, updateGatewaySettingValueEnsureDatabaseEntryUpdated ) { /*3.1*/
    using namespace CoreD::Model::Resources;

    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();
    Json data;

    const auto reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );

    auto gatewaySetting = db.findOne( DBNames::GatewaysSettingsDB, JSON( db::ID_FIELD_NAME << settingId ), nullptr );
    EXPECT_EQ( json_get_int( gatewaySetting, GatewaySettingFields::Value ), 42 );

    data[ RPCParams::SettingId ] = settingId;
    data[ RPCParams::Value ] = 100500;
    data[ RPCParams::ValueType ] = "int";

    updateGatewaySettingValue( data );

    gatewaySetting = db.findOne( DBNames::GatewaysSettingsDB, JSON( db::ID_FIELD_NAME << settingId ), nullptr );
    EXPECT_EQ( json_get_int( gatewaySetting, GatewaySettingFields::Value ), 100500 );
}

TEST_F( GatewaySettingsTest, updateGatewaySettingValueEnsureGatewaySettingUpdatedBroadcastRaised ) { /*3.2*/
    using namespace CoreD::Model::Resources;

    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();
    Json data;

    // throw away a SERVICE_STARTED event
    getFakeResponseBus().getMessage();

    const auto reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );
    getFakeResponseBus().getMessage();

    data[ RPCParams::SettingId ] = settingId;
    data[ RPCParams::Value ] = 100500;
    data[ RPCParams::ValueType ] = "int";

    updateGatewaySettingValue( data );

    auto message = getFakeResponseBus().getMessage();

    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_GATEWAY_SETTING_UPDATED );
    EXPECT_EQ( message.data, JSON( db::ID_FIELD_NAME << settingId << GatewaySettingFields::Value << 100500 ) );

    updateGatewaySettingValue( data );
}

TEST_F( GatewaySettingsTest, updateGatewaySettingStatusEnsureDatabaseEntryUpdated ) { /*4.1*/
    using namespace CoreD::Model::Resources;

    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();
    Json data;

    const auto reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );

    auto gatewaySetting = db.findOne( DBNames::GatewaysSettingsDB, JSON( db::ID_FIELD_NAME << settingId ), nullptr );
    EXPECT_EQ( json_get_str( gatewaySetting, GatewaySettingFields::SynchronizationStatus ), "synced" );

    data[ RPCParams::SettingId ] = settingId;
    data[ RPCParams::SynchronizationStatus ] = "failed";

    updateGatewaySettingStatus( data );

    gatewaySetting = db.findOne( DBNames::GatewaysSettingsDB, JSON( db::ID_FIELD_NAME << settingId ), nullptr );
    EXPECT_EQ( json_get_str( gatewaySetting, GatewaySettingFields::SynchronizationStatus ), "failed" );
}

TEST_F( GatewaySettingsTest, updateGatewaySettingStatusEnsureGatewaySettingUpdatedBroadcastRaised ) { /*4.2*/
    using namespace CoreD::Model::Resources;

    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;
    const auto& db = getDBClient();
    Json data;

    // throw away a SERVICE_STARTED event
    getFakeResponseBus().getMessage();

    const auto reply = addGatewaySetting( setting );
    const auto settingId = json_get_str( reply, "id" );
    getFakeResponseBus().getMessage();

    data[ RPCParams::SettingId ] = settingId;
    data[ RPCParams::SynchronizationStatus ] = "failed";

    updateGatewaySettingStatus( data );

    auto message = getFakeResponseBus().getMessage();

    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_GATEWAY_SETTING_UPDATED );
    EXPECT_EQ( message.data,
               JSON( db::ID_FIELD_NAME << settingId << GatewaySettingFields::SynchronizationStatus << "failed" ) );

    updateGatewaySettingStatus( data );
}

TEST( GatewaySettingTest, checkGatewaySettingObjectSerializationDeserialization ) { /* 5.1 */
    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "int",
	        "value": 42
	    })"_json;

    auto gatewaySetting = CoreD::Model::GatewaySetting::fromJson( setting );
    EXPECT_EQ( gatewaySetting.toJson(), setting );
}

TEST( GatewaySettingTest, checkGatewaySettingObjectDeserialization ) { /* 5.2 */
    using namespace CoreD::Model::Resources;

    Json setting;

    EXPECT_THROW( CoreD::Model::GatewaySetting::fromJson( setting ), CoreD::Model::CoreDModelError );

    setting[ GatewaySettingFields::Id ] = "test_plugin.wassup";
    EXPECT_THROW( CoreD::Model::GatewaySetting::fromJson( setting ), CoreD::Model::CoreDModelError );

    setting[ GatewaySettingFields::GatewayId ] = "100500";
    EXPECT_THROW( CoreD::Model::GatewaySetting::fromJson( setting ), CoreD::Model::CoreDModelError );

    setting[ GatewaySettingFields::SynchronizationStatus ] = "synced";
    EXPECT_THROW( CoreD::Model::GatewaySetting::fromJson( setting ), CoreD::Model::CoreDModelError );

    setting[ GatewaySettingFields::ValueType ] = "int";
    EXPECT_THROW( CoreD::Model::GatewaySetting::fromJson( setting ), CoreD::Model::CoreDModelError );

    setting[ GatewaySettingFields::Value ] = 42;
    EXPECT_NO_THROW( CoreD::Model::GatewaySetting::fromJson( setting ) );
}

TEST( GatewaySettingTest, checkGatewaySettingObjectDeserializationFromUnsopportedValueType ) { /* 5.3 */
    const auto setting = R"({
	        "_id": "test_plugin.wassup",
			"gatewayId": "100500",
	        "status": "synced",
	        "valueType": "bool",
	        "value": true
	    })"_json;

    EXPECT_THROW( CoreD::Model::GatewaySetting::fromJson( setting ), CoreD::Model::UnexpectedValueType );
}

}  // namespace UT
}  // namespace HubPlatform
