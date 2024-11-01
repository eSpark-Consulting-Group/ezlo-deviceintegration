#include "cored/tests/service/test_core_service.h"
#include "cored/tests/fakes/fake_feature_manager.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "cored/db_modifier/security_item_db_modifier.h"
#include "test_utils/test_utils.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/model/values/value_factory.h"

namespace HubPlatform {
namespace UT {

using namespace CoreD::Model::Resources;

/*
TEST PLAN
DONE   1. Add device with protection config
DONE   2. Update device with protection config
DONE   3. Remove device with protection config
DONE   4. Add item for device with protection config
DONE   5. Update item from device with protection config
DONE   6. Remove item from device with protection config
DONE   7. Update item with dictionary value from device with protection config
DONE   8. Security config fields in item Updated broadcast
DONE        8.1 signal in item updated broadcast
DONE        8.2 default security config id
DONE        8.3 alarm field not set
DONE        8.4 signal field not set
DONE        8.5 chime field other than none
 */

class ProtectionConfigTests : public ::testing::Test {
public:
    ProtectionConfigTests();
    void SetUp() override;
    void TearDown() override;

    CoreD::CoreService& getCoreDService();
    IDbClient& getDBClient();
    void cleanDB();
    const Services::UT::FakeResponseBus& getTestBus();

    CoreD::CoreService::GatewayID ensureGateway( const Json& gate );
    CoreD::CoreService::DeviceID addDevice( const Json& device );
    CoreD::CoreService::DeviceID updateDevice( const std::string& id, const Json& fields );
    CoreD::CoreService::ItemID addItem( const Json& item );
    CoreD::CoreService::ItemID addSecurityItem( const std::string& deviceId );
    void removeDevice( const CoreD::CoreService::DeviceID& deviceId );
    void removeItem( const CoreD::CoreService::ItemID& itemId );
    bool setItemValue( const JsonObject& json );
    Json makeIntItem( const std::string& deviceId, const std::string& name );
    void broadcaster( MessageSubClasses subClass, const Json& broadcast );
    const Services::Message& getBroadcastMessage();
    CoreD::ItemDbModifier& getItemDbModifier();

protected:
    CoreD::Tests::TestCoreServiceWithoutHouseModes m_cored;
    std::unique_ptr< CoreD::ItemDbModifier > m_itemDbModifier;
    ProvisionDataUtils::ProvisionData m_provisionData;
    std::unique_ptr< CoreD::IFeaturesManager > m_featureManager;
    std::unique_ptr< CoreD::SecurityItemDbModifier > m_securityItemDbModifier;
    Services::Message m_broadcastMessage;
    std::unique_ptr< CoreD::SwingerShutdown::SwingerShutdown > m_swingerShutdown;
};

ProtectionConfigTests::ProtectionConfigTests()
: m_cored( std::make_unique< Services::BusWrapper >(
      Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ) ) {}

void ProtectionConfigTests::SetUp() {
    m_featureManager = std::make_unique< CoreD::UT::FakeFeatureManager >();
    m_swingerShutdown = std::make_unique< CoreD::SwingerShutdown::SwingerShutdown >( m_cored );
    m_securityItemDbModifier =
        std::make_unique< CoreD::SecurityItemDbModifier >( m_cored, *m_featureManager, *m_swingerShutdown );

    m_itemDbModifier = std::make_unique< CoreD::ItemDbModifier >(
        m_cored,
        m_provisionData,
        [this]( MessageSubClasses subClass, const Json& broadcast ) { broadcaster( subClass, broadcast ); },
        *m_featureManager,
        *m_securityItemDbModifier );

    m_cored.setUp();
    cleanDB();
}

void ProtectionConfigTests::TearDown() {
    cleanDB();
    m_cored.tearDown();
}

CoreD::CoreService& ProtectionConfigTests::getCoreDService() {
    return m_cored;
}

IDbClient& ProtectionConfigTests::getDBClient() {
    return getCoreDService().getDBClient();
}

void ProtectionConfigTests::cleanDB() {
    getDBClient().clean();
}

const Services::UT::FakeResponseBus& ProtectionConfigTests::getTestBus() {
    return *dynamic_cast< Services::UT::FakeResponseBus* >( getCoreDService().getBus() );
}

CoreD::CoreService::GatewayID ProtectionConfigTests::ensureGateway( const Json& gate ) {
    auto gatewayId = getCoreDService().m_deviceManager->ensureGateway( gate );
    return gatewayId[ "id" ];
}
CoreD::CoreService::DeviceID ProtectionConfigTests::addDevice( const Json& device ) {
    auto deviceId = getCoreDService().m_deviceManager->addDevice( device );
    return deviceId[ "id" ];
}

CoreD::CoreService::DeviceID ProtectionConfigTests::updateDevice( const std::string& id, const Json& fields ) {
    auto deviceId = getCoreDService().m_deviceManager->modifyDevice( id, fields );
    return deviceId[ "id" ];
}
CoreD::CoreService::ItemID ProtectionConfigTests::addItem( const Json& item ) {
    auto itemId = getCoreDService().m_deviceManager->addItem( item );
    return itemId[ "id" ];
}

void ProtectionConfigTests::removeDevice( const CoreD::CoreService::DeviceID& deviceId ) {
    getCoreDService().removeDevice( deviceId );
}

void ProtectionConfigTests::removeItem( const CoreD::CoreService::ItemID& itemId ) {
    getCoreDService().removeItem( itemId );
}
bool ProtectionConfigTests::setItemValue( const JsonObject& json ) {
    return getCoreDService().handleSetItemValue( json );
}

Json ProtectionConfigTests::makeIntItem( const std::string& deviceId, const std::string& name ) {
    Json item;
    item[ ItemFields::DeviceIdFieldName ] = deviceId;
    item[ ItemFields::HasGetterFieldName ] = true;
    item[ ItemFields::HasSetterFieldName ] = true;
    item[ ItemFields::NameFieldName ] = name;
    item[ ItemFields::TypeFieldName ] = "int";
    item[ ItemFields::ValueFieldName ] = 0;
    item[ ItemFields::ShowFieldName ] = true;
    item[ ItemFields::EnumFieldName ] = JSON_EMPTY_ARR;
    return item;
}

void ProtectionConfigTests::broadcaster( MessageSubClasses subClass, const Json& broadcast ) {
    m_broadcastMessage.msgSubClass = subClass;
    m_broadcastMessage.data = broadcast;
}

const Services::Message& ProtectionConfigTests::getBroadcastMessage() {
    return m_broadcastMessage;
}

CoreD::ItemDbModifier& ProtectionConfigTests::getItemDbModifier() {
    return *m_itemDbModifier;
}

CoreD::CoreService::ItemID ProtectionConfigTests::addSecurityItem( const std::string& deviceId ) {
    using namespace CoreDHouseModes::Resources;
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item item( deviceId,
                             Items::SecurityThreat,
                             valueType,
                             true,
                             true,
                             CoreD::Model::ValueFactory::createDefaultItemValue( valueType ),
                             "" );

    auto itemId = getCoreDService().m_deviceManager->addItem( item.toJson() );
    return itemId[ "id" ];
}

TEST_F( ProtectionConfigTests, deviceAdded ) {  // 1
    addDevice( JSON( DeviceFields::NameFieldName << "device" ) );

    const auto message = getTestBus().getMessage();
    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_DEVICE_ADDED );

    EXPECT_EQ( json_get_str( message.data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, deviceUpdate ) {  // 2
    auto firmwareVersionInfo1 =
        JSON( DeviceFields::FirmwareIdFieldName << "123" << DeviceFields::FirmwareVersionFieldName << "1.1.0" );
    auto firmwareVersionInfo2 =
        JSON( DeviceFields::FirmwareIdFieldName << "123" << DeviceFields::FirmwareVersionFieldName << "1.2.0" );
    const auto deviceId = addDevice( JSON( DeviceFields::FirmwareFieldName << JSON_ARRAY( firmwareVersionInfo1 ) ) );
    updateDevice( deviceId, JSON( DeviceFields::FirmwareFieldName << JSON_ARRAY( firmwareVersionInfo2 ) ) );
    const auto message = getTestBus().getMessage();
    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_DEVICE_UPDATED );
    EXPECT_EQ( json_get_str( message.data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, deviceRemoved ) {  // 3
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "device" ) );
    removeDevice( deviceId );
    const auto message = getTestBus().getMessage();
    EXPECT_EQ( message.group, GroupsNames::DEVICE_GROUP );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_DEVICE_REMOVED );
    EXPECT_EQ( json_get_str( message.data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, setItemValue ) {  // 4
    constexpr int value = 10;
    const Json jsonValue = value;
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId = addDevice( JSON( DeviceFields::GatewayIdFieldName << gatewayId ) );
    Json item = makeIntItem( deviceId, "counter" );
    const auto itemId = addItem( item );
    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    auto message = getTestBus().getMessage();
    EXPECT_EQ( message.msgClass, EVENT ) << "MessageClass differs at ";
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED ) << "MessageSubClass differs at ";
    EXPECT_EQ( json_get_str( message.data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, addItem ) {  // 5
    constexpr bool value = true;
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "device" ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_ADDED );
    EXPECT_EQ( json_get_str( message.data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, removeItem ) {  // 6
    constexpr bool value = true;
    const auto deviceId = addDevice( JSON( DeviceFields::NameFieldName << "device" ) );
    const auto itemId = addItem( JSON( ItemFields::DeviceIdFieldName << deviceId ) );
    removeItem( itemId );
    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_REMOVED );
    EXPECT_EQ( json_get_str( message.data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, updateItemDictionary ) {  // 7
    const JsonObject device = R"(
    {
        "_id": "device_id1",
        "protectConfig":"default"
    }
    )"_json;

    const JsonObject item = R"(
    {
        "_id": "id_1",
        "deviceId": "device_id1",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes",
        "show": true,
        "type": "dictionary.userCode",
        "value": {
            "1": {
                "code": "1234",
                "name": "One"
            }
        }
    }
    )"_json;

    const JsonObject newValue = R"(
    {
        "code": "2812",
        "name": "Two"
    }
    )"_json;

    const JsonObject updatePortion = R"(
    {
        "value": {
            "1" : {
                "code": "1234",
                "name": "One"
            },
            "2" : {
                "code": "2812",
                "name": "Two"
            }
        }
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getItemDbModifier().insert( item );
    getItemDbModifier().updateDictionary( "id_1", updatePortion, { "2", newValue, "added" } );

    EXPECT_EQ( getBroadcastMessage().msgSubClass, HUB_ITEM_DICTIONARY_UPDATED );
    EXPECT_EQ( json_get_str( getBroadcastMessage().data, "protectConfig" ), "default" );
}

TEST_F( ProtectionConfigTests, itemUpdateSecurityConfigFields ) {  // 8.1

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "none",
        "signal": {
            "supervisory": {
                "send": true,
                "event": { "code": "381", "alias": "on" },
                "restoral": { "code": "381", "alias": "off" }
            },
            "trippedArmed": {
                "send": true,
                "event": { "code": "123", "alias": "on" },
                "restoral": { "code": "123", "alias": "off" }
            },
            "tamperedArmed": {
                "send": false
            },
            "tamperedDisarmed": {
                "send": false
            },
            "bypass": {
                "send": true,
                "event": { "code": "570", "alias": "on" }
            }
        }
    }
    )"_json;

    const JsonObject contactId = R"(
    {
        "_id": "123",
        "text": "123 Panic Audible",
        "on": "ALERT_PANIC_AUDIBLE",
        "off": "ALERT_PANIC_AUDIBLE_OFF"
    }
    )"_json;

    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );
    getDBClient().insert( DBNames::ContactIdsDB, contactId );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId =
        addDevice( JSON( DeviceFields::NameFieldName
                         << "device" << DeviceFields::ProtectConfig << "0" << DeviceFields::GatewayIdFieldName
                         << gatewayId << DeviceFields::CategoryFieldName << "security_sensor"
                         << DeviceFields::SubcategoryFieldName << "motion" << DeviceFields::ArmedFieldName << true ) );

    const auto itemId = addSecurityItem( deviceId );

    constexpr bool value = true;
    const Json jsonValue = value;

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
    ASSERT_TRUE( json_contains( message.data, "protect" ) );
    ASSERT_TRUE( json_contains( message.data, "protectNotification" ) );
    ASSERT_TRUE( json_contains( message.data, "houseMode" ) );
    JsonObject protect;
    json_get_obj( message.data, "protect", protect );
    ASSERT_TRUE( json_contains( protect, "signal" ) );
    EXPECT_EQ( json_get_str( protect, "signal" ), "ALERT_PANIC_AUDIBLE" );
    EXPECT_EQ( json_get_str( protect, "soundType" ), "police" );
}

TEST_F( ProtectionConfigTests, itemUpdateSecurityConfigFieldsDefaultSecurityConfigId ) {  // 8.2

    const JsonObject securityConfig = R"(
    {
        "_id": "default",
        "name": "Default",
        "alarm": true,
        "soundType": "police",
        "chime": "none",
        "signal": {
            "supervisory": {
                "send": true,
                "event": { "code": "381", "alias": "on" },
                "restoral": { "code": "381", "alias": "off" }
            },
            "trippedArmed": {
                "send": true,
                "event": { "code": "123", "alias": "on" },
                "restoral": { "code": "123", "alias": "off" }
            },
            "tamperedArmed": {
                "send": false
            },
            "tamperedDisarmed": {
                "send": false
            },
            "bypass": {
                "send": true,
                "event": { "code": "570", "alias": "on" }
            }
        }
    }
    )"_json;

    const JsonObject contactId = R"(
    {
        "_id": "123",
        "text": "123 Panic Audible",
        "on": "ALERT_PANIC_AUDIBLE",
        "off": "ALERT_PANIC_AUDIBLE_OFF"
    }
    )"_json;

    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );
    getDBClient().insert( DBNames::ContactIdsDB, contactId );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId =
        addDevice( JSON( DeviceFields::NameFieldName << "device" << DeviceFields::GatewayIdFieldName << gatewayId
                                                     << DeviceFields::CategoryFieldName << "security_sensor"
                                                     << DeviceFields::SubcategoryFieldName << "motion"
                                                     << DeviceFields::ArmedFieldName << true ) );

    const auto itemId = addSecurityItem( deviceId );

    constexpr bool value = true;
    const Json jsonValue = value;

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
    ASSERT_FALSE( json_contains( message.data, "protect" ) );
    ASSERT_TRUE( json_contains( message.data, "protectNotification" ) );
    ASSERT_TRUE( json_contains( message.data, "houseMode" ) );
}

TEST_F( ProtectionConfigTests, itemUpdateSecurityConfigAlarmFieldNotSet ) {  // 8.3

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": false,
        "soundType": "police",
        "chime": "none",
        "signal": {
            "supervisory": {
                "send": true,
                "event": { "code": "381", "alias": "on" },
                "restoral": { "code": "381", "alias": "off" }
            },
            "trippedArmed": {
                "send": true,
                "event": { "code": "123", "alias": "on" },
                "restoral": { "code": "123", "alias": "off" }
            },
            "tamperedArmed": {
                "send": false
            },
            "tamperedDisarmed": {
                "send": false
            },
            "bypass": {
                "send": true,
                "event": { "code": "570", "alias": "on" }
            }
        }
    }
    )"_json;

    const JsonObject contactId = R"(
    {
        "_id": "123",
        "text": "123 Panic Audible",
        "on": "ALERT_PANIC_AUDIBLE",
        "off": "ALERT_PANIC_AUDIBLE_OFF"
    }
    )"_json;

    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );
    getDBClient().insert( DBNames::ContactIdsDB, contactId );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId =
        addDevice( JSON( DeviceFields::NameFieldName
                         << "device" << DeviceFields::ProtectConfig << "0" << DeviceFields::GatewayIdFieldName
                         << gatewayId << DeviceFields::CategoryFieldName << "security_sensor"
                         << DeviceFields::SubcategoryFieldName << "motion" << DeviceFields::ArmedFieldName << true ) );

    const auto itemId = addSecurityItem( deviceId );

    constexpr bool value = true;
    const Json jsonValue = value;

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
    ASSERT_FALSE( json_contains( message.data, "protect" ) );
    ASSERT_FALSE( json_contains( message.data, "protectNotification" ) );
    ASSERT_FALSE( json_contains( message.data, "houseMode" ) );
}

TEST_F( ProtectionConfigTests, itemUpdateSecurityConfigSignalNotSet ) {  // 8.4

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "none",
        "signal": {
            "supervisory": {
                "send": true,
                "event": { "code": "381", "alias": "on" },
                "restoral": { "code": "381", "alias": "off" }
            },
            "trippedArmed": {
                "send": false,
                "event": { "code": "123", "alias": "on" },
                "restoral": { "code": "123", "alias": "off" }
            },
            "tamperedArmed": {
                "send": false
            },
            "tamperedDisarmed": {
                "send": false
            },
            "bypass": {
                "send": true,
                "event": { "code": "570", "alias": "on" }
            }
        }
    }
    )"_json;

    const JsonObject contactId = R"(
    {
        "_id": "123",
        "text": "123 Panic Audible",
        "on": "ALERT_PANIC_AUDIBLE",
        "off": "ALERT_PANIC_AUDIBLE_OFF"
    }
    )"_json;

    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );
    getDBClient().insert( DBNames::ContactIdsDB, contactId );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId =
        addDevice( JSON( DeviceFields::NameFieldName
                         << "device" << DeviceFields::ProtectConfig << "0" << DeviceFields::GatewayIdFieldName
                         << gatewayId << DeviceFields::CategoryFieldName << "security_sensor"
                         << DeviceFields::SubcategoryFieldName << "motion" << DeviceFields::ArmedFieldName << true ) );

    const auto itemId = addSecurityItem( deviceId );

    constexpr bool value = true;
    const Json jsonValue = value;

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
    ASSERT_TRUE( json_contains( message.data, "protect" ) );
    ASSERT_TRUE( json_contains( message.data, "protectNotification" ) );
    ASSERT_TRUE( json_contains( message.data, "houseMode" ) );
    JsonObject protect;
    json_get_obj( message.data, "protect", protect );
    ASSERT_FALSE( json_contains( protect, "signal" ) );
    EXPECT_EQ( json_get_str( protect, "soundType" ), "police" );
}

TEST_F( ProtectionConfigTests, itemUpdateSecurityConfigChimeField ) {  // 8.5

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "ring",
        "signal": {
            "supervisory": {
                "send": true,
                "event": { "code": "381", "alias": "on" },
                "restoral": { "code": "381", "alias": "off" }
            },
            "trippedArmed": {
                "send": false,
                "event": { "code": "123", "alias": "on" },
                "restoral": { "code": "123", "alias": "off" }
            },
            "tamperedArmed": {
                "send": false
            },
            "tamperedDisarmed": {
                "send": false
            },
            "bypass": {
                "send": true,
                "event": { "code": "570", "alias": "on" }
            }
        }
    }
    )"_json;

    const JsonObject contactId = R"(
    {
        "_id": "123",
        "text": "123 Panic Audible",
        "on": "ALERT_PANIC_AUDIBLE",
        "off": "ALERT_PANIC_AUDIBLE_OFF"
    }
    )"_json;

    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );
    getDBClient().insert( DBNames::ContactIdsDB, contactId );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    const auto deviceId =
        addDevice( JSON( DeviceFields::NameFieldName
                         << "device" << DeviceFields::ProtectConfig << "0" << DeviceFields::GatewayIdFieldName
                         << gatewayId << DeviceFields::CategoryFieldName << "security_sensor"
                         << DeviceFields::SubcategoryFieldName << "motion" << DeviceFields::ArmedFieldName << true ) );

    const auto itemId = addSecurityItem( deviceId );

    constexpr bool value = true;
    const Json jsonValue = value;

    setItemValue( JSON( "_id" << itemId << "value" << jsonValue ) );
    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
    ASSERT_TRUE( json_contains( message.data, "protect" ) );
    ASSERT_TRUE( json_contains( message.data, "protectNotification" ) );
    ASSERT_TRUE( json_contains( message.data, "houseMode" ) );
    JsonObject protect;
    json_get_obj( message.data, "protect", protect );
    ASSERT_FALSE( json_contains( protect, "signal" ) );
    EXPECT_EQ( json_get_str( protect, "soundType" ), "police" );
    EXPECT_EQ( json_get_str( protect, "chime" ), "ring" );
}
}  // namespace UT
}  // namespace HubPlatform
