#include "cored/tests/fixtures/house_modes_processor_fixture.h"

#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "database_client/fake_db/fake_db_client.h"
#include "config_reader/tests/config_reader_factory.h"
#include "bus/fake_bus.h"
#include "cored/house_mode_proc.h"
#include "cored/core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/cored_device.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/tests/service/test_core_service.h"
#include "cored/db_modifier/db_modifier.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/db_modifier/room_db_modifier.h"

#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"
#include <thread>

/*

 TESTPLAN

 DONE   1.0 set house mode
 DONE       1.1 set house mode failed
 DONE       1.2 House Modes getters
 DONE           1.2.1 listHouseModes
 DONE           1.2.2 getHouseModes
 DONE       1.3 add notifications
 DONE       1.4 remove notifications
 DONE       1.5 add alarmOff device
 DONE       1.6 remove alarmOff device
 DONE       1.7 add security device and set armed field
 DONE           1.7.1 add device without security item but matching the house mode categories: should not be armed
 DONE       1.8 add disarmed device (device should match categories/subcategories AND  have security_threat item in it)
 DONE       1.9 remove disarmed device
 DONE       1.10 setHouseModeWithUpdateArmedDevices
 DONE       1.11 notificationsDefault
 DONE       1.12 handleSetProtect
 DONE       1.13 handleProtectButtonsSet_AddModifyRemove
 DONE       1.14 addCamerasOffDevice
 DONE       1.15 removeCamerasOffDevice
 DONE       1.16 checkThrow_addRemoveCamerasOffDevice
 DONE       1.17 checkProtectDevicesRequest
 DONE       1.18 switch From Armed to Disarmed house mode: siren turned off
 DONE       1.19 handleSetProtectWithTimestamp

 DONE   2. constructors
 DONE       2.1 constructor 1
 DONE       2.2 constructor 2

 DONE   3. AlarmDelay and Entry delay
 DONE       3.1 Alarm delay value parallel with entry delay value

DONE   4. Bypass signal in item updated broadcast

DONE    5. Device arming based on security config armedWhen field
DONE        5.1 On default security config
DONE        5.2 On security config id set
DONE        5.3 On houseMode switching

DONE    6. Security Event based on security config entry delay
DONE        6.1 Instant
DONE        6.2 Normal
DONE        6.3 default security config

DONE   7.  SoundType and Chime fields
DONE       7.1 police soundType and chime present
DONE       7.2 silent soundType and no chime
*/

namespace HubPlatform {
namespace UT {
using namespace CoreDHouseModes;
class HouseModesProcessorTest : public HouseModesProcessorFixture {};

class HouseModeEntryTest : public ::testing::Test {
public:
    static void checkHouseModeEntry( const std::string& id,
                                     const std::string& name,
                                     const std::string& description,
                                     bool disarmedDefault,
                                     bool ignoreDelay,
                                     bool protect,
                                     bool armed,
                                     CoreDHouseModes::HouseModeEntry::Notifications notificationUserIds,
                                     CoreDHouseModes::HouseModeEntry::Notifications notificationUserIdsDefault,
                                     int codeLine );

    static void checkHouseModeEntry( const std::string& id,
                                     const std::string& name,
                                     const std::string& description,
                                     bool disarmedDefault,
                                     bool protect,
                                     bool armed,
                                     CoreDHouseModes::HouseModeEntry::Notifications notificationUserIds,
                                     CoreDHouseModes::HouseModeEntry::Notifications notificationUserIdsDefault,
                                     const JsonArray& armedCategories,
                                     const JsonArray& alarmCategories,
                                     int codeLine );
};

void HouseModeEntryTest::checkHouseModeEntry( const std::string& id,
                                              const std::string& name,
                                              const std::string& description,
                                              bool disarmedDefault,
                                              bool ignoreDelay,
                                              bool protect,
                                              bool armed,
                                              CoreDHouseModes::HouseModeEntry::Notifications notificationUserIds,
                                              CoreDHouseModes::HouseModeEntry::Notifications notificationUserIdsDefault,
                                              int codeLine ) {
    std::string codeLineStr = "code line: " + std::to_string( codeLine );

    using namespace CoreDHouseModes::Resources;
    JsonObject notifications;
    if( disarmedDefault ) {
        CoreDHouseModes::HouseModeEntry::notificationsToJson( notifications, notificationUserIdsDefault );
    }
    else {
        CoreDHouseModes::HouseModeEntry::notificationsToJson( notifications, notificationUserIds );
    }
    JsonObject expected{ { HouseModeFields::IdFieldName, id },
                         { HouseModeFields::ModeNameFieldName, name },
                         { HouseModeFields::DescriptionFieldName, description },
                         { HouseModeFields::DisarmedDefaultFieldName, disarmedDefault },
                         { HouseModeFields::ProtectFieldName, protect },
                         { HouseModeFields::ArmedFieldName, armed },
                         { HouseModeFields::NotificationsFieldName, notifications },
                         { HouseModeFields::DisarmedDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::AlarmsOffDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::CamerasOffDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::BypassDevicesFieldName, JSON_EMPTY_ARR } };
    if( ignoreDelay ) {
        expected[ HouseModeFields::IgnoreSwitchDelay ] = true;
    }
    CoreDHouseModes::HouseModeEntry houseModeEntry( id,
                                                    name,
                                                    description,
                                                    disarmedDefault,
                                                    ignoreDelay,
                                                    protect,
                                                    armed,
                                                    std::move( notificationUserIds ),
                                                    std::move( notificationUserIdsDefault ),
                                                    {},
                                                    {} );
    EXPECT_EQ( houseModeEntry.toJson().dump(), expected.dump() ) << codeLineStr;
}

void HouseModeEntryTest::checkHouseModeEntry( const std::string& id,
                                              const std::string& name,
                                              const std::string& description,
                                              bool disarmedDefault,
                                              bool protect,
                                              bool armed,
                                              CoreDHouseModes::HouseModeEntry::Notifications notificationUserIds,
                                              CoreDHouseModes::HouseModeEntry::Notifications notificationUserIdsDefault,
                                              const JsonArray& armedCategories,
                                              const JsonArray& alarmCategories,
                                              int codeLine ) {
    std::string codeLineStr = "code line: " + std::to_string( codeLine );
    using namespace CoreDHouseModes;
    using namespace CoreDHouseModes::Resources;
    JsonObject notifications;
    if( disarmedDefault ) {
        CoreDHouseModes::HouseModeEntry::notificationsToJson( notifications, notificationUserIdsDefault );
    }
    else {
        CoreDHouseModes::HouseModeEntry::notificationsToJson( notifications, notificationUserIds );
    }
    JsonObject notificationsDefault;
    CoreDHouseModes::HouseModeEntry::notificationsToJson( notificationsDefault, notificationUserIdsDefault );
    JsonObject expected{ { HouseModeFields::IdFieldName, id },
                         { HouseModeFields::ModeNameFieldName, name },
                         { HouseModeFields::DescriptionFieldName, description },
                         { HouseModeFields::DisarmedDefaultFieldName, disarmedDefault },
                         { HouseModeFields::ProtectFieldName, protect },
                         { HouseModeFields::ArmedFieldName, armed },
                         { HouseModeFields::NotificationsFieldName, notifications },
                         { HouseModeFields::DisarmedDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::AlarmsOffDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::CamerasOffDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::BypassDevicesFieldName, JSON_EMPTY_ARR },
                         { HouseModeFields::ArmedCategoriesFieldName, armedCategories },
                         { HouseModeFields::AlarmCategoriesFieldName, alarmCategories },
                         { HouseModeFields::NotificationsDefaultFieldName, notificationsDefault } };
    CoreDHouseModes::HouseModeEntry houseModeEntry( expected );
    houseModeEntry.m_notificationUserIdsDefault = notificationUserIdsDefault;
    EXPECT_EQ( houseModeEntry.toJsonDb().dump(), expected.dump() ) << codeLineStr;
}

TEST_F( HouseModesProcessorTest, setHouseMode ) /* 1.0 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );
    const auto response = m_houseModeProcessor->handleSetHouseMode( "2" );
    EXPECT_EQ( response[ "error" ], JSON_NULL_VAL );
    EXPECT_EQ( response[ "result" ], JSON( "switchToDelay" << 0 ) );
    waitSwitchingHouseMode();
    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "2" );
}

TEST_F( HouseModesProcessorTest, setHouseModeFailed ) /* 1.1 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );
    const auto response = m_houseModeProcessor->handleSetHouseMode( "wrong" );
    EXPECT_EQ( response[ "error" ][ "code" ], -32602 );
    EXPECT_EQ( response[ "error" ][ "data" ], "rpc.params.invalid" );
    EXPECT_EQ( response[ "result" ], JSON_NULL_VAL );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );
}

TEST_F( HouseModesProcessorTest, listHouseModes ) /* 1.2.1 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initRemoteInterface();

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );
    const auto response = m_remoteInterface->getHouseModes();

    ASSERT_EQ( response.size(), 2 );

    auto& houseMode1 = response[ 0 ];
    EXPECT_EQ( houseMode1.m_id, "1" );
    EXPECT_EQ( houseMode1.m_name, "Name 1" );
    EXPECT_EQ( houseMode1.m_description, "Description 1" );
    EXPECT_EQ( houseMode1.m_disarmedDefault, false );
    EXPECT_EQ( houseMode1.m_protect, true );
    EXPECT_EQ( houseMode1.m_armed, false );

    auto& houseMode2 = response[ 1 ];
    EXPECT_EQ( houseMode2.m_id, "2" );
    EXPECT_EQ( houseMode2.m_name, "Name 2" );
    EXPECT_EQ( houseMode2.m_description, "Description 2" );
    EXPECT_EQ( houseMode2.m_disarmedDefault, true );
    EXPECT_EQ( houseMode2.m_protect, false );
    EXPECT_EQ( houseMode2.m_armed, true );
}

TEST_F( HouseModesProcessorTest, getHouseModes ) /* 1.2.2 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initRemoteInterface();

    {
        auto modePtr = m_remoteInterface->getHouseModeById( "1" );
        ASSERT_TRUE( modePtr.get() != nullptr );
        auto& houseMode1 = *modePtr;
        EXPECT_EQ( houseMode1.m_id, "1" );
        EXPECT_EQ( houseMode1.m_name, "Name 1" );
        EXPECT_EQ( houseMode1.m_description, "Description 1" );
        EXPECT_EQ( houseMode1.m_disarmedDefault, false );
        EXPECT_EQ( houseMode1.m_protect, true );
        EXPECT_EQ( houseMode1.m_armed, false );
    }

    {
        auto modePtr = m_remoteInterface->getHouseModeById( "2" );
        ASSERT_TRUE( modePtr.get() != nullptr );
        auto& houseMode2 = *modePtr;
        EXPECT_EQ( houseMode2.m_id, "2" );
        EXPECT_EQ( houseMode2.m_name, "Name 2" );
        EXPECT_EQ( houseMode2.m_description, "Description 2" );
        EXPECT_EQ( houseMode2.m_disarmedDefault, true );
        EXPECT_EQ( houseMode2.m_protect, false );
        EXPECT_EQ( houseMode2.m_armed, true );
    }
}

TEST_F( HouseModesProcessorTest, addNotifications ) /* 1.4 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1" } ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "user1" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1", "user2" } ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "user1", "user2" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", true, {} ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_notificationUserIds );
    }
}

TEST_F( HouseModesProcessorTest, removeNotifications ) /* 1.4 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", true, {} ) );
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1" } ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "user1" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1", "user2" } ) );
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1" } ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "user1" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1" } ) );
        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, {} ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_notificationUserIds->empty() );
    }
}

TEST_F( HouseModesProcessorTest, addAlarmsOffDevice ) /* 1.5 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::addAlarmsOffDevice( getDBClient(), "1", "device1" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1" };
        EXPECT_EQ( houseModePtr->m_alarmsOffDeviceIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::addAlarmsOffDevice( getDBClient(), "1", "device2" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1", "device2" };
        EXPECT_EQ( houseModePtr->m_alarmsOffDeviceIds, expected );
    }

    {
        EXPECT_FALSE( CoreDHouseModes::addAlarmsOffDevice( getDBClient(), "1", "device1" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1", "device2" };
        EXPECT_EQ( houseModePtr->m_alarmsOffDeviceIds, expected );
    }
}

TEST_F( HouseModesProcessorTest, removeAlarmsOffDevice ) /* 1.6 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::addAlarmsOffDevice( getDBClient(), "1", "device1" ) );
        EXPECT_TRUE( CoreDHouseModes::removeAlarmsOffDevice( getDBClient(), "1", "device1" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_alarmsOffDeviceIds.empty() );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::addAlarmsOffDevice( getDBClient(), "1", "device1" ) );
        EXPECT_FALSE( CoreDHouseModes::removeAlarmsOffDevice( getDBClient(), "1", "device2" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1" };
        EXPECT_EQ( houseModePtr->m_alarmsOffDeviceIds, expected );
    }
}

TEST_F( HouseModesProcessorTest, addSecurityDeviceAndSetArmedField ) /* 1.7 */ {
    {
        auto data = generateHouseModes();
        initService( data.dump() );
        initRemoteInterface();
    }

    {
        auto data = generateDevices();
        auto deviceIds = initDevices( data );
        auto itemsData = generateSecurityItem( deviceIds[ 0 ] );
        initItems( itemsData );
    }

    auto devices = m_remoteInterface->getDevices();
    Log::info( "dump device start 1 :" + devices[ 0 ].dump() );

    {
        const auto& device1 = devices[ 0 ];
        Log::info( "dump device1 :" + device1.dump() );
        EXPECT_EQ( json_get_str( device1, "name" ), "Device 1" );
        EXPECT_TRUE( json_get_bool( device1, "armed" ) );
    }
    {
        const auto& device2 = devices[ 1 ];
        EXPECT_EQ( json_get_str( device2, "name" ), "Device 2" );
        bool temp{ false };
        EXPECT_FALSE( json_get< bool >( device2, "armed", temp ) );
    }
}

TEST_F( HouseModesProcessorTest, addDeviceWithoutSecurityItemShouldNotBeArmed ) /* 1.7.1 */ {
    {
        auto data = generateHouseModes();
        initService( data.dump() );
        initRemoteInterface();
    }

    {
        auto data = generateDevices();
        initDevices( data );
    }

    auto devices = m_remoteInterface->getDevices();
    Log::info( "dump device start 1 :" + devices[ 0 ].dump() );

    {
        const auto& device1 = devices[ 0 ];
        Log::info( "dump device1 :" + device1.dump() );
        EXPECT_EQ( json_get_str( device1, "name" ), "Device 1" );
        EXPECT_EQ( json_get_bool( device1, "armed", false ), false ) << "should not be armed";
    }
    {
        const auto& device2 = devices[ 1 ];
        EXPECT_EQ( json_get_str( device2, "name" ), "Device 2" );
        bool temp{ false };
        EXPECT_EQ( json_get_bool( device2, "armed", false ), false ) << "should not be armed without security_threat";
    }
}

TEST_F( HouseModesProcessorTest, addDisarmedDevice ) /* 1.8 */ {
    {
        auto data = generateHouseModes();
        initService( data.dump() );
        initHouseModeProcessor( data.dump() );
        initRemoteInterface();
    }
    {
        auto data = generateDevices();
        auto deviceIds = initDevices( data );

        auto itemsData = generateSecurityItem( deviceIds[ 0 ] );
        initItems( itemsData );
        itemsData = generateSecurityItem( deviceIds[ 2 ] );
        initItems( itemsData );
    }

    auto devices = m_remoteInterface->getDevices();
    const auto& device1 = json_get_str( devices[ 0 ], "_id" );
    const auto& device3 = json_get_str( devices[ 2 ], "_id" );  //  add to disarmedDeviceIds when init

    const char* modeId = "1";
    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), modeId );
    {
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), modeId );
        std::set< std::string > expected{ device3 };
        EXPECT_EQ( houseModePtr->m_disarmedDeviceIds, expected ) << houseModePtr->toJson();
    }

    {
        EXPECT_TRUE( CoreDHouseModes::addDisarmedDevice( getBroadcaster(), getDevicesDbModifier(), modeId, device1 ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), modeId );
        std::set< std::string > expected{ device1, device3 };
        EXPECT_EQ( houseModePtr->m_disarmedDeviceIds, expected );
    }

    {
        EXPECT_FALSE( CoreDHouseModes::addDisarmedDevice( getBroadcaster(), getDevicesDbModifier(), modeId, device1 ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), modeId );
        std::set< std::string > expected{ device1, device3 };
        EXPECT_EQ( houseModePtr->m_disarmedDeviceIds, expected );
    }
}

TEST_F( HouseModesProcessorTest, removeDisarmedDevice ) /* 1.9 */ {
    {
        auto data = generateHouseModes();
        initService( data.dump() );
        initHouseModeProcessor( data.dump() );
        initRemoteInterface();
    }
    {
        auto data = generateDevices();
        auto deviceIds = initDevices( data );

        auto itemsData = generateSecurityItem( deviceIds[ 0 ] );
        initItems( itemsData );
        itemsData = generateSecurityItem( deviceIds[ 2 ] );
        initItems( itemsData );
    }

    auto devices = m_remoteInterface->getDevices();
    const auto& device1 = json_get_str( devices[ 0 ], "_id" );
    const auto& device3 = json_get_str( devices[ 2 ], "_id" );  //  add to disarmedDeviceIds when init

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::addDisarmedDevice( getBroadcaster(), getDevicesDbModifier(), "1", device1 ) );
        EXPECT_TRUE( CoreDHouseModes::removeDisarmedDevice( getBroadcaster(), getDevicesDbModifier(), "1", device1 ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ device3 };
        EXPECT_EQ( houseModePtr->m_disarmedDeviceIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::addDisarmedDevice( getBroadcaster(), getDevicesDbModifier(), "1", device1 ) );
        EXPECT_FALSE(
            CoreDHouseModes::removeDisarmedDevice( getBroadcaster(), getDevicesDbModifier(), "1", "device2" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ device1, device3 };
        EXPECT_EQ( houseModePtr->m_disarmedDeviceIds, expected );
    }
}

TEST_F( HouseModesProcessorTest, setHouseModeWithUpdateArmedDevices ) /* 1.10 */ {
    {
        auto data = generateHouseModes();
        initService( data.dump() );
        initHouseModeProcessor( data.dump() );
        initRemoteInterface();
    }

    {
        auto data = generateDevices();
        auto deviceIds = initDevices( data );

        auto itemsData = generateSecurityItem( deviceIds[ 0 ] );
        initItems( itemsData );
        itemsData = generateSecurityItem( deviceIds[ 2 ] );
        initItems( itemsData );
    }

    auto devices = m_remoteInterface->getDevices();

    {
        const auto& device1 = devices[ 0 ];
        EXPECT_EQ( json_get_str( device1, "name" ), "Device 1" );
        EXPECT_TRUE( json_get_bool( device1, "armed" ) );
    }
    {
        const auto& device3 = devices[ 2 ];
        EXPECT_EQ( json_get_str( device3, "name" ), "Device 3" );
        EXPECT_FALSE( json_get_bool( device3, "armed" ) );
    }

    const auto response = m_houseModeProcessor->handleSetHouseMode( "2" );
    waitSwitchingHouseMode();

    auto updateDevices = m_remoteInterface->getDevices();
    {
        const auto& device1 = updateDevices[ 0 ];
        EXPECT_EQ( json_get_str( device1, "name" ), "Device 1" );
        EXPECT_FALSE( json_get_bool( device1, "armed" ) );
    }
    {
        const auto& device3 = updateDevices[ 2 ];
        EXPECT_EQ( json_get_str( device3, "name" ), "Device 3" );
        EXPECT_TRUE( json_get_bool( device3, "armed" ) );
    }
}

TEST_F( HouseModesProcessorTest, notificationsDefault ) /* 1.11 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    {
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_notificationUserIds );
        EXPECT_FALSE( houseModePtr->m_disarmedDefault );

        CoreDHouseModes::setDisarmedDefault( getBroadcaster(), getDevicesDbModifier(), "1", true );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_notificationUserIds );
        EXPECT_TRUE( houseModePtr->m_disarmedDefault );

        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "1", false, { "user1" } ) );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected = { "user1" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
        EXPECT_FALSE( houseModePtr->m_disarmedDefault );

        CoreDHouseModes::setDisarmedDefault( getBroadcaster(), getDevicesDbModifier(), "1", true );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_notificationUserIds );
        EXPECT_TRUE( houseModePtr->m_disarmedDefault );
    }

    {
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "2" );
        std::set< std::string > expected{ "user1", "user2" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
        EXPECT_TRUE( houseModePtr->m_disarmedDefault );

        EXPECT_TRUE( CoreDHouseModes::setNotifications(
            getDBClient(), getBroadcaster(), getDevicesDbModifier(), "2", false, { "user1", "user2", "user3" } ) );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "2" );
        expected = { "user1", "user2", "user3" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
        EXPECT_FALSE( houseModePtr->m_disarmedDefault );

        CoreDHouseModes::setDisarmedDefault( getBroadcaster(), getDevicesDbModifier(), "2", true );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "2" );
        expected = { "user1", "user2" };
        EXPECT_EQ( *houseModePtr->m_notificationUserIds, expected );
        EXPECT_TRUE( houseModePtr->m_disarmedDefault );
    }
}

TEST_F( HouseModesProcessorTest, handleSetProtect ) /* 1.12 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );

    {
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_protect );

        CoreDHouseModes::setProtect( getBroadcaster(), getDevicesDbModifier(), "1", false );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_protect );

        auto response = m_houseModeProcessor->handleSetProtect( "1", true );
        EXPECT_TRUE( response.empty() );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_protect );

        response = m_houseModeProcessor->handleSetProtect( "1", false );
        EXPECT_TRUE( response.empty() );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_protect );
    }

    {
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "2" );
        EXPECT_FALSE( houseModePtr->m_protect );
    }
}

TEST_F( HouseModesProcessorTest, handleProtectButtonsSet_AddModifyRemove ) /* 1.13 */ {
    using namespace CoreDHouseModes;
    {
        auto data = generateHouseModes();
        initService( data.dump() );
        initHouseModeProcessor( data.dump() );
        initRemoteInterface();
    }
    {
        auto data = generateDevices();
        initDevices( data );
    }

    auto devices = m_remoteInterface->getDevices();
    const auto& device1 = json_get_str( devices[ 0 ], "_id" );

    auto protectService = ProtectServiceEnum::POLICE;

    auto checkBroadcast = []( const Services::Message& message,
                              MessageSubClasses subClass,
                              const std::string& errorMessage = std::string{} ) {
        EXPECT_EQ( message.group, GroupsNames::BROADCAST_GROUP ) << errorMessage;
        EXPECT_EQ( message.msgClass, EVENT ) << errorMessage;
        EXPECT_EQ( message.msgSubClass, subClass ) << errorMessage;
        LOGGER_TRACE_RAW( "Message.data: " + cc::j( message.data ) );
    };

    {
        m_houseModeProcessor->handleProtectButtonsSet( device1, ProtectServiceEnum::POLICE );
        auto protectButtons = getProtectButtons( getDBClient() );
        std::vector< ProtectButton > expected{ ProtectButton{ device1, protectService } };
        EXPECT_EQ( protectButtons, expected ) << "record should be created";

        checkBroadcast( getTestBus().getMessage(),
                        HOUSE_MODE_PROTECT_BUTTONS_ADDED,
                        MAKE_MESSAGE( "hub.modes.protect.buttons.added" ) );
    }
    // modify record, fire
    {
        protectService = ProtectServiceEnum::FIRE;
        m_houseModeProcessor->handleProtectButtonsSet( device1, protectService );

        auto protectButtons = getProtectButtons( getDBClient() );
        std::vector< ProtectButton > expected{ ProtectButton{ device1, protectService } };
        EXPECT_EQ( protectButtons, expected ) << "record should be changed";
        checkBroadcast( getTestBus().getMessage(),
                        HOUSE_MODE_PROTECT_BUTTONS_UPDATED,
                        MAKE_MESSAGE( "hub.modes.protect.buttons.updated" ) );
    }
    // modify record, medical
    {
        protectService = ProtectServiceEnum::MEDICAL;
        m_houseModeProcessor->handleProtectButtonsSet( device1, protectService );

        auto protectButtons = getProtectButtons( getDBClient() );
        std::vector< ProtectButton > expected{ ProtectButton{ device1, protectService } };
        EXPECT_EQ( protectButtons, expected ) << "record should be changed";
        checkBroadcast( getTestBus().getMessage(),
                        HOUSE_MODE_PROTECT_BUTTONS_UPDATED,
                        MAKE_MESSAGE( "hub.modes.protect.buttons.updated" ) );
    }
    // remove record
    {
        protectService = ProtectServiceEnum::NONE;
        m_houseModeProcessor->handleProtectButtonsSet( device1, protectService );

        auto protectButtons = getProtectButtons( getDBClient() );
        std::vector< ProtectButton > expected{};
        EXPECT_EQ( protectButtons, expected ) << "record should be removed";
        checkBroadcast( getTestBus().getMessage(),
                        HOUSE_MODE_PROTECT_BUTTONS_REMOVED,
                        MAKE_MESSAGE( "hub.modes.protect.buttons.removed" ) );
    }

    {
        protectService = ProtectServiceEnum::MEDICAL;
        auto joResult = m_houseModeProcessor->handleProtectButtonsSet( "device-does-not-exist", protectService );
        LOGGER_TRACE_RAW( "handleProtectButtonsSet: returned result: " + cc::j( joResult ) );
        auto error = joResult[ Errors::ERROR_FIELD ];

        LOGGER_TRACE_RAW( "error:" + cc::j( error ) );
        EXPECT_EQ( error[ "code" ], -32500 );  // APPLICATION_ERROR
        EXPECT_EQ( error[ "message" ], "No device found!" );
        EXPECT_EQ( error[ "data" ], "ezlo.device.notfound" );
    }
}

TEST_F( HouseModesProcessorTest, addCamerasOffDevice ) /* 1.14 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "1", "device1" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1" };
        EXPECT_EQ( houseModePtr->m_camerasOffDeviceIds, expected );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "1", "device2" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1", "device2" };
        EXPECT_EQ( houseModePtr->m_camerasOffDeviceIds, expected );
    }

    {
        EXPECT_FALSE( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "1", "device1" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1", "device2" };
        EXPECT_EQ( houseModePtr->m_camerasOffDeviceIds, expected );
    }
}

TEST_F( HouseModesProcessorTest, removeCamerasOffDevice ) /* 1.15 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_TRUE( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "1", "device1" ) );
        EXPECT_TRUE( CoreDHouseModes::removeCamerasOffDevice( getDBClient(), "1", "device1" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_camerasOffDeviceIds.empty() );
    }

    {
        EXPECT_TRUE( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "1", "device1" ) );
        EXPECT_FALSE( CoreDHouseModes::removeCamerasOffDevice( getDBClient(), "1", "device2" ) );
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        std::set< std::string > expected{ "device1" };
        EXPECT_EQ( houseModePtr->m_camerasOffDeviceIds, expected );
    }
}

TEST_F( HouseModesProcessorTest, checkThrow_addRemoveCamerasOffDevice ) /* 1.16 */ {
    auto data = generateHouseModes();
    initService( data.dump() );

    EXPECT_EQ( CoreDHouseModes::getCurrentHouseModeId( getDBClient() ), "1" );

    {
        EXPECT_THROW( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "", "device1" ), std::runtime_error );
        EXPECT_THROW( CoreDHouseModes::addCamerasOffDevice( getDBClient(), "1", "" ), std::runtime_error );
        EXPECT_THROW( CoreDHouseModes::removeCamerasOffDevice( getDBClient(), "", "device1" ), std::runtime_error );
        EXPECT_THROW( CoreDHouseModes::removeCamerasOffDevice( getDBClient(), "1", "" ), std::runtime_error );
    }
}

TEST_F( HouseModesProcessorTest, checkProtectDevicesRequest ) /* 1.17 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initRemoteInterface();
    initHouseModeProcessor( data.dump() );
    initDevices( generateDevices() );
    auto rawDevices = m_remoteInterface->getDevices();

    struct ProtectDevicesCheckResult {
        bool isGood;
        std::vector< std::string > deviceIds;
        Json errorResult;
    };

    auto checkError = []( const ProtectDevicesCheckResult& result,
                          const ProtectDevicesCheckResult& expected,
                          const std::string& message ) {
        LOGGER_TRACE_RAW( "checkError: isGood: " + cc::flag( result.isGood ) );
        LOGGER_TRACE_RAW( "checkError: deviceIds: " + cc::j( result.deviceIds ) );
        LOGGER_TRACE_RAW( "checkError: errorResult: " + cc::j( result.errorResult ) );

        ASSERT_EQ( result.isGood, expected.isGood ) << message;
        if( result.isGood ) {
            ASSERT_EQ( result.deviceIds, expected.deviceIds ) << message;
        }
        else {
            ASSERT_EQ( result.errorResult, expected.errorResult ) << message;
        };
    };

    {
        ProtectDevicesCheckResult checkResult;
        std::tie( checkResult.isGood, checkResult.deviceIds, checkResult.errorResult ) =
            CoreDHouseModes::readDeviceIdsParams( JSON_EMPTY_OBJ );

        Json expectedError =
            R"({"error":{
                    "code":-32600,
                    "data":"rpc.params.notfound.deviceIds",
                    "message":"Bad request, deviceIds does not exist"}})"_json;
        ProtectDevicesCheckResult expected = { false, {}, expectedError };
        checkError( checkResult, expected, MAKE_MESSAGE( "deviceIds does not exist" ) );
    }

    {
        ProtectDevicesCheckResult checkResult;
        std::tie( checkResult.isGood, checkResult.deviceIds, checkResult.errorResult ) =
            CoreDHouseModes::readDeviceIdsParams( JSON( "deviceIds" << 111 ) );

        Json expectedError =
            R"({"error":{
                "code":-32602,
                "data":"rpc.params.invalid.deviceIds",
                "message":"Wrong params, deviceIds",
                "reason":"Format of a required field is wrong"}})"_json;
        ProtectDevicesCheckResult expected = { false, {}, expectedError };
        checkError( checkResult, expected, MAKE_MESSAGE( "deviceIds is not an array of strings" ) );
    }

    {
        ProtectDevicesCheckResult checkResult;
        std::tie( checkResult.isGood, checkResult.deviceIds, checkResult.errorResult ) =
            CoreDHouseModes::readDeviceIdsParams( JSON( "deviceIds" << JSON_ARRAY_( "AAA" ) ) );

        ProtectDevicesCheckResult expected = { true, { "AAA" }, {} };
        checkError( checkResult, expected, MAKE_MESSAGE( "No device with this ID" ) );
    }

    {
        std::vector< std::string > deviceIds;
        for_each( begin( rawDevices ), end( rawDevices ), [&deviceIds]( const Json& device ) {
            deviceIds.push_back( json_get_str( device, "_id" ) );
        } );
        EXPECT_FALSE( deviceIds.empty() ) << "Should have devices set up";

        ProtectDevicesCheckResult checkResult;
        std::tie( checkResult.isGood, checkResult.deviceIds, checkResult.errorResult ) =
            CoreDHouseModes::readDeviceIdsParams( JSON( "deviceIds" << deviceIds ) );

        ProtectDevicesCheckResult expected = { true, deviceIds, {} };
        checkError( checkResult, expected, MAKE_MESSAGE( "Devices lists should match" ) );
    }

    {
        ProtectDevicesCheckResult checkResult;
        std::tie( checkResult.isGood, checkResult.deviceIds, checkResult.errorResult ) =
            CoreDHouseModes::readDeviceIdsParams( JSON( "deviceIds" << JSON_ARRAY_( "AAA", "" ) ) );

        Json expectedError =
            R"({"error":{
                "code":-32602,
                "data":"rpc.params.invalid.deviceIds",
                "message":"Wrong params, deviceIds",
                "reason":"Format of a required field is wrong"}})"_json;
        ProtectDevicesCheckResult expected = { false, {}, expectedError };
        checkError( checkResult, expected, MAKE_MESSAGE( "Empty string in deviceIds parameter" ) );
    }
}

TEST_F( HouseModesProcessorTest, switchFromArmedToDisarmed_sirenTurnedOff ) /* 1.18 */ {
    using namespace CoreD::Model::Resources;
    auto data = generateHouseModes();
    initService( data.dump() );
    initRemoteInterface();
    initHouseModeProcessor( data.dump() );

    const std::string builtInGatewaySetItemValueCommand = "cored:set_item_value";
    const std::string commandScript{ builtInGatewaySetItemValueCommand };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );
    auto devicesData = generateDevices( gatewayId );

    auto deviceIds = initDevices( devicesData );
    Json sirenSwitchItem = generateSwitchItem( deviceIds[ 2 ] );
    std::string sirenItemId = json_get_str( sirenSwitchItem, "id" );

    auto devices = m_remoteInterface->getDevices();
    Log::info( "dump device start 1 :" + devices[ 0 ].dump() );
    Log::info( "dump device start 3 :" + devices[ 2 ].dump() );

    {
        const auto& device1 = devices[ 0 ];
        Log::info( "dump device1 :" + device1.dump() );
        EXPECT_EQ( json_get_str( device1, "name" ), "Device 1" );
        EXPECT_FALSE( json_get_bool( device1, "armed" ) );
    }
    {
        const auto& device2 = devices[ 1 ];
        EXPECT_EQ( json_get_str( device2, "name" ), "Device 2" );
        bool temp{ false };
        EXPECT_FALSE( json_get< bool >( device2, "armed", temp ) );
    }
    {
        const auto& sirenDevice = devices[ 2 ];
        EXPECT_EQ( json_get_str( sirenDevice, "name" ), "Device 3" );
        bool temp{ false };
        EXPECT_FALSE( json_get< bool >( sirenDevice, "armed", temp ) );
    }

    auto itemBefore = m_remoteInterface->getItem( sirenItemId );

    // switch mode to 2
    m_houseModeProcessor->handleSetHouseMode( "2" );
    waitSwitchingHouseMode();
    const auto message = getTestBus().getMessage();

    setItemValue( JSON( "_id" << sirenItemId << "value" << true ), "" );

    {  // item.updated
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        const auto message = getTestBus().getMessage();
        const auto sirenIsOn = true;
        ASSERT_EQ( json_get_bool( message.data, "value", false ), sirenIsOn );
    }

    // switch mode to 1
    m_houseModeProcessor->handleSetHouseMode( "1" );
    {  // mode switched
        const auto message = getTestBus().getMessage();
    }
    {  // item.updated
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        const auto message = getTestBus().getMessage();
        const auto sirenIsOff = false;
        ASSERT_EQ( json_get_bool( message.data, "value", false ), sirenIsOff );
    }
}

TEST_F( HouseModesProcessorTest, handleSetProtectWithTimestamp ) /* 1.19 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    auto timestamp = std::chrono::milliseconds( 200 );
    auto updatedTimestamp = std::chrono::milliseconds( 400 );

    {
        auto houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_protect );

        CoreDHouseModes::setProtect( getBroadcaster(), getDevicesDbModifier(), "1", false );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_FALSE( houseModePtr->m_protect );
        CoreDHouseModes::setProtectTimestamp( getDevicesDbModifier(), "1", timestamp );
        using namespace CoreDHouseModes::Resources;
        auto jsonTimestampData =
            getDBClient().findOne( HaTempHouseModesCommandTimestampsDBName,
                                   JSON( HouseModeCommandFields::CommandFieldName
                                         << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1" ),
                                   nullptr );
        auto timestampValue = json_get_int64( jsonTimestampData, "timestamp" );
        EXPECT_EQ( timestampValue, timestamp.count() );
        auto response = m_houseModeProcessor->handleSetProtectWithTimestamp( "1", true, updatedTimestamp );
        EXPECT_TRUE( response.empty() );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_protect );
        jsonTimestampData =
            getDBClient().findOne( HaTempHouseModesCommandTimestampsDBName,
                                   JSON( HouseModeCommandFields::CommandFieldName
                                         << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << "1" ),
                                   nullptr );
        timestampValue = json_get_int64( jsonTimestampData, "timestamp" );
        EXPECT_EQ( timestampValue, updatedTimestamp.count() );
        response = m_houseModeProcessor->handleSetProtectWithTimestamp( "1", false, timestamp );
        EXPECT_FALSE( response.empty() );
        EXPECT_EQ( response[ "error" ][ "message" ], "Command is outdated" );
        houseModePtr = CoreDHouseModes::getHouseMode( getDBClient(), "1" );
        EXPECT_TRUE( houseModePtr->m_protect );
    }
}

TEST_F( HouseModeEntryTest, constructor1 ) /* 2.1 */ {
    using namespace CoreDHouseModes;
    std::string id = "id";
    std::string name = "name";
    std::string description = "description";
    CoreDHouseModes::HouseModeEntry::Notifications notificationUserIds;
    CoreDHouseModes::HouseModeEntry::Notifications notificationUserIdsDefault;
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ true,
                         /*ignoreDelay*/ true,
                         /*protect*/ false,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         __LINE__ );

    notificationUserIds = std::make_unique< std::set< std::string > >();
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ false,
                         /*ignoreDelay*/ false,
                         /*protect*/ true,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         __LINE__ );

    std::set< std::string > notifications = { "user1", "user2" };
    notificationUserIds = std::make_unique< std::set< std::string > >( notifications );
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ false,
                         /*ignoreDelay*/ false,
                         /*protect*/ false,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         __LINE__ );

    notificationUserIds = std::make_unique< std::set< std::string > >( notifications );
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ true,
                         /*ignoreDelay*/ false,
                         /*protect*/ true,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         __LINE__ );
}

TEST_F( HouseModeEntryTest, constructor2 ) /* 2.2 */ {
    using namespace CoreDHouseModes;
    using namespace CoreDHouseModes::Resources;
    std::string id = "id";
    std::string name = "name";
    std::string description = "description";
    CoreDHouseModes::HouseModeEntry::Notifications notificationUserIds;
    CoreDHouseModes::HouseModeEntry::Notifications notificationUserIdsDefault;
    JsonArray armedCategories =
        R"( [ { "category" : "security_sensor", "subcategories" : [ "co", "glass", "smoke" ] }, { "category" : "security_sensor1", "subcategories" : [ "door", "leak", "motion" ] } ] )"_json;
    JsonArray alarmCategories = R"( [ { "category" : "siren1" } ] )"_json;
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ true,
                         /*protect*/ false,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         JSON_EMPTY_ARR,
                         nullptr,
                         __LINE__ );

    notificationUserIds = std::make_unique< std::set< std::string > >();
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ true,
                         /*protect*/ true,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         armedCategories,
                         alarmCategories,
                         __LINE__ );

    notificationUserIds = std::make_unique< std::set< std::string > >();
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ false,
                         /*protect*/ false,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         armedCategories,
                         alarmCategories,
                         __LINE__ );

    std::set< std::string > notifications = { "user1", "user2" };
    notificationUserIds = std::make_unique< std::set< std::string > >( notifications );
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ true,
                         /*protect*/ true,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         armedCategories,
                         alarmCategories,
                         __LINE__ );

    notificationUserIds = std::make_unique< std::set< std::string > >( notifications );
    checkHouseModeEntry( id,
                         name,
                         description,
                         /*disarmedDefault*/ false,
                         /*protect*/ false,
                         /*armed*/ true,
                         std::move( notificationUserIds ),
                         notificationUserIdsDefault,
                         armedCategories,
                         alarmCategories,
                         __LINE__ );
}

TEST_F( HouseModesProcessorTest, alarmDelayParallelWithEntryDelayNormal ) /* 3.1 */ {
    auto data = generateHouseModes();
    initService( data.dump() );
    auto alarmDelay = CoreDHouseModes::getAlarmDelay( getDBClient() );
    auto entryDelay = CoreDHouseModes::getEntryDelay( getDBClient() );
    EXPECT_EQ( entryDelay[ "normal" ], alarmDelay );

    auto delayValue = 20;
    CoreDHouseModes::setAlarmDelay( getDBClient(), delayValue );
    alarmDelay = CoreDHouseModes::getAlarmDelay( getDBClient() );
    entryDelay = CoreDHouseModes::getEntryDelay( getDBClient() );
    EXPECT_EQ( entryDelay[ "normal" ], alarmDelay );
    EXPECT_EQ( entryDelay[ "normal" ], delayValue );
    EXPECT_EQ( alarmDelay, delayValue );
}

TEST_F( HouseModesProcessorTest, itemUpdateSecurityConfigBypassSignal ) {  // 4

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "ring",
        "signal": {
            "bypass": {
                "send": true,
                "event": { "code": "570", "alias": "on" }
            }
        }
    }
    )"_json;

    const JsonObject contactId = R"(
    {
        "_id": "570",
        "text": "570 Bypass",
        "on": "EVENT_BYPASS",
        "off": "EVENT_BYPASS_OFF"
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );
    getDBClient().insert( DBNames::ContactIdsDB, contactId );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1( gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "0" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    addBypassDevices( getCoreDService(), "1", deviceIds );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );
    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_BYPASS_DEVICES_ADDED );

    constexpr bool value = true;
    const Json jsonValue = value;

    setItemValue( JSON( "_id" << itemId[ 0 ][ "id" ] << "value" << jsonValue ), "" );

    message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
    ASSERT_TRUE( json_contains( message.data, "protect" ) );
    ASSERT_FALSE( json_contains( message.data, "protectNotification" ) );
    ASSERT_FALSE( json_contains( message.data, "houseMode" ) );
    JsonObject protect;
    json_get_obj( message.data, "protect", protect );
    ASSERT_TRUE( json_contains( protect, "signal" ) );
    EXPECT_EQ( json_get_str( protect, "signal" ), "EVENT_BYPASS" );

    removeAllBypassDevices( getCoreDService(), "1" );
}

TEST_F( HouseModesProcessorTest, securityConfigDefault ) {  // 5.1

    auto data = generateHouseModes();
    initService( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );
    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1(
        gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "default" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    auto devicesArray = m_remoteInterface->getDevices();
    EXPECT_EQ( json_get_str( devicesArray[ 0 ], "_id" ), deviceIds[ 0 ] );
    EXPECT_EQ( json_get_bool( devicesArray[ 0 ], "armed" ), false );
}

TEST_F( HouseModesProcessorTest, securityConfigId ) {  // 5.2

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "ring",
        "armedWhen": [
            "Disarm",
            "Away"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1( gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "0" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    auto devicesArray = m_remoteInterface->getDevices();
    EXPECT_EQ( json_get_str( devicesArray[ 0 ], "_id" ), deviceIds[ 0 ] );
    EXPECT_EQ( json_get_bool( devicesArray[ 0 ], "armed" ), true );
}

TEST_F( HouseModesProcessorTest, switchHouseMode ) {  // 5.3

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "ring",
        "armedWhen": [
            "Away",
            "Stay"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1( gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "0" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    auto devicesArray = m_remoteInterface->getDevices();
    EXPECT_EQ( json_get_str( devicesArray[ 0 ], "_id" ), deviceIds[ 0 ] );
    EXPECT_EQ( json_get_bool( devicesArray[ 0 ], "armed" ), false );

    {
        const auto response = m_houseModeProcessor->handleSetHouseMode( "2" );  // setting to Away
        waitSwitchingHouseMode();

        EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "2" );
        devicesArray = m_remoteInterface->getDevices();
        EXPECT_EQ( json_get_str( devicesArray[ 0 ], "_id" ), deviceIds[ 0 ] );
        EXPECT_EQ( json_get_bool( devicesArray[ 0 ], "armed" ), true );
    }

    {
        const auto response = m_houseModeProcessor->handleSetHouseMode( "1" );  // setting to Home
        waitSwitchingHouseMode();

        EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );
        devicesArray = m_remoteInterface->getDevices();
        EXPECT_EQ( json_get_str( devicesArray[ 0 ], "_id" ), deviceIds[ 0 ] );
        EXPECT_EQ( json_get_bool( devicesArray[ 0 ], "armed" ), false );
    }
}

TEST_F( HouseModesProcessorTest, securityConfigEntryDelayInstant ) {  // 6.1

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "entryDelay": "instant",
        "armedWhen": [
            "Disarm",
            "Stay"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );
    m_remoteInterface->setAlarmDelay( 30 );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1( gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "0" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    setItemValue( JSON( "_id" << itemId[ 0 ][ "id" ] << "value" << true ), "" );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    EXPECT_EQ( json_get_str( message.data, "phase" ), "main" );
    EXPECT_EQ( json_get_str( message.data, "status" ), "begin" );

    message = getTestBus().getMessage();

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
}

TEST_F( HouseModesProcessorTest, securityConfigEntryDelayNormal ) {  // 6.2

    const JsonObject securityConfig = R"(
    {
        "_id": "0",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "entryDelay": "normal",
        "armedWhen": [
            "Disarm",
            "Stay"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1( gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "0" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    setItemValue( JSON( "_id" << itemId[ 0 ][ "id" ] << "value" << true ), "" );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    EXPECT_EQ( json_get_str( message.data, "phase" ), "entryDelay" );
    EXPECT_EQ( json_get_str( message.data, "status" ), "begin" );

    message = getTestBus().getMessage();

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
}

TEST_F( HouseModesProcessorTest, defaultSecurityConfigEntryDelay ) {  // 6.3

    const JsonObject securityConfig = R"(
    {
        "_id": "default",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "entryDelay": "instant",
        "armedWhen": [
            "Disarm",
            "Stay"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );
    m_remoteInterface->setAlarmDelay( 45 );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1(
        gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "default" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    setItemValue( JSON( "_id" << itemId[ 0 ][ "id" ] << "value" << true ), "" );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    EXPECT_EQ( json_get_str( message.data, "phase" ), "entryDelay" );
    EXPECT_EQ( json_get_str( message.data, "status" ), "begin" );

    message = getTestBus().getMessage();

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
}

TEST_F( HouseModesProcessorTest, soundTypeAndChimeFields ) {  // 7.1

    const JsonObject securityConfig = R"(
    {
        "_id": "default",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "police",
        "chime": "yes",
        "entryDelay": "normal",
        "armedWhen": [
            "Disarm",
            "Stay"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1(
        gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "default" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    setItemValue( JSON( "_id" << itemId[ 0 ][ "id" ] << "value" << true ), "" );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    EXPECT_EQ( json_get_str( message.data, "phase" ), "entryDelay" );
    EXPECT_EQ( json_get_str( message.data, "status" ), "begin" );
    EXPECT_EQ( json_get_str( message.data, "soundType" ), "police" );
    EXPECT_EQ( json_get_bool( message.data, "chime" ), true );
    EXPECT_EQ( json_get_bool( message.data, "silent" ), false );

    message = getTestBus().getMessage();

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
}

TEST_F( HouseModesProcessorTest, silentSoundTypeAndNoChime ) {  // 7.2

    const JsonObject securityConfig = R"(
    {
        "_id": "default",
        "name": "[0] Fixed Panic",
        "alarm": true,
        "soundType": "silent",
        "chime": "none",
        "entryDelay": "normal",
        "armedWhen": [
            "Disarm",
            "Stay"
        ]
    }
    )"_json;

    auto data = generateHouseModes();
    initService( data.dump() );
    initHouseModeProcessor( data.dump() );
    initRemoteInterface();
    using namespace CoreD::Model::Resources;
    getDBClient().insert( DBNames::FeaturesDB,
                          JSON( "_id"
                                << "protect"
                                << "status" << true ) );
    getDBClient().insert( DBNames::SecurityConfigsDB, securityConfig );

    EXPECT_EQ( m_remoteInterface->getCurrentHouseModeId(), "1" );

    const std::string commandScript{ CoreD::Model::Resources::GatewayBuiltInScripts::SetItemValueScript };
    const auto gatewayId = ensureGateway( JSON( GatewayFields::SetItemValueCommandFieldName << commandScript ) );

    CoreD::Model::Device device1(
        gatewayId, "Test Device 1", "test_device", "139_21586_21570", "sensor", false, "default" );
    device1.setSubcategory( "motion" );
    auto deviceIds = initDevices( { device1 } );

    const auto itemId = generateSecurityItem( deviceIds[ 0 ] );

    setItemValue( JSON( "_id" << itemId[ 0 ][ "id" ] << "value" << true ), "" );

    auto message = getTestBus().getMessage();
    LOGGER_ERROR( "message.data: " + cc::j( message.data ) );

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HOUSE_MODE_ALARMED );

    EXPECT_EQ( json_get_str( message.data, "phase" ), "entryDelay" );
    EXPECT_EQ( json_get_str( message.data, "status" ), "begin" );
    EXPECT_EQ( json_get_str( message.data, "soundType" ), "silent" );
    EXPECT_EQ( json_get_bool( message.data, "chime" ), false );
    EXPECT_EQ( json_get_bool( message.data, "silent" ), true );

    message = getTestBus().getMessage();

    EXPECT_EQ( message.msgClass, EVENT );
    EXPECT_EQ( message.msgSubClass, HUB_ITEM_UPDATED );
}

}  // namespace UT
}  // namespace HubPlatform
