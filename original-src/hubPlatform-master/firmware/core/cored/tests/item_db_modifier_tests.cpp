#include "cored/tests/service/test_core_service.h"
#include "cored/tests/fakes/fake_feature_manager.h"

#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/configuration/configuration_factory.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "cored/db_modifier/security_item_db_modifier.h"
#include "test_utils/test_utils.h"

#include "cored/core_service.h"

#include "common_utils/cpp14.h"

#include "database_client/fake_db/fake_db_client.h"
#include "service_interfaces/bus_wrapper.h"
#include "bus/fake_bus.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

namespace HubPlatform {
namespace UT {

using namespace CoreD::Model::Resources;

class ItemDbModifierTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;
    void broadcaster( MessageSubClasses subClass, const Json& broadcast );
    IDbClient& getDBClient();
    const Services::Message& getBroadcastMessage() const;
    CoreD::ItemDbModifier& getItemDbModifier();
    CoreD::GatewayDbModifier& getGatewaysDbModifier();
    CoreD::IFeaturesManager& getFeatureManager();
    Services::Service& getService();
    void setEzloProtect( bool state, bool houseModeProtectState = true );
    void cleanDB();
    CoreD::Model::Configuration& getConfiguration();

    bool shouldUserBeNotified( const std::string& itemName,
                               const std::string& deviceCategory,
                               const std::string& deviceSubcategory,
                               const Json& broadcastData,
                               const Json& oldFieldsValues = JSON_EMPTY_OBJ );
    Json prepareUserLockOperationBroadcast( const IDbClient& db, const Json& data );
    Json prepareUserCodeActionBroadcast( const IDbClient& db, const Json& data );

    static JsonObject jsonValue( Json value );
    static JsonObject jsonValueArmed( Json value, bool isArmed );

    const std::string m_dbName = DBNames::ItemsDB;

    const CoreD::DeviceDbModifier::Broadcasts m_itemBroadcasts{
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_ITEM_ADDED ), {} },
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_ITEM_UPDATED ), {} },
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_ITEM_REMOVED ), {} }
    };
    void checkBroadcastWithTimestamp( const Json& message, const Json& expected, int codeLine );

private:
    std::unique_ptr< CoreD::IFeaturesManager > m_featureManager;
    Services::Message m_broadcastMessage;
    ProvisionDataUtils::ProvisionData m_provisionData;
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
    CoreD::Model::ConfigUPtr m_configUPtr;
    std::unique_ptr< CoreD::SwingerShutdown::SwingerShutdown > m_swingerShutdown;
    std::unique_ptr< CoreD::SecurityItemDbModifier > m_securityItemDbModifier;
    std::unique_ptr< CoreD::ItemDbModifier > m_itemDbModifier;
};

CoreD::Model::Configuration& ItemDbModifierTest::getConfiguration() {
    return *m_configUPtr;
}

void ItemDbModifierTest::SetUp() {
    m_featureManager = std::make_unique< CoreD::UT::FakeFeatureManager >();
    m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >();
    cleanDB();

    m_swingerShutdown = std::make_unique< CoreD::SwingerShutdown::SwingerShutdown >( *m_service );

    m_securityItemDbModifier =
        std::make_unique< CoreD::SecurityItemDbModifier >( *m_service, *m_featureManager, *m_swingerShutdown );

    m_itemDbModifier = std::make_unique< CoreD::ItemDbModifier >(
        *m_service,
        m_provisionData,
        [this]( MessageSubClasses subClass, const Json& broadcast ) { broadcaster( subClass, broadcast ); },
        *m_featureManager,
        *m_securityItemDbModifier );
    m_service->setUp();

    m_configUPtr = CoreD::Model::ConfigurationFactory::createConfiguration( m_service->getDBClient() );
}

void ItemDbModifierTest::TearDown() {
    cleanDB();
    if( m_service )
        m_service->tearDown();
}

void ItemDbModifierTest::broadcaster( MessageSubClasses subClass, const Json& broadcast ) {
    m_broadcastMessage.msgSubClass = subClass;
    m_broadcastMessage.data = broadcast;
}

IDbClient& ItemDbModifierTest::getDBClient() {
    return m_service->getDBClient();
}

const Services::Message& ItemDbModifierTest::getBroadcastMessage() const {
    return m_broadcastMessage;
}

CoreD::ItemDbModifier& ItemDbModifierTest::getItemDbModifier() {
    return *m_itemDbModifier;
}

CoreD::GatewayDbModifier& ItemDbModifierTest::getGatewaysDbModifier() {
    return m_service->getGatewaysDbModifier();
}

CoreD::IFeaturesManager& ItemDbModifierTest::getFeatureManager() {
    return *m_featureManager;
}

Services::Service& ItemDbModifierTest::getService() {
    return *m_service;
}

void ItemDbModifierTest::setEzloProtect( bool state, bool houseModeProtectState ) {
    using namespace HubPlatform::CoreDHouseModes;
    using namespace HubPlatform::CoreDHouseModes::Resources;

    getDBClient().insert( HUBSettings,
                          JSON( HouseModeFields::IdFieldName << HOUSE_MODE_ENTRY << HOUSE_MODE_ENTRY << "1" ) );
    getDBClient().insert( Resources::DBNames::HouseModesDB,
                          JSON( HouseModeFields::IdFieldName << "1"
                                                             << "protect" << houseModeProtectState ) );

    getFeatureManager().setFeatureStatus( "protect", state );
}

void ItemDbModifierTest::cleanDB() {
    getDBClient().clean();
}

bool ItemDbModifierTest::shouldUserBeNotified( const std::string& itemName,
                                               const std::string& deviceCategory,
                                               const std::string& deviceSubcategory,
                                               const Json& broadcastData,
                                               const Json& oldFieldsValues ) {
    return getItemDbModifier().shouldUserBeNotified(
        itemName, deviceCategory, deviceSubcategory, broadcastData, oldFieldsValues );
}

Json ItemDbModifierTest::prepareUserLockOperationBroadcast( const IDbClient& db, const Json& data ) {
    return CoreD::ItemDbModifier::prepareUserLockOperationBroadcast( db, data );
}

Json ItemDbModifierTest::prepareUserCodeActionBroadcast( const IDbClient& db, const Json& data ) {
    return CoreD::ItemDbModifier::prepareUserActionBroadcast( db, data );
}

JsonObject ItemDbModifierTest::jsonValue( Json value ) {
    return { { "value", std::move( value ) } };
}

JsonObject ItemDbModifierTest::jsonValueArmed( Json value, bool isArmed ) {
    return { { "value", value }, { "deviceArmed", isArmed } };
}

void ItemDbModifierTest::checkBroadcastWithTimestamp( const Json& message, const Json& expected, int codeLine ) {
    Json fwTimestamp;
    Json broadcast = message;
    EXPECT_TRUE( json_get( broadcast, System::FirmwareTimestampMs, fwTimestamp ) ) << __FILE__ << ":" << codeLine;
    json_remove( broadcast, System::FirmwareTimestampMs );
    EXPECT_EQ( broadcast, expected ) << __FILE__ << ":" << codeLine;
}

TEST_F( ItemDbModifierTest, userNotificationBattery ) {
    EXPECT_FALSE( shouldUserBeNotified( "battery", "", "", jsonValue( 30 ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "battery", "", "", jsonValue( 20 ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "battery", "", "", jsonValue( 10 ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "battery", "", "", jsonValue( 5 ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationSecurityThreat ) {
    EXPECT_FALSE( shouldUserBeNotified( "security_threat", "security_sensor1", "door", jsonValueArmed( true, true ) ) );

    EXPECT_FALSE(
        shouldUserBeNotified( "security_threat", "security_sensor", "door", jsonValueArmed( false, false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "security_threat", "security_sensor", "door", jsonValueArmed( false, true ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "security_threat", "security_sensor", "door", jsonValueArmed( true, false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "security_threat", "security_sensor", "door", jsonValueArmed( true, true ) ) );

    EXPECT_FALSE(
        shouldUserBeNotified( "security_threat", "security_sensor", "leak", jsonValueArmed( false, false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "security_threat", "security_sensor", "leak", jsonValueArmed( false, true ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "security_threat", "security_sensor", "leak", jsonValueArmed( true, false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "security_threat", "security_sensor", "leak", jsonValueArmed( true, true ) ) );

    EXPECT_FALSE(
        shouldUserBeNotified( "security_threat", "security_sensor", "motion", jsonValueArmed( false, false ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "security_threat", "security_sensor", "motion", jsonValueArmed( false, true ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "security_threat", "security_sensor", "motion", jsonValueArmed( true, false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "security_threat", "security_sensor", "motion", jsonValueArmed( true, true ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationSmokeAlarm ) {
    EXPECT_FALSE( shouldUserBeNotified( "smoke_alarm", "", "", jsonValueArmed( "smoke", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "smoke_alarm", "", "", jsonValueArmed( "no_smoke", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "smoke_alarm", "", "", jsonValueArmed( "no_smoke", true ) ) );
    EXPECT_FALSE( shouldUserBeNotified(
        "smoke_alarm", "", "", jsonValueArmed( "no_smoke", false ), jsonValueArmed( "unknown", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified(
        "smoke_alarm", "", "", jsonValueArmed( "no_smoke", true ), jsonValueArmed( "unknown", true ) ) );
    EXPECT_FALSE( shouldUserBeNotified(
        "smoke_alarm", "", "", jsonValueArmed( "no_smoke", false ), jsonValueArmed( "smoke_detected", false ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "smoke_alarm", "", "", jsonValueArmed( "no_smoke", true ), jsonValueArmed( "smoke_detected", true ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "smoke_alarm", "", "", jsonValueArmed( "smoke_detected", false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "smoke_alarm", "", "", jsonValueArmed( "smoke_detected", true ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationLockOperation ) {
    EXPECT_FALSE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "unlock_by_rf_with_invalid_user_code" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "lock_by_rf_with_invalid_user_code" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "manual_lock_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "keypad_lock_operation" ) ) );
    EXPECT_TRUE(
        shouldUserBeNotified( "lock_operation", "", "", jsonValue( "auto_lock_not_fully_locked_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "rf_not_fully_locked_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "manual_not_fully_locked_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "lock_jammed" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "rf_lock_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "manual_unlock_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "rf_unlock_operation" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "lock_operation", "", "", jsonValue( "keypad_unlock_operation" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationTamperingCoverAlarm ) {
    EXPECT_FALSE( shouldUserBeNotified( "tampering_cover_alarm", "", "", jsonValue( "no_value" ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "tampering_cover_alarm", "", "", jsonValue( "no_tampering_cover" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "tampering_cover_alarm", "", "", jsonValue( "tampering_cover" ) ) );
    EXPECT_FALSE( shouldUserBeNotified(
        "tampering_cover_alarm", "", "", jsonValue( "no_tampering_cover" ), jsonValue( "unknown" ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "tampering_cover_alarm", "", "", jsonValue( "tampering_cover" ), jsonValue( "unknown" ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "tampering_cover_alarm", "", "", jsonValue( "no_tampering_cover" ), jsonValue( "tampering_cover" ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "tampering_cover_alarm", "", "", jsonValue( "tampering_cover" ), jsonValue( "no_tampering_cover" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationDwStateItems ) {
    EXPECT_FALSE( shouldUserBeNotified( "dw_state", "", "", jsonValueArmed( "no_value", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "dw_state", "", "", jsonValueArmed( "no_value", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "dw_state", "", "", jsonValueArmed( "dw_is_closed", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "dw_state", "", "", jsonValueArmed( "dw_is_closed", true ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "dw_state", "", "", jsonValueArmed( "dw_is_closed", false ), jsonValue( "unknown" ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "dw_state", "", "", jsonValueArmed( "dw_is_closed", true ), jsonValue( "unknown" ) ) );
    EXPECT_FALSE( shouldUserBeNotified(
        "dw_state", "", "", jsonValueArmed( "dw_is_closed", false ), jsonValue( "dw_is_opened" ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "dw_state", "", "", jsonValueArmed( "dw_is_closed", true ), jsonValue( "dw_is_opened" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationWaterLeakAlarm ) {
    EXPECT_FALSE( shouldUserBeNotified( "water_leak_alarm", "", "", jsonValueArmed( "water_leak_detected", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "water_leak_alarm", "", "", jsonValueArmed( "water_leak_detected", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "water_leak_alarm", "", "", jsonValueArmed( "no_water_leak", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "water_leak_alarm", "", "", jsonValueArmed( "no_water_leak", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified(
        "water_leak_alarm", "", "", jsonValueArmed( "no_water_leak", false ), jsonValueArmed( "unknown", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified(
        "water_leak_alarm", "", "", jsonValueArmed( "no_water_leak", true ), jsonValueArmed( "unknown", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "water_leak_alarm",
                                        "",
                                        "",
                                        jsonValueArmed( "no_water_leak", false ),
                                        jsonValueArmed( "water_leak_detected", false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "water_leak_alarm",
                                       "",
                                       "",
                                       jsonValueArmed( "no_water_leak", true ),
                                       jsonValueArmed( "water_leak_detected", false ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationMotion ) {
    EXPECT_FALSE( shouldUserBeNotified( "motion", "", "", jsonValueArmed( true, false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "motion", "", "", jsonValueArmed( true, true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "motion", "", "", jsonValueArmed( true, false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "motion", "", "", jsonValueArmed( false, true ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationCoAlarm ) {
    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_value", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_value", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "co_detected", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "co_detected", true ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "co_detected", false ), jsonValue( "unknown" ) ) );
    EXPECT_TRUE(
        shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "co_detected", true ), jsonValue( "unknown" ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "co_detected", false ), jsonValue( "no_co" ) ) );
    EXPECT_TRUE(
        shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "co_detected", true ), jsonValue( "no_co" ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_co", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_co", true ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_co", false ), jsonValue( "unknown" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_co", true ), jsonValue( "unknown" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_co", false ), jsonValue( "no_co" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "co_alarm", "", "", jsonValueArmed( "no_co", true ), jsonValue( "no_co" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationGlassBreakageAlarm ) {
    EXPECT_FALSE( shouldUserBeNotified( "glass_breakage_alarm", "", "", jsonValueArmed( "no_value", false ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "glass_breakage_alarm", "", "", jsonValueArmed( "no_value", true ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "glass_breakage_alarm", "", "", jsonValueArmed( "glass_breakage", false ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "glass_breakage_alarm", "", "", jsonValueArmed( "glass_breakage", true ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationUserCodeOperation ) {
    EXPECT_FALSE( shouldUserBeNotified( "user_code_operation", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "user_code_operation", "", "", jsonValue( "new_program_code_entered" ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "user_code_operation", "", "", jsonValue( "new_user_code_not_added_due_to_duplicated_code" ) ) );
    EXPECT_TRUE( shouldUserBeNotified(
        "user_code_operation", "", "", jsonValue( "manually_enter_user_code_exceeds_code_limit" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "user_code_operation", "", "", jsonValue( "new_user_code_added" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "user_code_operation", "", "", jsonValue( "single_user_code_deleted" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "user_code_operation", "", "", jsonValue( "all_user_codes_deleted" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationThermostatMode ) {
    EXPECT_FALSE( shouldUserBeNotified( "thermostat_mode", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "thermostat_mode", "", "", jsonValue( "off" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_mode", "", "", jsonValue( "heat" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_mode", "", "", jsonValue( "cool" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_mode", "", "", jsonValue( "auto" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationThermostatFanMode ) {
    EXPECT_FALSE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "low" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "high" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "medium" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "auto_low" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "auto_high" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "thermostat_fan_mode", "", "", jsonValue( "auto_medium" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationSwitch ) {
    EXPECT_FALSE( shouldUserBeNotified( "switch", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "switch", "", "", jsonValue( true ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "switch", "", "", jsonValue( false ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationBarrierState ) {
    EXPECT_FALSE( shouldUserBeNotified( "barrier_state", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified( "barrier_state", "", "", jsonValue( "barrier_opened" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "barrier_state", "", "", jsonValue( "barrier_closed" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationWaterTemperatureAlarmState ) {
    EXPECT_FALSE( shouldUserBeNotified( "water_temperature_alarm", "", "", jsonValue( "no_value" ) ) );

    EXPECT_TRUE( shouldUserBeNotified(
        "water_temperature_alarm", "", "", jsonValue( "water_temperature_below_low_threshold" ) ) );

    EXPECT_TRUE( shouldUserBeNotified(
        "water_temperature_alarm", "", "", jsonValue( "water_temperature_above_high_threshold" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationHeatAlarmState ) {
    constexpr char Heat[] = "heat";
    constexpr char HeatAlarm[] = "heat_alarm";
    constexpr char SecuritySensor[] = "security_sensor";
    constexpr char OverheatDetected[] = "overheat_detected";
    constexpr char UnderHeatDetected[] = "under_heat_detected";

    EXPECT_FALSE( shouldUserBeNotified( HeatAlarm, SecuritySensor, Heat, jsonValue( OverheatDetected ) ) );

    EXPECT_FALSE( shouldUserBeNotified( HeatAlarm, SecuritySensor, Heat, jsonValue( UnderHeatDetected ) ) );

    EXPECT_TRUE( shouldUserBeNotified( HeatAlarm, SecuritySensor, Heat, jsonValueArmed( OverheatDetected, true ) ) );

    EXPECT_TRUE( shouldUserBeNotified( HeatAlarm, SecuritySensor, Heat, jsonValueArmed( UnderHeatDetected, true ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationDoorBellRingState ) {
    EXPECT_TRUE( shouldUserBeNotified( "doorbell_state", "", "", jsonValue( "ring" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationPanicButton ) {
    EXPECT_TRUE( shouldUserBeNotified( "security_threat", "scene_controller", "panic", jsonValueArmed( true, true ) ) );
    EXPECT_FALSE(
        shouldUserBeNotified( "security_threat", "scene_controller", "panic", jsonValueArmed( true, false ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationHealthItems ) {
    auto jsonValue = []( const std::string& value ) -> JsonObject { return { { "value", value } }; };
    EXPECT_TRUE( shouldUserBeNotified( "position", "any", "any", jsonValue( "leaving_bed" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "position", "any", "any", jsonValue( "sitting_on_bed" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "position", "any", "any", jsonValue( "lying_on_bed" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "position", "any", "any", jsonValue( "sitting_on_bed_edge" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationHwState ) {
    EXPECT_FALSE( shouldUserBeNotified( "hw_state", "", "", jsonValue( "unknown" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "hw_state", "", "", jsonValue( "hardware_ok" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "hw_state", "", "", jsonValue( "hardware_failure" ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "hw_state", "", "", jsonValue( "hardware_ok" ), jsonValue( "unknown" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "hw_state", "", "", jsonValue( "hardware_failure" ), jsonValue( "unknown" ) ) );

    EXPECT_TRUE(
        shouldUserBeNotified( "hw_state", "", "", jsonValue( "hardware_ok" ), jsonValue( "hardware_failure" ) ) );
    EXPECT_TRUE(
        shouldUserBeNotified( "hw_state", "", "", jsonValue( "hardware_failure" ), jsonValue( "hardware_ok" ) ) );
}

TEST_F( ItemDbModifierTest, userNotificationSwState ) {
    EXPECT_FALSE( shouldUserBeNotified( "sw_state", "", "", jsonValue( "unknown" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "sw_state", "", "", jsonValue( "software_ok" ) ) );
    EXPECT_FALSE( shouldUserBeNotified( "sw_state", "", "", jsonValue( "software_failure" ) ) );

    EXPECT_FALSE( shouldUserBeNotified( "sw_state", "", "", jsonValue( "software_ok" ), jsonValue( "unknown" ) ) );
    EXPECT_TRUE( shouldUserBeNotified( "sw_state", "", "", jsonValue( "software_failure" ), jsonValue( "unknown" ) ) );

    EXPECT_TRUE(
        shouldUserBeNotified( "sw_state", "", "", jsonValue( "software_ok" ), jsonValue( "software_failure" ) ) );
    EXPECT_TRUE(
        shouldUserBeNotified( "sw_state", "", "", jsonValue( "software_failure" ), jsonValue( "software_ok" ) ) );
}

TEST_F( ItemDbModifierTest, prepareUserLockOperationBroadcast ) {
    auto& db = getDBClient();
    db.insert( DBNames::ItemsDB, R"(
    {
        "_id": "5df0b9e4e0dabe58a5a60021",
        "deviceId": "5df0b9e4dfdabe58a5a60020",
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes",
        "show": true,
        "userCodeRestriction": "^\\d{4,10}$",
        "value": {
            "1": {
                "code": "2812",
                "name": "One"
            },
            "2": {
                "code": "775544",
                "name": "Two"
            },
            "3": {
                "code": "0085744",
                "name": "Three"
            },
            "4": {
                "code": "886234",
                "name": "Four"
            }
        },
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json );

    const auto userLockOperationBroadcast = R"(
    {
        "_id": "5df0b9e5e0dabe58a5a6002b",
        "deviceCategory": "door_lock",
        "deviceId": "5df0b9e4dfdabe58a5a60020",
        "deviceName": "Touchscreen Deadbolt",
        "deviceSubcategory": "",
        "name": "user_lock_operation",
        "notifications": null,
        "roomName": "Good Room",
        "serviceNotification": false,
        "userNotification": false,
        "value": {
            "action": "unlock",
            "userId": 3
        },
        "valueFormatted": "",
        "valueType": "user_lock_operation"
    }
    )"_json;

    {
        const auto broadcast = prepareUserLockOperationBroadcast( getDBClient(), userLockOperationBroadcast );
        const auto expectedBroadcast = R"(
        {
            "type": "pin_code_is_used",
            "params": {
                  "user": "Three",
                  "deviceName": "Touchscreen Deadbolt",
                  "roomName": "Good Room"
            }
        }
        )"_json;
        EXPECT_EQ( broadcast.dump(), expectedBroadcast.dump() );
    }

    {
        const auto eraseThreeCode( R"(
        {
            "value": {
                "1": {
                    "code": "2812",
                    "name": "One"
                },
                "2": {
                    "code": "775544",
                    "name": "Two"
                },
                "4": {
                    "code": "886234",
                    "name": "Four"
                }
            }
        }
        )"_json );
        db.update( DBNames::ItemsDB,
                   JSON( "_id"
                         << "5df0b9e4e0dabe58a5a60021" ),
                   JSON( "$set" << eraseThreeCode ) );
        const auto broadcast = prepareUserLockOperationBroadcast( getDBClient(), userLockOperationBroadcast );
        const auto expectedBroadcast = R"(
        {
            "type": "pin_code_is_used",
            "params": {
                  "user": "Code 3",
                  "deviceName": "Touchscreen Deadbolt",
                  "roomName": "Good Room"
            }
        }
        )"_json;
        EXPECT_EQ( broadcast.dump(), expectedBroadcast.dump() );
    }

    {
        const auto userLockOperationBroadcastWithUnknownAction = R"(
        {
            "_id": "5df0b9e5e0dabe58a5a6002b",
            "deviceCategory": "door_lock",
            "deviceId": "5df0b9e4dfdabe58a5a60020",
            "deviceName": "Touchscreen Deadbolt",
            "deviceSubcategory": "",
            "name": "user_lock_operation",
            "notifications": null,
            "roomName": "Good Room",
            "serviceNotification": false,
            "userNotification": false,
            "value": {
                "action": "unknown",
                "userId": -1
            },
            "valueFormatted": "",
            "valueType": "user_lock_operation"
        }
        )"_json;

        const auto broadcast =
            prepareUserLockOperationBroadcast( getDBClient(), userLockOperationBroadcastWithUnknownAction );
        EXPECT_TRUE( broadcast.empty() );
    }
}

TEST_F( ItemDbModifierTest, prepareUserCodeActionBroadcast ) {
    auto& db = getDBClient();
    db.insert( DBNames::ItemsDB, R"(
    {
        "_id": "5df0b9e4e0dabe58a5a60021",
        "deviceId": "5df0b9e4dfdabe58a5a60020",
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes",
        "show": true,
        "userCodeRestriction": "^\\d{4,10}$",
        "value": {
            "1": {
                "code": "2812",
                "name": "One"
            },
            "2": {
                "code": "775544",
                "name": "Two"
            },
            "3": {
                "code": "0085744",
                "name": "Three"
            },
            "4": {
                "code": "886234",
                "name": "Four"
            }
        },
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json );

    const auto userCodeActionBroadcast = R"(
    {
        "_id": "5df0b9e5e0dabe58a5a6002b",
        "deviceCategory": "door_lock",
        "deviceId": "5df0b9e4dfdabe58a5a60020",
        "deviceName": "Touchscreen Deadbolt",
        "deviceSubcategory": "",
        "name": "user_code_action",
        "notifications": null,
        "roomName": "Good Room",
        "serviceNotification": false,
        "userNotification": false,
        "value": {
            "action": "user_code_entered",
            "userId": 3
        },
        "valueFormatted": "",
        "valueType": "user_code_action"
    }
    )"_json;

    {
        const auto broadcast = prepareUserCodeActionBroadcast( getDBClient(), userCodeActionBroadcast );
        const auto expectedBroadcast = R"(
        {
            "type": "pin_code_is_used",
            "params": {
                  "user": "Three",
                  "deviceName": "Touchscreen Deadbolt",
                  "roomName": "Good Room"
            }
        }
        )"_json;
        EXPECT_EQ( broadcast.dump(), expectedBroadcast.dump() );
    }

    {
        const auto eraseThreeCode( R"(
        {
            "value": {
                "1": {
                    "code": "2812",
                    "name": "One"
                },
                "2": {
                    "code": "775544",
                    "name": "Two"
                },
                "4": {
                    "code": "886234",
                    "name": "Four"
                }
            }
        }
        )"_json );
        db.update( DBNames::ItemsDB,
                   JSON( "_id"
                         << "5df0b9e4e0dabe58a5a60021" ),
                   JSON( "$set" << eraseThreeCode ) );
        const auto broadcast = prepareUserCodeActionBroadcast( getDBClient(), userCodeActionBroadcast );
        const auto expectedBroadcast = R"(
        {
            "type": "pin_code_is_used",
            "params": {
                  "user": "Code 3",
                  "deviceName": "Touchscreen Deadbolt",
                  "roomName": "Good Room"
            }
        }
        )"_json;
        EXPECT_EQ( broadcast.dump(), expectedBroadcast.dump() );
    }

    {
        const auto userCodeActionBroadcastWithUnknownCode = R"(
        {
            "_id": "5df0b9e5e0dabe58a5a6002b",
            "deviceCategory": "door_lock",
            "deviceId": "5df0b9e4dfdabe58a5a60020",
            "deviceName": "Touchscreen Deadbolt",
            "deviceSubcategory": "",
            "name": "user_code_action",
            "notifications": null,
            "roomName": "Good Room",
            "serviceNotification": false,
            "userNotification": false,
            "value": {
                "action": "invalid_user_code_entered",
                "userId": -1
            },
            "valueFormatted": "",
            "valueType": "user_code_action"
        }
        )"_json;

        const auto broadcast = prepareUserCodeActionBroadcast( getDBClient(), userCodeActionBroadcastWithUnknownCode );
        EXPECT_TRUE( broadcast.empty() );
    }
}

TEST_F( ItemDbModifierTest, updateItem ) {
    const JsonObject device = R"(
    {
        "_id": "device_id1",
        "batteryPowered": false,
        "category": "dimmable_light",
        "deviceTypeId": "134_3_25",
        "gatewayId": "gateway_id1",
        "name": "Dimmer",
        "parentDeviceId": "",
        "persistent": false,
        "reachable": true,
        "ready": true,
        "roomId": "",
        "security": "no",
        "status": "idle",
        "subcategory": "dimmable_plugged",
        "type": "dimmer.outlet"
    }
    )"_json;

    const JsonObject item = R"(
    {
        "_id": "item_id1",
        "deviceId": "device_id1",
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": true,
        "valueFormatted": "true",
        "valueType": "bool"
    }
    )"_json;

    const JsonObject expectedBroadcast = R"(
    {
        "_id": "item_id1",
        "deviceCategory": "dimmable_light",
        "deviceId": "device_id1",
        "deviceName": "Dimmer",
        "deviceSubcategory": "dimmable_plugged",
        "name": "switch",
        "notifications": null,
        "protectConfig":"",
        "roomName": "",
        "syncNotification": false,
        "userNotification": true,
        "value": false
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );

    getItemDbModifier().insert( item );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    JsonObject updatePortion;
    updatePortion[ ItemFields::ValueFieldName ] = false;

    getItemDbModifier().update( "item_id1", updatePortion, false );
    const Services::Message& message = getBroadcastMessage();
    EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.updateOperation.classType );
    checkBroadcastWithTimestamp( message.data, expectedBroadcast, __LINE__ );
    EXPECT_EQ( message.data.count( System::FirmwareTimestampMs ), 1 );
}

TEST_F( ItemDbModifierTest, updateItemWithParentDevice ) {
    const JsonObject device = R"(
    {
        "_id": "device_id1",
        "batteryPowered": false,
        "category": "dimmable_light",
        "deviceTypeId": "134_3_25",
        "gatewayId": "gateway_id1",
        "name": "Dimmer",
        "parentDeviceId": "parent_device_id1",
        "persistent": false,
        "reachable": true,
        "ready": true,
        "roomId": "",
        "security": "no",
        "status": "idle",
        "subcategory": "dimmable_plugged",
        "type": "dimmer.outlet"
    }
    )"_json;

    const JsonObject item = R"(
    {
        "_id": "item_id1",
        "deviceId": "device_id1",
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": true,
        "valueFormatted": "true",
        "valueType": "bool"
    }
    )"_json;

    const JsonObject expectedBroadcast = R"(
    {
        "_id": "item_id1",
        "deviceCategory": "dimmable_light",
        "deviceId": "device_id1",
        "deviceName": "Dimmer",
        "deviceSubcategory": "dimmable_plugged",
        "name": "switch",
        "notifications": null,
        "parentDeviceId": "parent_device_id1",
        "protectConfig":"",
        "roomName": "",
        "syncNotification": false,
        "userNotification": true,
        "value": false
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );

    getItemDbModifier().insert( item );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    JsonObject updatePortion;
    updatePortion[ ItemFields::ValueFieldName ] = false;

    getItemDbModifier().update( "item_id1", updatePortion, false );
    const Services::Message& message = getBroadcastMessage();
    EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.updateOperation.classType );
    checkBroadcastWithTimestamp( message.data, expectedBroadcast, __LINE__ );
}

TEST_F( ItemDbModifierTest, updateDictionary ) {
    const JsonObject item = R"(
    {
        "_id": "id_1",
        "deviceId": "5df780747f000004b1eb356f",
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

    const JsonObject broadcast = R"(
    {
        "_id": "id_1",
        "deviceCategory": "",
        "deviceId": "5df780747f000004b1eb356f",
        "deviceName": "",
        "deviceSubcategory": "",
        "element": {},
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "name": "user_codes",
        "notifications": null,
        "operation": "",
        "protectConfig":"",
        "roomName": "",
        "userNotification": false,
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json;

    getItemDbModifier().insert( item );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    auto testUpdateDictionary = [this, &broadcast]( const JsonObject& updatedValue,
                                                    const JsonObject& oldValue,
                                                    const JsonObject& updatePortion,
                                                    const std::string& operation,
                                                    const std::string& dictionaryId,
                                                    int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        JsonObject expectedBroadcast = broadcast;
        expectedBroadcast[ "operation" ] = operation;
        expectedBroadcast[ "element" ][ dictionaryId ][ "value" ] = updatedValue;

        expectedBroadcast[ System::SyncNotification ] = json_contains( updatePortion, ItemFields::NameFieldName );

        if( operation == "updated" ) {
            expectedBroadcast[ DictionaryEditRequestFields::OldElementName ] =
                JSON( dictionaryId << JSON( "value" << oldValue ) );
        }

        getItemDbModifier().updateDictionary( "id_1", updatePortion, { dictionaryId, updatedValue, operation } );

        checkBroadcastWithTimestamp( getBroadcastMessage().data, expectedBroadcast, __LINE__ );

        EXPECT_EQ( getBroadcastMessage().msgSubClass, HUB_ITEM_DICTIONARY_UPDATED ) << codeLineStr;

        const auto result =
            getDBClient().findOne( TemporaryTableNamePreffix + m_dbName, JSON( db::ID_FIELD_NAME << "id_1" ), nullptr );
        JsonObject expectedItem = updatePortion;
        expectedItem[ "_id" ] = "id_1";
        EXPECT_EQ( result.dump(), expectedItem.dump() ) << codeLineStr;
    };

    {  // add new element
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

        testUpdateDictionary( newValue, JSON_EMPTY_OBJ, updatePortion, "added", "2", __LINE__ );
    }
    {  // update element
        const JsonObject updatedValue = R"(
        {
            "code": "9999",
            "name": "Nines"
        }
        )"_json;

        const JsonObject oldValue = R"(
        {
            "code": "1234",
            "name": "One"
        }
        )"_json;

        const JsonObject updatePortion = R"(
        {
            "value": {
                "1" : {
                    "code": "9999",
                    "name": "Nines"
                },
                "2" : {
                    "code": "2812",
                    "name": "Two"
                }
            }
        }
        )"_json;

        testUpdateDictionary( updatedValue, oldValue, updatePortion, "updated", "1", __LINE__ );
    }

    {  // remove element
        const JsonObject removedValue = R"(
        {
            "code": "9999",
            "name": "Nines"
        }
        )"_json;

        const JsonObject updatePortion = R"(
        {
            "value": {
                "2" : {
                    "code": "2812",
                    "name": "Two"
                }
            }
        }
        )"_json;

        testUpdateDictionary( removedValue, JSON_EMPTY_OBJ, updatePortion, "removed", "1", __LINE__ );
    }

    {  // update name
        std::string initialName = item[ ItemFields::NameFieldName ];
        const auto originalPortion = JSON( ItemFields::NameFieldName << initialName );

        getItemDbModifier().updateDictionary( "id_1", originalPortion, {} );
        EXPECT_FALSE( json_get_bool( getBroadcastMessage().data, System::SyncNotification ) );

        auto updatePortion = JSON( ItemFields::NameFieldName << "new_name" );
        getItemDbModifier().updateDictionary( "id_1", updatePortion, {} );
        EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, System::SyncNotification ) );

        getItemDbModifier().updateDictionary( "id_1", originalPortion, {} );
        EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, System::SyncNotification ) );
    }
}

TEST_F( ItemDbModifierTest, minMaxValuesBroadcastsTest ) {
    std::string gatewayId = "1";
    std::string deviceId = "2";

    // scale of these objects is 'celsius'
    JsonObject item = R"(
    {
        "_id": "id_1",
        "deviceId": "2",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "temperature_setpoint",
        "type": "temperature",
        "show": true,
        "value": {
             "value": 20, "scale": "celsius"
        },
        "valueType": "temperature",
        "valueMin" : {
             "value": 0, "scale": "celsius"
        },
        "valueMax" : {
            "value": 60, "scale": "celsius"
        }
    }
    )"_json;

    const JsonObject itemAddedBroadcast = R"(
    {
        "_id": "id_1",
        "deviceId": "2",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "maxValue": 60,
        "minValue": 0,
        "value": 20,
        "valueFormatted": "20",
        "scale": "celsius",
        "protectConfig":"",
        "show": true,
        "syncNotification": true,
        "name": "temperature_setpoint",
        "valueType": "temperature"
    }
    )"_json;

    // convert value to hub's "scale.temperature" setting
    auto convertFromScale = [this]( double fromValue, const char* fromScale ) -> double {
        using namespace HubPlatform::CoreD::Model;
        const auto valueType = ValueType::fromString( "temperature" );
        auto scale = getConfiguration().getScaleForValueType( valueType );
        auto converter = valueType.getConverter();
        return converter( fromValue, fromScale, scale.c_str() );
    };

    auto& dbClient = getDBClient();
    auto& itemDbModifier = getItemDbModifier();
    dbClient.insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    dbClient.insert( DBNames::DevicesDB,
                     JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::GatewayIdFieldName << gatewayId ) );
    item[ ItemFields::DeviceIdFieldName ] = deviceId;

    std::string itemId, dump;
    // Add items with valueMin/valueMax properties, hub.item.added broadcast test
    {
        itemDbModifier.insert( item );
        const Services::Message& message = getBroadcastMessage();
        EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.addOperation.classType );
        dump = message.data.dump();
        EXPECT_EQ( message.data.count( "minValue" ), 1 ) << "Should have fields minValue/maxValue: " << dump;
        EXPECT_EQ( message.data.count( "minValue" ), 1 ) << "Should have fields minValue/maxValue: " << dump;
        EXPECT_EQ( message.data.count( "valueMax" ), 0 )
            << "Should have no deprecated fields valueMin/valueMax: " << dump;
        EXPECT_EQ( message.data.count( "valueMax" ), 0 )
            << "Should have no deprecated fields valueMin/valueMax: " << dump;

        std::set< std::string > excludeFields = { "minValue", "maxValue", "value", "scale", "valueFormatted" };
        using Strings = std::vector< std::string >;
        Strings difference;
        auto keys = json_object_keys( message.data ).get< Strings >();
        std::sort( begin( keys ), end( keys ) );
        std::set_difference( begin( keys ),
                             end( keys ),
                             begin( excludeFields ),
                             end( excludeFields ),
                             std::back_inserter( difference ) );

        Json diffMessage, diffBroadcast;
        jsonCopy( message.data, difference, diffMessage );
        json_remove( diffMessage, System::FirmwareTimestampMs );
        jsonCopy( itemAddedBroadcast, difference, diffBroadcast );
        EXPECT_EQ( diffMessage, diffBroadcast ) << "Not scalable fields should match: message " << cc::j( diffMessage )
                                                << " expected " << cc::j( diffBroadcast );

        // Avoid comparing dumps, because of scales, compare with scale instead
        auto scale = json_get_str( itemAddedBroadcast, "scale" );
        for( const auto& field : Strings{ "value", "minValue", "maxValue" } ) {
            float responseValue = json_get_float( message.data, field );
            float expectedValue = json_get_float( itemAddedBroadcast, field );
            float expectedConverted = convertFromScale( expectedValue, scale.c_str() );
            EXPECT_EQ( responseValue, expectedConverted );
        }
    }

    // Update item with valueMin properties hub.item.updated broadcast test
    auto optionalFieldUpdate = [&]( const std::string& itemField, const std::string& responseField, float newValue ) {
        item[ itemField ][ "value" ] = newValue;
        itemDbModifier.update( "id_1", item, false );
        const Services::Message& message = getBroadcastMessage();
        EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.updateOperation.classType );
        dump = message.data.dump();
        Log::info( "optionalFieldUpdate: " + dump );
        EXPECT_EQ( message.data.count( itemField ), 0 ) << "Should have no '" << itemField << "' field, deprecated";
        EXPECT_EQ( message.data.count( responseField ), 1 ) << "Should have field: '" << responseField << "'";
        float fromResponse = json_get_float( message.data, responseField, -101 );
        auto itemScale = json_get_str( item[ itemField ], "scale" );
        EXPECT_FLOAT_EQ( fromResponse, convertFromScale( newValue, itemScale.c_str() ) ) << "item.updated: " << dump;
    };

    optionalFieldUpdate( "valueMin", "minValue", -10 );
    optionalFieldUpdate( "valueMax", "maxValue", 50 );

    getGatewaysDbModifier().remove( gatewayId );
}

TEST_F( ItemDbModifierTest, minMaxValuesHumidityBroadcastsTest ) {
    std::string gatewayId = "1";
    std::string deviceId = "2";

    JsonObject item = R"(
    {
        "_id": "id_1",
        "deviceId": "2",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "humidity",
        "type": "humidity",
        "show": true,
        "value": {
             "value": 40, "scale": "percent"
        },
        "valueType": "temperature",
        "valueMin" : {
             "value": 0, "scale": "percent"
        },
        "valueMax" : {
            "value": 80, "scale": "percent"
        }
    }
    )"_json;

    const JsonObject itemAddedBroadcast = R"(
    {
        "_id": "id_1",
        "deviceId": "2",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "maxValue": 80,
        "minValue": 0,
        "value": 40,
        "valueFormatted": "40",
        "protectConfig":"",
        "scale": "percent",
        "show": true,
        "syncNotification": true,
        "name": "humidity",
        "valueType": "humidity"
    }
    )"_json;

    auto& dbClient = getDBClient();
    auto& itemDbModifier = getItemDbModifier();
    dbClient.insert( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ) );
    dbClient.insert( DBNames::DevicesDB,
                     JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::GatewayIdFieldName << gatewayId ) );
    item[ ItemFields::DeviceIdFieldName ] = deviceId;

    std::string itemId, dump;
    // Add items with valueMin/valueMax properties, hub.item.added broadcast test
    {
        itemDbModifier.insert( item );
        const Services::Message& message = getBroadcastMessage();
        EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.addOperation.classType );
        EXPECT_EQ( message.data.count( "minValue" ), 1 ) << "Should have fields minValue/maxValue: " << dump;
        EXPECT_EQ( message.data.count( "minValue" ), 1 ) << "Should have fields minValue/maxValue: " << dump;
        EXPECT_EQ( message.data.count( "valueMax" ), 0 )
            << "Should have no deprecated fields valueMin/valueMax: " << dump;
        EXPECT_EQ( message.data.count( "valueMax" ), 0 )
            << "Should have no deprecated fields valueMin/valueMax: " << dump;
        checkBroadcastWithTimestamp( message.data, itemAddedBroadcast, __LINE__ );
    }

    // Update item with valueMin properties hub.item.updated broadcast test
    auto optionalFieldUpdate = [&]( const std::string& itemField, const std::string& responseField, float newValue ) {
        item[ itemField ][ "value" ] = newValue;
        itemDbModifier.update( "id_1", item, false );
        const Services::Message& message = getBroadcastMessage();
        EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.updateOperation.classType );
        dump = message.data.dump();
        EXPECT_EQ( message.data.count( itemField ), 0 ) << "Should have no '" << itemField << "' field, deprecated";
        EXPECT_EQ( message.data.count( responseField ), 1 ) << "Should have field: '" << responseField << "'";
        EXPECT_FLOAT_EQ( json_get_float( message.data, responseField, -101 ), newValue ) << "item.updated: " << dump;
    };

    optionalFieldUpdate( "valueMin", "minValue", 10 );
    optionalFieldUpdate( "valueMax", "maxValue", 50 );

    getGatewaysDbModifier().remove( gatewayId );
}

TEST_F( ItemDbModifierTest, ezloProtectCheck ) {
    JsonObject updatePortion;
    const auto item = R"(
    {
        "_id": "item_id1",
        "name": "smoke_alarm",
        "value": "no_smoke"
    }
    )"_json;

    getItemDbModifier().insert( item );

    setEzloProtect( false );

    updatePortion[ ItemFields::ValueFieldName ] = "smoke_detected";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_FALSE( json_get_bool( getBroadcastMessage().data, "protectNotification", false ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 0 );

    setEzloProtect( true, false );

    updatePortion[ ItemFields::ValueFieldName ] = "no_smoke";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    updatePortion[ ItemFields::ValueFieldName ] = "smoke_detected";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", false ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 1 );

    setEzloProtect( true );

    updatePortion[ ItemFields::ValueFieldName ] = "no_smoke";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    updatePortion[ ItemFields::ValueFieldName ] = "smoke_detected";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", false ) );
}

TEST_F( ItemDbModifierTest, protectNotificationSmokeDetected ) {
    JsonObject updatePortion;
    const auto item = R"(
    {
        "_id": "item_id1",
        "name": "smoke_alarm",
        "value": "no_smoke"
    }
    )"_json;

    getItemDbModifier().insert( item );
    setEzloProtect( true );

    updatePortion[ ItemFields::ValueFieldName ] = "smoke_detected";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", false ) );

    updatePortion[ ItemFields::ValueFieldName ] = "no_smoke";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 1 );
}

TEST_F( ItemDbModifierTest, protectNotificationCarbonMonoxideDetected ) {
    JsonObject updatePortion;
    const auto item = R"(
    {
        "_id": "item_id1",
        "name": "co_alarm",
        "deviceId": "device_id1",
        "value": "no_co"
    }
    )"_json;

    const JsonObject device = R"(
    {
        "_id": "device_id1",
		"armed": true
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getItemDbModifier().insert( item );
    setEzloProtect( true );

    updatePortion[ ItemFields::ValueFieldName ] = "co_detected";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );

    updatePortion[ ItemFields::ValueFieldName ] = "no_co";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 1 );
}

TEST_F( ItemDbModifierTest, protectNotificationGlassWasBroken ) {
    JsonObject updatePortion;
    const auto item = R"(
    {
        "_id": "item_id1",
        "name": "glass_breakage_alarm",
        "deviceId": "device_id1",
        "value": "no_glass_breakage"
    }
    )"_json;

    const JsonObject device = R"(
    {
        "_id": "device_id1",
		"armed": true
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getItemDbModifier().insert( item );
    setEzloProtect( true );

    updatePortion[ ItemFields::ValueFieldName ] = "glass_breakage";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );

    updatePortion[ ItemFields::ValueFieldName ] = "no_glass_breakage";
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 1 );
}

TEST_F( ItemDbModifierTest, protectNotificationDWIsOpen ) {
    JsonObject updatePortion;
    const auto item = R"(
    {
        "_id": "item_id1",
        "name": "security_threat",
        "deviceId": "device_id1",
        "value": false
    }
    )"_json;

    const JsonObject device = R"(
    {
        "_id": "device_id1",
		"armed": true,
		"category": "security_sensor",
		"subcategory": "door"
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getItemDbModifier().insert( item );
    setEzloProtect( true );

    updatePortion[ ItemFields::ValueFieldName ] = true;
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", false ) );

    updatePortion[ ItemFields::ValueFieldName ] = false;
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 1 );
}

TEST_F( ItemDbModifierTest, protectNotificationMotionDetected ) {
    JsonObject updatePortion;
    const auto item = R"(
    {
        "_id": "item_id1",
        "name": "security_threat",
        "deviceId": "device_id1",
        "value": false
    }
    )"_json;

    const JsonObject device = R"(
    {
        "_id": "device_id1",
		"armed": true,
		"category": "security_sensor",
		"subcategory": "motion"
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getItemDbModifier().insert( item );
    setEzloProtect( true );

    updatePortion[ ItemFields::ValueFieldName ] = true;
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", false ) );

    updatePortion[ ItemFields::ValueFieldName ] = false;
    getItemDbModifier().update( "item_id1", updatePortion, false );
    EXPECT_TRUE( json_get_bool( getBroadcastMessage().data, "protectNotification", true ) );
    EXPECT_EQ( getBroadcastMessage().data.count( "protectNotification" ), 1 );
}

TEST_F( ItemDbModifierTest, protectUpdateItem ) {
    const JsonObject device = R"(
    {
        "_id": "device_id1",
        "batteryPowered": true,
        "category": "scene_controller",
        "deviceTypeId": "134_3_25",
        "gatewayId": "gateway_id1",
        "name": "Panic Button",
        "parentDeviceId": "",
        "persistent": false,
        "reachable": true,
        "ready": true,
        "roomId": "",
        "security": "no",
        "status": "idle",
        "subcategory": "panic",
        "type": "panic.button",
        "armed": true
    }
    )"_json;

    const JsonObject item = R"(
    {
        "_id": "item_id1",
        "deviceId": "device_id1",
        "hasGetter": true,
        "hasSetter": true,
        "name": "security_threat",
        "show": true,
        "value": false,
        "valueFormatted": "false",
        "valueType": "bool"
    }
    )"_json;

    const JsonObject expectedBroadcast = R"(
    {
        "_id": "item_id1",
        "deviceCategory": "scene_controller",
        "deviceId": "device_id1",
        "deviceName": "Panic Button",
        "deviceSubcategory": "panic",
        "houseMode":{"alarmDelay":-1,"armed":false,"id":""},
        "deviceArmed": true,
        "name": "security_threat",
        "notifications": null,
        "roomName": "",
        "syncNotification": false,
        "userNotification": true,
        "protectService": "medical",
        "protectConfig":"",
        "protectNotification": true,
        "value": true
    }
    )"_json;

    const JsonObject protectServiceRecord = R"({"_id": "device_id1", "service": "medical"})"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getDBClient().insert( CoreDHouseModes::Resources::DBNames::HouseModesProtectButtonsDB, protectServiceRecord );

    getItemDbModifier().insert( item );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    bool isProtectOn = getFeatureManager().getFeatureStatus( "protect" );
    if( !isProtectOn ) {
        getFeatureManager().setFeatureStatus( "protect", true );
    }
    const bool protectOn = true;
    auto currentHouseModeId = CoreDHouseModes::getCurrentHouseModeId( getDBClient() );
    CoreDHouseModes::setProtect( getService(), getItemDbModifier(), currentHouseModeId, protectOn );

    JsonObject updatePortion;
    updatePortion[ ItemFields::ValueFieldName ] = true;

    getItemDbModifier().update( "item_id1", updatePortion, false );
    const Services::Message& message = getBroadcastMessage();

    EXPECT_EQ( message.msgSubClass, m_itemBroadcasts.updateOperation.classType );
    EXPECT_EQ( json_get_str( message.data, "protectService" ), "medical" )
        << "Should have specific protectService field value";
    checkBroadcastWithTimestamp( message.data, expectedBroadcast, __LINE__ );

    getFeatureManager().setFeatureStatus( "protect", isProtectOn );
}

TEST_F( ItemDbModifierTest, protectUpdateCheckHousModesItem ) {
    const JsonObject device = R"(
{
    "_id": "device_id1",
    "batteryPowered": true,
    "category": "scene_controller",
    "deviceTypeId": "134_3_25",
    "gatewayId": "gateway_id1",
    "name": "Panic Button",
    "parentDeviceId": "",
    "persistent": false,
    "reachable": true,
    "ready": true,
    "roomId": "",
    "security": "no",
    "status": "idle",
    "subcategory": "panic",
    "type": "panic.button",
    "armed": true
}
)"_json;

    const JsonObject item = R"(
{
    "_id": "item_id1",
    "deviceId": "device_id1",
    "hasGetter": true,
    "hasSetter": true,
    "name": "security_threat",
    "show": true,
    "value": false,
    "valueFormatted": "false",
    "valueType": "bool"
}
)"_json;

    const JsonObject expectedBroadcast = R"(
{
    "_id": "item_id1",
    "deviceCategory": "scene_controller",
    "deviceId": "device_id1",
    "deviceName": "Panic Button",
    "deviceSubcategory": "panic",
    "houseMode":{"alarmDelay":-1,"armed":false,"id":"1"},
    "deviceArmed": true,
    "name": "security_threat",
    "notifications": null,
    "roomName": "",
    "syncNotification": false,
    "userNotification": true,
    "protectService": "medical",
    "protectConfig":"",
    "protectNotification": true,
    "value": true
}
)"_json;

    const JsonObject protectServiceRecord = R"({"_id": "device_id1", "service": "medical"})"_json;

    getDBClient().insert( DBNames::DevicesDB, device );
    getDBClient().insert( CoreDHouseModes::Resources::DBNames::HouseModesProtectButtonsDB, protectServiceRecord );

    setEzloProtect( true );
    getItemDbModifier().insert( item );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    bool isProtectOn = getFeatureManager().getFeatureStatus( "protect" );
    if( !isProtectOn ) {
        getFeatureManager().setFeatureStatus( "protect", true );
    }
    const bool protectOn = true;
    auto currentHouseModeId = CoreDHouseModes::getCurrentHouseModeId( getDBClient() );
    CoreDHouseModes::setProtect( getService(), getItemDbModifier(), currentHouseModeId, protectOn );

    JsonObject updatePortion;
    updatePortion[ ItemFields::ValueFieldName ] = true;

    getItemDbModifier().update( "item_id1", updatePortion, false );
    const Services::Message& message = getBroadcastMessage();
    checkBroadcastWithTimestamp( message.data, expectedBroadcast, __LINE__ );
}

TEST_F( ItemDbModifierTest, userNotificationPowerState ) {
    constexpr char Heat[] = "heat";
    constexpr char HeatAlarm[] = "heat_alarm";
    constexpr char SecuritySensor[] = "security_sensor";

    auto checkShouldBeNotified = [this, &Heat, &SecuritySensor]( const std::string& state,
                                                                 const std::string& eventType,
                                                                 const std::string& message ) -> void {
        EXPECT_TRUE( shouldUserBeNotified( state, SecuritySensor, Heat, jsonValue( eventType ) ) ) << message;
    };

    checkShouldBeNotified( "power_surge_state", "surge_detected", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "voltage_drop_drift_state", "drop_drift_detected", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "over_current_state", "over_current_detected", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "over_voltage_state", "over_voltage_detected", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "over_load_state", "over_load_detected", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "load_error_state", "load_error", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "battery_maintenance_state", "replace_battery_soon", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "battery_maintenance_state", "replace_battery_now", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "battery_maintenance_state", "battery_fluid_is_low", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "battery_charging_state", "in_progress", MAKE_MESSAGE( "" ) );
    checkShouldBeNotified( "ac_state", "connected", MAKE_MESSAGE( "" ) );
}

TEST_F( ItemDbModifierTest, powerStateUpdateBroadcastPrepare ) {
    constexpr char Category[] = "state_sensor";
    constexpr char SubCategory[] = "power";

    const auto item = R"({
        "_id": "item_id1",
        "name": "power_surge_state",
        "deviceId": "device_id1",
        "value": false
    })"_json;

    const Json parentDevice = R"({
        "_id": "parent_device_id0",
		"category": "security_sensor",
		"subcategory": "motion",
        "name": "parent_device_name"
    })"_json;

    const Json device = R"({
        "_id": "device_id1",
        "parentDeviceId": "parent_device_id0",
        "name": "child device name",
		"armed": true,
		"category": "security_sensor",
		"subcategory": "motion",
        "roomId" : ""
    })"_json;

    const Json broadcastTemplate = R"({
        "_id": "item_id1",
        "deviceId": "device_id1",
        "parentDeviceId" : "parent_device_id0",
        "value": "surge_detected"
    })"_json;

    getDBClient().insert( DBNames::DevicesDB, parentDevice );
    getDBClient().insert( DBNames::DevicesDB, device );
    getItemDbModifier().insert( item );

    auto checkBroadcast = [this]( const std::string& itemName,
                                  const std::string& deviceCategory,
                                  const std::string& deviceSubcategory,
                                  Json broadcastData,
                                  const std::string& message = "" ) {
        auto broadcast = getItemDbModifier().prepareUpdateBroadcastMessage( broadcastData, JSON_EMPTY_OBJ );
        const bool expectUserNotification = true;
        EXPECT_EQ( json_get_bool( broadcast, "userNotification", false ), expectUserNotification )
            << "userNotification field should be true. At: " << message;
        EXPECT_FALSE( json_get_str( broadcastData, "parentDeviceName" ).empty() )
            << "Should have non-empty parent device name. At: " << message;
    };

    auto testPowerState = [&broadcastTemplate, item, &checkBroadcast, this, Category, SubCategory](
                              const std::string& name, const std::string& value, const std::string& message = "" ) {
        std::string itemId = json_get_str( item, "_id" );
        Json broadcastData = broadcastTemplate;
        Json changedItem = item;
        changedItem[ "name" ] = name;
        changedItem[ "value" ] = value;
        getItemDbModifier().update( itemId, changedItem );

        broadcastData[ "value" ] = value;
        checkBroadcast( name, Category, SubCategory, broadcastData, message.empty() ? MAKE_MESSAGE( "" ) : message );
    };

    testPowerState( "power_surge_state", "surge_detected", MAKE_MESSAGE( "" ) );
    testPowerState( "voltage_drop_drift_state", "drop_drift_detected", MAKE_MESSAGE( "" ) );
    testPowerState( "over_current_state", "over_current_detected", MAKE_MESSAGE( "" ) );
    testPowerState( "over_voltage_state", "over_voltage_detected", MAKE_MESSAGE( "" ) );
    testPowerState( "over_load_state", "over_load_detected", MAKE_MESSAGE( "" ) );
    testPowerState( "load_error_state", "load_error", MAKE_MESSAGE( "" ) );
    testPowerState( "battery_maintenance_state", "replace_battery_soon", MAKE_MESSAGE( "" ) );
    testPowerState( "battery_maintenance_state", "replace_battery_now", MAKE_MESSAGE( "" ) );
    testPowerState( "battery_maintenance_state", "battery_fluid_is_low", MAKE_MESSAGE( "" ) );
    testPowerState( "battery_charging_state", "in_progress", MAKE_MESSAGE( "" ) );
    testPowerState( "ac_state", "connected", MAKE_MESSAGE( "" ) );
}

TEST_F( ItemDbModifierTest, updateDictionaryProtectConfig ) {
    const JsonObject device = R"(
    {
        "_id": "device_id1",
		"armed": true,
        "protectConfig":"default"
    }
    )"_json;

    getDBClient().insert( DBNames::DevicesDB, device );

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

    const JsonObject broadcast = R"(
    {
        "_id": "id_1",
        "deviceCategory": "",
        "deviceId": "5df780747f000004b1eb356f",
        "deviceName": "",
        "deviceSubcategory": "",
        "element": {},
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "name": "user_codes",
        "notifications": null,
        "operation": "",
        "roomName": "",
        "userNotification": false,
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json;

    getItemDbModifier().insert( item );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    auto testUpdateDictionary = [this, &broadcast]( const JsonObject& updatedValue,
                                                    const JsonObject& oldValue,
                                                    const JsonObject& updatePortion,
                                                    const std::string& operation,
                                                    const std::string& dictionaryId,
                                                    int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        JsonObject expectedBroadcast = broadcast;
        expectedBroadcast[ "operation" ] = operation;
        expectedBroadcast[ "element" ][ dictionaryId ][ "value" ] = updatedValue;

        expectedBroadcast[ System::SyncNotification ] = json_contains( updatePortion, ItemFields::NameFieldName );

        if( operation == "updated" ) {
            expectedBroadcast[ DictionaryEditRequestFields::OldElementName ] =
                JSON( dictionaryId << JSON( "value" << oldValue ) );
        }

        getItemDbModifier().updateDictionary( "id_1", updatePortion, { dictionaryId, updatedValue, operation } );
        EXPECT_EQ( getBroadcastMessage().msgSubClass, HUB_ITEM_DICTIONARY_UPDATED ) << codeLineStr;

        LOGGER_ERROR( "getBroadcastMessage().data: " + cc::j( getBroadcastMessage().data ) );

        const auto result =
            getDBClient().findOne( TemporaryTableNamePreffix + m_dbName, JSON( db::ID_FIELD_NAME << "id_1" ), nullptr );
        JsonObject expectedItem = updatePortion;
        expectedItem[ "_id" ] = "id_1";
        EXPECT_EQ( result.dump(), expectedItem.dump() ) << codeLineStr;
    };

    {  // add new element
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

        testUpdateDictionary( newValue, JSON_EMPTY_OBJ, updatePortion, "added", "2", __LINE__ );
    }
}

}  // namespace UT
}  // namespace HubPlatform
