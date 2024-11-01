#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1 modify_device() for disarmed devices
DONE        1.1 modify_device: add a device as disarmed to two more house modes.
DONE        1.2 modify_device: remove a device as disarmed from two out of four house modes.
DONE        1.3 modify_device, no house_modes_options: armed state should not be changed
DONE    2 modify_device() for sirens
DONE        2.1 modify_device: add a device as alarmsOff to two more house modes.
DONE        2.2 modify_device: remove a device as alarmsOff from two out of four house modes.
DONE    3 Panic button support
DONE        3.1 add panic button to armed devices
DONE        3.2 add button then modify to panic button and back
DONE    4 modifyItem(item[name = 'security_threat'])
DONE        4.1 rename "security_threat" item to something else
DONE        4.2 rename some item to "security_threat"

*/

namespace HouseMode {
const std::string Home = "1";
const std::string Away = "2";
const std::string Night = "3";
const std::string Vacation = "4";
}  // namespace HouseMode

class ModifyDeviceHouseModesTests : public CoreNmaTest {
public:
    ModifyDeviceHouseModesTests()
    : m_remote( nma() ) {}

    void SetUp() override;

    void TearDown() override;

    using ExpectedDevices = std::set< std::string >;

    void checkHouseModesPresetsDisarmed( const JsonArray& modes,
                                         const ExpectedDevices& expectedDevices,
                                         bool disarmedDefault = true,
                                         const std::string& message = "" );
    void checkHouseModesPresetsAlarmsOff( const JsonArray& modes,
                                          const ExpectedDevices& expectedDevices,
                                          bool disarmedDefault = true,
                                          const std::string& message = "" );

    static JsonObject callHouseModesGet( const std::string& errorMessage );
    static JsonObject fetchResultDevices( const JsonObject& answer, const std::string& errorMessage );
    void createSecurityThreatItem( std::string& itemId,
                                   const std::string& deviceId,
                                   bool armedFlag,
                                   const std::string& message );

protected:
    CoreDRemoteInterface::CoreDRemoteInterface m_remote;
    std::string m_gatewayId;
    std::string m_houseModeId;
};

void ModifyDeviceHouseModesTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
    clearExistingDevices( __LINE__ );

    JsonObject broadcast;
    m_gatewayId = getMainGateway( m_remote, __LINE__ );

    m_houseModeId = m_remote.getCurrentHouseModeId();
    if( m_houseModeId != HouseMode::Home ) {
        m_remote.setHouseMode( HouseMode::Home );
        broadcast = waitForBroadcast( "hub.modes.switched", getCodeLineString( __LINE__ ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.switched" );
    }

    auto houseMode = m_remote.getHouseModeById( HouseMode::Home );
    if( houseMode && houseMode->m_disarmedDefault != true ) {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.disarmed_default.set" },
              { "params", JSON( "modeId" << HouseMode::Home << "disarmedDefault" << true ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );

        broadcast = waitForBroadcast( "hub.modes.changed", getCodeLineString( __LINE__ ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
    }

    m_houseModeId = HouseMode::Home;
}

void ModifyDeviceHouseModesTests::TearDown() {
    clearExistingDevices( __LINE__ );
    CoreNmaTest::TearDown();
}

void ModifyDeviceHouseModesTests::checkHouseModesPresetsDisarmed( const JsonArray& modes,
                                                                  const ExpectedDevices& expectedDevices,
                                                                  bool disarmedDefault,
                                                                  const std::string& message ) {
    for( const auto& modeIt : modes ) {
        auto houseMode = m_remote.getHouseModeById( modeIt.get< std::string >() );
        LOGGER_TRACE( houseMode->toJson().dump() );
        EXPECT_EQ( houseMode->m_disarmedDefault, disarmedDefault ) << message;
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expectedDevices ) << message;
    }
}

void ModifyDeviceHouseModesTests::checkHouseModesPresetsAlarmsOff( const JsonArray& modes,
                                                                   const ExpectedDevices& expectedDevices,
                                                                   bool disarmedDefault,
                                                                   const std::string& message ) {
    for( const auto& modeIt : modes ) {
        auto houseMode = m_remote.getHouseModeById( modeIt.get< std::string >() );
        LOGGER_TRACE( houseMode->toJson().dump() );
        EXPECT_EQ( houseMode->m_disarmedDefault, disarmedDefault ) << message;
        EXPECT_EQ( houseMode->m_alarmsOffDeviceIds, expectedDevices ) << message;
    }
}

JsonObject ModifyDeviceHouseModesTests::callHouseModesGet( const std::string& errorMessage ) {
    auto answer = HubPlatform::Tests::CommonNmaTests::nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ } } );
    return answer;
}

JsonObject ModifyDeviceHouseModesTests::fetchResultDevices( const JsonObject& answer,
                                                            const std::string& errorMessage ) {
    JsonObject result;
    JsonArray devices;
    if( !json_get_obj( answer, "result", result ) )
        ADD_FAILURE() << errorMessage;
    if( !json_get_arr( result, "devices", devices ) )
        ADD_FAILURE() << errorMessage;

    return devices;
}

void ModifyDeviceHouseModesTests::createSecurityThreatItem( std::string& itemId,
                                                            const std::string& deviceId,
                                                            bool armedFlag,
                                                            const std::string& message ) {
    using namespace CoreD::Model::Resources;
    using namespace CoreDHouseModes::Resources;
    bool isGood;
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item item( deviceId,
                             Items::SecurityThreat,
                             valueType,
                             true,
                             true,
                             CoreD::Model::ValueFactory::createDefaultItemValue( valueType ),
                             "" );

    std::tie( itemId, isGood ) = m_remote.addItem( item );
    EXPECT_TRUE( isGood ) << "Item creation error";
    checkBroadcast( itemId, "hub.item.added", {}, message );
    checkBroadcast( deviceId, "hub.device.updated", { { DeviceFields::ArmedFieldName, armedFlag } }, message );
}

TEST_F( ModifyDeviceHouseModesTests, appendTwoDisarmedDevices /* 1.1 */ ) {
    LOGGER_TRACE( "appendTwoDisarmedDevices STARTED..." );
    JsonObject broadcast;
    // add a device here with houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device device02(
        m_gatewayId, "testDeviceWithHouseModesOptions3", "motion", "sensor.motion", "security_sensor", false );

    JsonArray disarmedModes = JSON_ARRAY_( HouseMode::Home, HouseMode::Night );
    JsonArray armedModes = JSON_ARRAY_( HouseMode::Away, HouseMode::Vacation );

    Json houseModeOptions = { { Fields::HouseModesOptions::DisarmedFieldName, disarmedModes } };
    device02.setHouseModesOptions( houseModeOptions );

    device02.setSubcategory( "motion" );
    auto deviceAddingResult = m_remote.addDevice( device02 );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto deviceId = deviceAddingResult.first;

    auto securityItemId =
        addItem( m_remote, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { Fields::ArmedFieldName, false } }, __LINE__ );

    checkHouseModesPresetsDisarmed( disarmedModes, { deviceId }, true, NMA_TEST_MAKE_MESSAGE( "DISARMED" ) );
    checkHouseModesPresetsDisarmed( armedModes, {}, true, NMA_TEST_MAKE_MESSAGE( "ARMED" ) );

    // Extend disarmed devices from 2 to 4
    {
        auto fetchedDevice = m_remote.getSingleDevice( deviceId );
        LOGGER_TRACE_RAW( cc::j( fetchedDevice ) );
        EXPECT_FALSE( fetchedDevice.find( Fields::HouseModesOptionsFieldName ) == fetchedDevice.end() )
            << "No internal fields";

        Json nextHouseModeOptions = {
            { Fields::HouseModesOptions::DisarmedFieldName,
              JSON_ARRAY_( HouseMode::Home, HouseMode::Away, HouseMode::Night, HouseMode::Vacation ) }
        };

        auto prevDisarmed = houseModeOptions[ Fields::HouseModesOptions::DisarmedFieldName ];
        auto nextDisarmed = nextHouseModeOptions[ Fields::HouseModesOptions::DisarmedFieldName ];
        auto prevSize = prevDisarmed.size();
        auto nextSize = nextDisarmed.size();
        LOGGER_TRACE( "prevDisarmed: " + cc::j( prevDisarmed ).append( ", size=" + cc::num10( prevSize ) ) );
        LOGGER_TRACE( "nextDisarmed: " + cc::j( nextDisarmed ).append( ", size=" + cc::num10( nextSize ) ) );

        int expectedModesChanged = (int) nextSize - prevSize;

        Json modifiedDevice = fetchedDevice;
        modifiedDevice[ Fields::HouseModesOptionsFieldName ] = nextHouseModeOptions;

        CoreD::Model::Device nextDevice = device02;

        nextDevice.setHouseModesOptions( nextHouseModeOptions );

        LOGGER_DEBUG_RAW( "Call modify device" );

        m_remote.modifyDevice( deviceId, nextDevice );

        std::string expectedBroadcast =
            expectedModesChanged >= 0 ? "hub.modes.disarmed_devices.added" : "hub.modes.disarmed_devices.removed";

        for( int i = 0; i < abs( expectedModesChanged ); i++ ) {
            broadcast.clear();
            broadcast = waitForBroadcast( expectedBroadcast,
                                          NMA_TEST_MAKE_MESSAGE( expectedBroadcast + ", loop N: " + cc::num10( i ) ),
                                          std::chrono::seconds( 5 ) );
            EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), expectedBroadcast );
            LOGGER_TRACE( cc::j( broadcast ) );
        }
        houseModeOptions = nextHouseModeOptions;
    }
}

TEST_F( ModifyDeviceHouseModesTests, removeDisarmedDeviceFromTwoModes /* 1.2 */ ) {
    LOGGER_TRACE( "removeDisarmedDeviceFromTwoModes STARTED..." );
    JsonObject broadcast;

    // add a device here with houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device device02(
        m_gatewayId, "testDeviceWithHouseModesOptions3", "motion", "sensor.motion", "security_sensor", false );

    JsonArray disarmedModes = JSON_ARRAY_( HouseMode::Home, HouseMode::Away, HouseMode::Night, HouseMode::Vacation );
    Json houseModeOptions = { { Fields::HouseModesOptions::DisarmedFieldName, disarmedModes } };

    device02.setHouseModesOptions( houseModeOptions );

    device02.setSubcategory( "motion" );
    auto deviceAddingResult = m_remote.addDevice( device02 );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto deviceId = deviceAddingResult.first;

    auto securityItemId =
        addItem( m_remote, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { Fields::ArmedFieldName, false } }, __LINE__ );

    checkHouseModesPresetsDisarmed( disarmedModes, { deviceId }, true, NMA_TEST_MAKE_MESSAGE( "DISARMED" ) );

    // Reduce disarmed devices from 4 to 1
    {
        LOGGER_TRACE( "Disarm 3 of 4 tests" );

        auto fetchedDevice = m_remote.getSingleDevice( deviceId );
        LOGGER_TRACE_RAW( cc::j( fetchedDevice ) );
        EXPECT_FALSE( fetchedDevice.find( Fields::HouseModesOptionsFieldName ) == fetchedDevice.end() )
            << "No internal fields";

        Json nextHouseModeOptions = { { Fields::HouseModesOptions::DisarmedFieldName,
                                        JSON_ARRAY_( HouseMode::Home ) } };

        auto prevDisarmed = houseModeOptions[ Fields::HouseModesOptions::DisarmedFieldName ];
        auto nextDisarmed = nextHouseModeOptions[ Fields::HouseModesOptions::DisarmedFieldName ];
        auto prevSize = prevDisarmed.size();
        auto nextSize = nextDisarmed.size();
        LOGGER_TRACE( "prevDisarmed: " + cc::j( prevDisarmed ).append( ", size=" + cc::num10( prevSize ) ) );
        LOGGER_TRACE( "nextDisarmed: " + cc::j( nextDisarmed ).append( ", size=" + cc::num10( nextSize ) ) );

        int expectedModesChanged = (int) nextSize - prevSize;

        Json modifiedDevice = fetchedDevice;
        modifiedDevice[ Fields::HouseModesOptionsFieldName ] = nextHouseModeOptions;

        CoreD::Model::Device nextDevice = device02;

        nextDevice.setHouseModesOptions( nextHouseModeOptions );

        LOGGER_DEBUG_RAW( "Call modify device" );

        m_remote.modifyDevice( deviceId, nextDevice );

        std::string expectedBroadcast =
            expectedModesChanged >= 0 ? "hub.modes.disarmed_devices.added" : "hub.modes.disarmed_devices.removed";

        for( int i = 0; i < abs( expectedModesChanged ); i++ ) {
            broadcast.clear();
            broadcast = waitForBroadcast( expectedBroadcast,
                                          NMA_TEST_MAKE_MESSAGE( expectedBroadcast + ", loop N: " + cc::num10( i ) ),
                                          std::chrono::seconds( 5 ) );
            EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), expectedBroadcast );
            LOGGER_TRACE( cc::j( broadcast ) );
        }
    }
}

TEST_F( ModifyDeviceHouseModesTests, sirenAlarmOffAdd /* 2.1 */ ) {
    LOGGER_TRACE( "sirenAlarmOffAdd STARTED..." );
    JsonObject broadcast;

    // add a device here with houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device device( m_gatewayId, "Controller Siren", "siren", "e550_siren", "siren", false );
    device.setSecurity( "high" );
    device.setPersistent( true );

    JsonArray alarmOffModes = JSON_ARRAY_( HouseMode::Home );
    JsonArray alarmOnModes = JSON_ARRAY_( HouseMode::Away, HouseMode::Night, HouseMode::Vacation );

    Json houseModeOptions = { { Fields::HouseModesOptions::AlarmsOffFieldName, alarmOffModes } };
    device.setHouseModesOptions( houseModeOptions );

    auto deviceAddingResult = m_remote.addDevice( device );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto deviceId = deviceAddingResult.first;

    checkHouseModesPresetsAlarmsOff( alarmOnModes, {}, true, NMA_TEST_MAKE_MESSAGE( "alarmOn" ) );
    checkHouseModesPresetsAlarmsOff( alarmOffModes, { deviceId }, true, NMA_TEST_MAKE_MESSAGE( "alarmOff" ) );

    // Extend disarmed devices from 2 to 4
    {
        auto fetchedDevice = m_remote.getSingleDevice( deviceId );
        LOGGER_TRACE_RAW( cc::j( fetchedDevice ) );
        EXPECT_FALSE( fetchedDevice.find( Fields::HouseModesOptionsFieldName ) == fetchedDevice.end() )
            << "No internal fields";

        Json nextHouseModeOptions = { { Fields::HouseModesOptions::AlarmsOffFieldName,
                                        JSON_ARRAY_( HouseMode::Home, HouseMode::Night ) } };

        auto prevAlarmOff = houseModeOptions[ Fields::HouseModesOptions::AlarmsOffFieldName ];
        auto nextAlarmOff = nextHouseModeOptions[ Fields::HouseModesOptions::AlarmsOffFieldName ];
        auto prevSize = prevAlarmOff.size();
        auto nextSize = nextAlarmOff.size();
        LOGGER_TRACE( "prevAlarmOff: " + cc::j( prevAlarmOff ).append( ", size=" + cc::num10( prevSize ) ) );
        LOGGER_TRACE( "nextAlarmOff: " + cc::j( nextAlarmOff ).append( ", size=" + cc::num10( nextSize ) ) );

        int expectedModesChanged = (int) nextSize - prevSize;

        Json modifiedDevice = fetchedDevice;
        modifiedDevice[ Fields::HouseModesOptionsFieldName ] = nextHouseModeOptions;

        CoreD::Model::Device nextDevice = device;

        nextDevice.setHouseModesOptions( nextHouseModeOptions );

        LOGGER_DEBUG_RAW( "Call modify device" );

        m_remote.modifyDevice( deviceId, nextDevice );

        std::string expectedBroadcast =
            expectedModesChanged >= 0 ? "hub.modes.alarms_off.added" : "hub.modes.alarms_off.removed";

        for( int i = 0; i < abs( expectedModesChanged ); i++ ) {
            broadcast.clear();
            broadcast = waitForBroadcast( expectedBroadcast,
                                          NMA_TEST_MAKE_MESSAGE( expectedBroadcast + ", loop N: " + cc::num10( i ) ),
                                          std::chrono::seconds( 5 ) );
            EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), expectedBroadcast );
            LOGGER_TRACE( cc::j( broadcast ) );
        }
    }
}

TEST_F( ModifyDeviceHouseModesTests, sirenAlarmOffRemove /* 2.2 */ ) {
    LOGGER_TRACE( "sirenAlarmOffRemove STARTED..." );
    JsonObject broadcast;

    // add a device here with houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device device( m_gatewayId, "Controller Siren", "siren", "e550_siren", "siren", false );
    device.setSecurity( "high" );
    device.setPersistent( true );

    JsonArray alarmOffModes = JSON_ARRAY_( HouseMode::Home, HouseMode::Away, HouseMode::Night, HouseMode::Vacation );
    JsonArray alarmOnModes = JSON_ARRAY_();

    Json houseModeOptions = { { Fields::HouseModesOptions::AlarmsOffFieldName, alarmOffModes } };
    device.setHouseModesOptions( houseModeOptions );

    auto deviceAddingResult = m_remote.addDevice( device );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto deviceId = deviceAddingResult.first;

    checkHouseModesPresetsAlarmsOff( alarmOnModes, {}, true, NMA_TEST_MAKE_MESSAGE( "alarmOn" ) );
    checkHouseModesPresetsAlarmsOff( alarmOffModes, { deviceId }, true, NMA_TEST_MAKE_MESSAGE( "alarmOff" ) );

    // Remove from two modes
    {
        auto fetchedDevice = m_remote.getSingleDevice( deviceId );
        LOGGER_TRACE_RAW( cc::j( fetchedDevice ) );
        EXPECT_FALSE( fetchedDevice.find( Fields::HouseModesOptionsFieldName ) == fetchedDevice.end() )
            << "No internal fields";

        Json nextHouseModeOptions = { { Fields::HouseModesOptions::AlarmsOffFieldName,
                                        JSON_ARRAY_( HouseMode::Home, HouseMode::Night ) } };

        auto prevAlarmOff = houseModeOptions[ Fields::HouseModesOptions::AlarmsOffFieldName ];
        auto nextAlarmOff = nextHouseModeOptions[ Fields::HouseModesOptions::AlarmsOffFieldName ];
        auto prevSize = prevAlarmOff.size();
        auto nextSize = nextAlarmOff.size();
        LOGGER_TRACE( "prevAlarmOff: " + cc::j( prevAlarmOff ).append( ", size=" + cc::num10( prevSize ) ) );
        LOGGER_TRACE( "nextAlarmOff: " + cc::j( nextAlarmOff ).append( ", size=" + cc::num10( nextSize ) ) );

        int expectedModesChanged = (int) nextSize - prevSize;

        Json modifiedDevice = fetchedDevice;
        modifiedDevice[ Fields::HouseModesOptionsFieldName ] = nextHouseModeOptions;

        CoreD::Model::Device nextDevice = device;

        nextDevice.setHouseModesOptions( nextHouseModeOptions );

        LOGGER_DEBUG_RAW( "Call modify device" );

        m_remote.modifyDevice( deviceId, nextDevice );

        std::string expectedBroadcast =
            expectedModesChanged >= 0 ? "hub.modes.alarms_off.added" : "hub.modes.alarms_off.removed";

        for( int i = 0; i < abs( expectedModesChanged ); i++ ) {
            broadcast.clear();
            broadcast = waitForBroadcast( expectedBroadcast,
                                          NMA_TEST_MAKE_MESSAGE( expectedBroadcast + ", loop N: " + cc::num10( i ) ),
                                          std::chrono::seconds( 5 ) );
            EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), expectedBroadcast );
            LOGGER_TRACE( cc::j( broadcast ) );
        }
    }
}

TEST_F( ModifyDeviceHouseModesTests, addPanicButtonToArmedDevices /* 3.1 */ ) {
    LOGGER_TRACE( "addPanicButtonToArmedDevices STARTED..." );
    JsonObject broadcast;

    // add a device here with houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device panicButtonDevice(
        m_gatewayId, "testPanicButton01", "controller.panic", "panic_button", "scene_controller", false );
    panicButtonDevice.setSubcategory( "panic" );

    bool isGood;
    std::string deviceId, itemId;
    std::tie( deviceId, isGood ) = m_remote.addDevice( panicButtonDevice );
    ASSERT_TRUE( isGood );

    checkBroadcast( deviceId, "hub.device.added", {}, NMA_TEST_MAKE_MESSAGE( "device added" ) );

    const bool shouldBeArmed = true;
    createSecurityThreatItem( itemId,
                              deviceId,
                              shouldBeArmed,
                              NMA_TEST_MAKE_MESSAGE( "security item created, device added to house modes" ) );

    auto deviceIds = m_remote.getArmedDeviceIDsAffectedByHouseModes();
    LOGGER_TRACE_RAW( "m_remote.getArmedDeviceIDsAffectedByHouseModes() returned array: size=" +
                      cc::num10( deviceIds.size() ) );
    ASSERT_EQ( deviceIds.size(), 1 );
    for( const auto& id : deviceIds ) {
        LOGGER_TRACE_RAW( id );
    }
    EXPECT_EQ( deviceId, deviceIds[ 0 ] );

    {
        const auto answer = callHouseModesGet( NMA_TEST_MAKE_MESSAGE( "" ) );
        JsonArray devices = fetchResultDevices( answer, NMA_TEST_MAKE_MESSAGE( "" ) );
        LOGGER_TRACE_RAW( cc::p( "hub.modes.get/result/devices: " ) + cc::j( devices ) );

        ASSERT_EQ( devices.size(), 1 );
        ASSERT_EQ( deviceId, devices[ 0 ] );
    }
}

TEST_F( ModifyDeviceHouseModesTests, addButtonThenModifyToPanicButton /* 3.2 */ ) {
    LOGGER_TRACE_RAW( "addButtonThenModifyToPanicButton STARTED..." );
    JsonObject broadcast;

    const int houseModesCount = m_remote.getHouseModes().size();

    // add a device here with houseModesOptions dictionary set.
    using namespace HubPlatform::CoreD::Model::Resources;
    CoreD::Model::Device buttonDevice(
        m_gatewayId, "testButton01", "controller.portable", "controller.portable", "scene_controller", false );
    buttonDevice.setSubcategory( "generic" );

    bool isGood;
    std::string deviceId, itemId;
    std::tie( deviceId, isGood ) = m_remote.addDevice( buttonDevice );
    ASSERT_TRUE( isGood );

    checkBroadcast( deviceId,
                    "hub.device.added",
                    { { "$has-not", "armed" }, { "subcategory", "generic" } },
                    NMA_TEST_MAKE_MESSAGE( "generic device added" ) );

    const bool shouldNotBeArmed = false;
    createSecurityThreatItem( itemId,
                              deviceId,
                              shouldNotBeArmed,
                              NMA_TEST_MAKE_MESSAGE( "security item created, device added to house modes" ) );

    auto deviceIds = m_remote.getArmedDeviceIDsAffectedByHouseModes();
    ASSERT_EQ( deviceIds.size(), 1 ) << "Generic device added to armed devices";
    const auto houseModeId = m_remote.getCurrentHouseModeId();

    {
        auto houseMode = m_remote.getHouseModeById( houseModeId );
        Log::debug( "Current house mode: " + cc::j( houseMode->toJson() ) );
        // should be added to disarmed by default
        EXPECT_EQ( houseMode->m_disarmedDeviceIds.count( deviceId ), 1 )
            << "deviceId should be in 'disarmed' collection";
    }
    // modify device: change category/subcategory to become a panic button
    CoreD::Model::Device panicButtonDevice = buttonDevice;
    panicButtonDevice.setSubcategory( "panic" );

    m_remote.modifyDevice( deviceId, panicButtonDevice );
    checkBroadcast( deviceId,
                    "hub.device.updated",
                    { /*{ "armed", true },*/ { "subcategory", "panic" } },
                    NMA_TEST_MAKE_MESSAGE( "hub.device.updated to panic button" ) );
    for( int i = 0; i < houseModesCount; i++ ) {
        checkBroadcast( "",
                        "hub.modes.disarmed_devices.removed",
                        { { "disarmedDevice", deviceId } },
                        NMA_TEST_MAKE_MESSAGE( "hub.modes.disarmed_devices.removed" ) );
    }

    // re-check armed devices
    deviceIds = m_remote.getArmedDeviceIDsAffectedByHouseModes();
    ASSERT_EQ( deviceIds.size(), 1 ) << "should have armed device in the list";
    EXPECT_EQ( deviceId, deviceIds[ 0 ] );

    {
        auto houseMode = m_remote.getHouseModeById( houseModeId );
        Log::debug( "Current house mode: " + cc::j( houseMode->toJson() ) );
        // should be added to disarmed by default
        EXPECT_EQ( houseMode->m_disarmedDeviceIds.count( deviceId ), 0 )
            << "deviceId should be removed from 'disarmed' collection";
    }

    {
        const auto answer = callHouseModesGet( NMA_TEST_MAKE_MESSAGE( "" ) );
        JsonArray devices = fetchResultDevices( answer, NMA_TEST_MAKE_MESSAGE( "" ) );
        LOGGER_TRACE_RAW( cc::p( "hub.modes.get/result/devices: " ) + cc::j( devices ) );

        ASSERT_EQ( devices.size(), 1 ) << "should have armed device in the list";
        ASSERT_EQ( deviceId, devices[ 0 ] );
    }

    // modify device back to ordinal button
    m_remote.modifyDevice( deviceId, buttonDevice );
    checkBroadcast( deviceId,
                    "hub.device.updated",
                    { // { "armed", false },
                      { "subcategory", "generic" } },
                    NMA_TEST_MAKE_MESSAGE( "hub.device.updated back to generic device" ) );

    deviceIds = m_remote.getArmedDeviceIDsAffectedByHouseModes();
    ASSERT_EQ( deviceIds.size(), 1 ) << "armed devices list should be empty";

    {
        auto houseMode = m_remote.getHouseModeById( houseModeId );
        Log::debug( "Current house mode: " + cc::j( houseMode->toJson() ) );
        // should be added to disarmed by default
        EXPECT_EQ( houseMode->m_disarmedDeviceIds.count( deviceId ), 1 )
            << "deviceId should be in 'disarmed' collection";
    }

    for( int i = 0; i < houseModesCount; i++ ) {
        checkBroadcast( "",
                        "hub.modes.disarmed_devices.added",
                        { { "disarmedDevice", deviceId } },
                        NMA_TEST_MAKE_MESSAGE( "hub.modes.disarmed_devices.added" ) );
    }
}

TEST_F( ModifyDeviceHouseModesTests, modifyDeviceWithSameCategories_armedShouldNotBeChanged /* 1.3 */ ) {
    LOGGER_TRACE_RAW( "modifyDeviceWithSameCategories_armedShouldNotBeChanged STARTED..." );
    JsonObject broadcast;

    // add a device here without houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device device02(
        m_gatewayId, "testArmedShouldNotBeChange", "sensor", "sensor.motion", "security_sensor", true );
    device02.setSubcategory( "door" );
    auto deviceAddingResult = m_remote.addDevice( device02 );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto deviceId = deviceAddingResult.first;

    auto deviceBefore = m_remote.getSingleDevice( deviceId );
    LOGGER_TRACE_RAW( "Before modify_device: " + cc::j( deviceBefore ) );

    CoreD::Model::Device nextDevice = device02;
    nextDevice.setReady( true );

    LOGGER_DEBUG_RAW( "Call modify device" );

    m_remote.modifyDevice( deviceId, nextDevice );

    auto deviceAfter = m_remote.getSingleDevice( deviceId );
    LOGGER_TRACE_RAW( "device after modify_device(): " + cc::j( deviceAfter ) );
    ASSERT_EQ( json_get_bool( deviceBefore, "armed", false ), json_get_bool( deviceAfter, "armed", false ) );
}

TEST_F( ModifyDeviceHouseModesTests, modifyItem_stopBeingSecurityThreatItem ) /* 4.1 */ {
    Log::trace( cc::trace( "Started test case: " ) + test_info_->test_case_name() + cc::trace( ", test: " ) +
                test_info_->name() );

    using namespace HubPlatform::CoreD::Model;
    using namespace HubPlatform::CoreD::Model::Resources;
    JsonObject broadcast;
    Device device( m_gatewayId, test_info_->name(), "sensor", "0_0_7", "security_sensor", true );
    device.setSubcategory( "co" );

    bool isGood;
    std::string deviceId, itemId;
    std::tie( deviceId, isGood ) = m_remote.addDevice( device );
    ASSERT_TRUE( isGood );
    ASSERT_FALSE( deviceId.empty() );

    checkBroadcast( deviceId, "hub.device.added", {}, NMA_TEST_MAKE_MESSAGE( "device added" ) );

    using namespace CoreDHouseModes::Resources;
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item item( deviceId,
                             Items::SecurityThreat,
                             valueType,
                             true,
                             true,
                             CoreD::Model::ValueFactory::createDefaultItemValue( valueType ),
                             "" );

    std::tie( itemId, isGood ) = m_remote.addItem( item );
    checkBroadcast( itemId, "hub.item.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );
    checkBroadcast( deviceId,
                    "hub.device.updated",
                    { { DeviceFields::ArmedFieldName, true } },
                    NMA_TEST_MAKE_MESSAGE( "device.armed set to true" ) );

    {
        const auto answer = callHouseModesGet( NMA_TEST_MAKE_MESSAGE( "" ) );
        JsonArray devices = fetchResultDevices( answer, NMA_TEST_MAKE_MESSAGE( "" ) );
        LOGGER_TRACE_RAW( cc::p( "hub.modes.get/result/devices: " ) + cc::j( devices ) );

        ASSERT_EQ( devices.size(), 1 );
        ASSERT_EQ( deviceId, devices[ 0 ] );
    }

    Json prevItem = m_remote.getItem( itemId );

    const char* newItemName = "another_name";
    CoreD::Model::Item newItem( deviceId,
                                newItemName,
                                valueType,
                                false,
                                true,
                                CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );
    ASSERT_TRUE( m_remote.modifyItem( itemId, newItem ) );
    checkBroadcast( itemId,
                    "hub.item.updated",
                    { { ItemFields::NameFieldName, newItemName } },
                    NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    checkBroadcast( deviceId,
                    "hub.device.updated",
                    { { DeviceFields::ArmedFieldName, false } },
                    NMA_TEST_MAKE_MESSAGE( "device.armed set to false" ) );

    {
        const auto answer = callHouseModesGet( NMA_TEST_MAKE_MESSAGE( "" ) );
        JsonArray devices = fetchResultDevices( answer, NMA_TEST_MAKE_MESSAGE( "" ) );
        LOGGER_TRACE_RAW( cc::p( "hub.modes.get/result/devices: " ) + cc::j( devices ) );

        ASSERT_EQ( devices.size(), 0 ) << "Should be no devices";
    }
}

TEST_F( ModifyDeviceHouseModesTests, modifyItem_startBeingSecurityThreatItem ) /* 4.2 */ {
    Log::trace( cc::trace( "Started test case: " ) + test_info_->test_case_name() + cc::trace( ", test: " ) +
                test_info_->name() );

    using namespace HubPlatform::CoreD::Model;
    using namespace HubPlatform::CoreD::Model::Resources;
    JsonObject broadcast;
    Device device( m_gatewayId, test_info_->name(), "sensor", "0_0_7", "security_sensor", true );
    device.setSubcategory( "co" );

    bool isGood;
    std::string deviceId, itemId;
    std::tie( deviceId, isGood ) = m_remote.addDevice( device );
    ASSERT_TRUE( isGood );
    ASSERT_FALSE( deviceId.empty() );

    checkBroadcast( deviceId, "hub.device.added", {}, NMA_TEST_MAKE_MESSAGE( "device added" ) );

    using namespace CoreDHouseModes::Resources;
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item item( deviceId,
                             "another_name",
                             valueType,
                             true,
                             true,
                             CoreD::Model::ValueFactory::createDefaultItemValue( valueType ),
                             "" );

    std::tie( itemId, isGood ) = m_remote.addItem( item );
    checkBroadcast( itemId, "hub.item.added", {}, NMA_TEST_MAKE_MESSAGE( "item added" ) );

    {
        const auto answer = callHouseModesGet( NMA_TEST_MAKE_MESSAGE( "" ) );
        JsonArray devices = fetchResultDevices( answer, NMA_TEST_MAKE_MESSAGE( "" ) );

        ASSERT_EQ( devices.size(), 0 );
    }

    Json prevItem = m_remote.getItem( itemId );

    const char* newItemName = Items::SecurityThreat;
    CoreD::Model::Item newItem( deviceId,
                                newItemName,
                                valueType,
                                true,
                                false,
                                CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );
    ASSERT_TRUE( m_remote.modifyItem( itemId, newItem ) );
    checkBroadcast( itemId,
                    "hub.item.updated",
                    { { ItemFields::NameFieldName, newItemName } },
                    NMA_TEST_MAKE_MESSAGE( "item updated" ) );

    {
        const auto answer = callHouseModesGet( NMA_TEST_MAKE_MESSAGE( "" ) );
        JsonArray devices = fetchResultDevices( answer, NMA_TEST_MAKE_MESSAGE( "" ) );
        LOGGER_TRACE_RAW( cc::p( "hub.modes.get/result/devices: " ) + cc::j( devices ) );

        ASSERT_EQ( devices.size(), 1 ) << "Should be one devices";
    }
}

}  // namespace Tests
}  // namespace HubPlatform
