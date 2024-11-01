#include "nma_api_tests/tests/fixtures/house_mode_fixture.h"

#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1. add bypass devices
DONE    2. add bypass devices to all house modes
DONE    3. remove bypass devices
DONE    4. add valid and invalid devices
DONE    5. broadcast on removal of bypassed device
DONE    6. remove "security_threat" item from bypass device
DONE    7. rename "security_threat" item for bypass device
DONE    8. protect notification not sent for bypass device
DONE    9. alarm not activated for bypass device

*/

class HouseModesBypassTests : public TestsHouseModeFixture {
public:
    using DeviceIds = std::vector< std::string >;
    using HouseModeToDevicesMap = std::map< std::string, DeviceIds >;

    void SetUp() override;
    void TearDown() override;

    std::string createSecurityDevice( bool expectedArmedState, int codeLine );
    std::string createSirenDevice( int codeLine );
    DeviceIds createDevices( size_t num, bool expectedArmedState, int codeLine );
    void cleanup( const HouseModeToDevicesMap& bypassDevices, int codeLine );

    void addBypass( const std::string& modeId, const DeviceIds& deviceIds, int codeLine );
    void removeBypass( const std::string& modeId, const DeviceIds& deviceIds, int codeLine );
    void checkBypassDevices( const HouseModeToDevicesMap& expected, int codeLine );
    static HouseModeToDevicesMap getEmptyModeToDevicesMap();
    static Json sendAddBypassRequest( const std::string& modeId, const DeviceIds& deviceIds );

    void triggerBypassedSecurityItem( CoreDRemoteInterface::CoreDInterface& coredRemote,
                                      const std::string& itemId,
                                      int codeLine );
    void switchToHouseMode( CoreDRemoteInterface::CoreDInterface& coredRemote,
                            const std::string& modeId,
                            int codeLine );
    bool setHouseModeProtectOn( CoreDRemoteInterface::CoreDInterface& coredRemote,
                                const std::string& modeId,
                                int codeLine );

    void waitDisarmedDeviceRemovedBroadcast( int codeLine );
    void waitDeviceUpdatedBroadcast( const std::string& deviceId, bool expectedArmedState, int codeLine );
    void waitBypassAddedBroadcast( const std::string& modeId, const DeviceIds& deviceIds, int codeLine );
    void waitBypassRemovedBroadcast( const std::string& modeId, const DeviceIds& deviceIds, int codeLine );
    void waitBypassBroadcast( const std::string& broadcastName,
                              const std::string& modeId,
                              const DeviceIds& deviceIds,
                              int codeLine );

public:
    std::map< std::string, std::string > m_deviceToItemMap;
};

void HouseModesBypassTests::SetUp() {
    TestsHouseModeFixture::SetUp();
    callingFile( __FILE__ );
}

void HouseModesBypassTests::TearDown() {
    const std::string defaultHouseModeId = "1";
    if( makeRemote().getCurrentHouseModeId() != defaultHouseModeId )
        switchToHouseMode( makeRemote(), defaultHouseModeId, __LINE__ );

    TestsHouseModeFixture::TearDown();
}

std::string HouseModesBypassTests::createSecurityDevice( bool expectedArmedState, int codeLine ) {
    auto& remote = makeRemote();

    const bool batteryPowered = false;
    const std::string category = "security_sensor";
    const std::string name = "test_device";
    auto deviceId = addDevice( remote, name, "sensor", "134_3_24", category, "door", batteryPowered, codeLine );

    const auto itemType = CoreD::Model::ValueType::Enum::Bool;
    auto itemId = addItem( remote, deviceId, "security_threat", itemType, true, true, codeLine );
    waitDeviceUpdatedBroadcast( deviceId, expectedArmedState, codeLine );

    m_deviceToItemMap.emplace( deviceId, std::move( itemId ) );
    return deviceId;
}

std::string HouseModesBypassTests::createSirenDevice( int codeLine ) {
    auto& remote = makeRemote();

    const bool batteryPowered = false;
    auto deviceId = addDevice( remote, "test_siren", "siren", "134_3_24", "siren", "", batteryPowered, codeLine );

    const auto itemType = CoreD::Model::ValueType::Enum::Bool;
    auto itemId = addItem( remote, deviceId, "siren", itemType, true, true, codeLine );

    m_deviceToItemMap.emplace( deviceId, std::move( itemId ) );
    return deviceId;
}

HouseModesBypassTests::DeviceIds HouseModesBypassTests::createDevices( size_t num,
                                                                       bool expectedArmedState,
                                                                       int codeLine ) {
    DeviceIds result;
    result.reserve( num );
    for( size_t i = 0; i < num; ++i ) {
        result.push_back( createSecurityDevice( expectedArmedState, codeLine ) );
    }
    return result;
}

void HouseModesBypassTests::cleanup( const HouseModeToDevicesMap& bypassDevices, int codeLine ) {
    // to avoid hub.modes.bypass_devices.removed broadcasts
    for( const auto& houseModeDevicesPair : bypassDevices ) {
        const std::string& houseModeId = houseModeDevicesPair.first;
        const DeviceIds& deviceIds = houseModeDevicesPair.second;
        if( !deviceIds.empty() ) {
            removeBypass( houseModeId, deviceIds, codeLine );
            waitBypassRemovedBroadcast( houseModeId, deviceIds, codeLine );
        }
    }
    clearExistingDevices( __LINE__ );
}

void HouseModesBypassTests::triggerBypassedSecurityItem( CoreDRemoteInterface::CoreDInterface& coredRemote,
                                                         const std::string& itemId,
                                                         int codeLine ) {
    namespace ItemFields = CoreD::Model::Resources::ItemFields;
    coredRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << true ) );
    const Json broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    const Json& result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( result.count( "protectNotification" ), 0 );
    EXPECT_EQ( result.count( "protectService" ), 0 );
}

void HouseModesBypassTests::addBypass( const std::string& modeId, const DeviceIds& deviceIds, int codeLine ) {
    const auto answer = sendAddBypassRequest( modeId, deviceIds );
    EXPECT_EQ( answer[ "result" ], JSON_EMPTY_OBJ ) << getCodeLineString( codeLine );
    EXPECT_EQ( answer[ "error" ], JSON_NULL_VAL )
        << getCodeLineString( codeLine ) << ", error: " << answer[ "error" ].dump();
}

void HouseModesBypassTests::removeBypass( const std::string& modeId, const DeviceIds& deviceIds, int codeLine ) {
    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.bypass_devices.remove" },
                                       { "params", JSON( "modeId" << modeId << "deviceIds" << Json( deviceIds ) ) } } );

    EXPECT_EQ( answer[ "result" ], JSON_EMPTY_OBJ ) << getCodeLineString( codeLine );
    EXPECT_EQ( answer[ "error" ], JSON_NULL_VAL )
        << getCodeLineString( codeLine ) << ", error: " << answer[ "error" ].dump();
}

void HouseModesBypassTests::checkBypassDevices( const HouseModeToDevicesMap& expected, int codeLine ) {
    auto checkHouseMode = [&expected]( const Json& mode, const std::string& errorMessage ) {
        JsonArray bypassDevices;
        EXPECT_TRUE( json_get_arr( mode, "bypassDevices", bypassDevices ) ) << errorMessage;

        const std::string houseModeId = json_get_str( mode, "_id" );
        auto it = expected.find( houseModeId );
        ASSERT_NE( it, expected.end() ) << errorMessage;
        const JsonArray expectedBypassDevices( it->second );
        EXPECT_EQ( expectedBypassDevices.dump(), bypassDevices.dump() ) << errorMessage;
    };
    const std::string message = getCodeLineString( codeLine );
    checkHouseModesGet( checkHouseMode, message, nullptr );
}

HouseModesBypassTests::HouseModeToDevicesMap HouseModesBypassTests::getEmptyModeToDevicesMap() {
    return { { "1", DeviceIds() }, { "2", DeviceIds() }, { "3", DeviceIds() }, { "4", DeviceIds() } };
}

Json HouseModesBypassTests::sendAddBypassRequest( const std::string& modeId, const DeviceIds& deviceIds ) {
    return nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.bypass_devices.add" },
          { "params", JSON( "modeId" << modeId << "deviceIds" << Json( deviceIds ) ) } } );
}

void HouseModesBypassTests::switchToHouseMode( CoreDRemoteInterface::CoreDInterface& coredRemote,
                                               const std::string& modeId,
                                               int codeLine ) {
    coredRemote.setSwitchDelay( 0 );
    coredRemote.setHouseMode( modeId );
    testHouseModesSwitchedBroadcast( modeId, "begin", codeLine );
    testHouseModesSwitchedBroadcast( modeId, "done", codeLine );
}

bool HouseModesBypassTests::setHouseModeProtectOn( CoreDRemoteInterface::CoreDInterface& coredRemote,
                                                   const std::string& modeId,
                                                   int codeLine ) {
    auto houseModes = coredRemote.getHouseModes();
    auto it = std::find_if(
        houseModes.begin(), houseModes.end(), [&modeId]( const CoreDHouseModes::HouseModeEntry& houseMode ) {
            return houseMode.m_id == modeId;
        } );

    EXPECT_NE( it, houseModes.end() );
    if( it == houseModes.end() )
        return false;

    if( !it->m_protect ) {
        coredRemote.houseModeProtectSet( modeId, true );
        const Json broadcast = waitForBroadcast( "hub.modes.changed", __FILE__ + std::to_string( codeLine ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        int64_t timestamp;
        EXPECT_TRUE( json_get( broadcast[ "result" ], "timestamp", timestamp ) );
    }

    return true;
}

void HouseModesBypassTests::waitDisarmedDeviceRemovedBroadcast( int codeLine ) {
    const Json broadcast =
        waitForBroadcast( "hub.modes.disarmed_devices.removed", __FILE__ + std::to_string( codeLine ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.disarmed_devices.removed" );
}

void HouseModesBypassTests::waitDeviceUpdatedBroadcast( const std::string& deviceId,
                                                        bool expectedArmedState,
                                                        int codeLine ) {
    const char* armedFieldName = CoreD::Model::Resources::DeviceFields::ArmedFieldName;
    checkDeviceUpdateBroadcast( deviceId, { { armedFieldName, expectedArmedState } }, codeLine );
}

void HouseModesBypassTests::waitBypassAddedBroadcast( const std::string& modeId,
                                                      const DeviceIds& deviceIds,
                                                      int codeLine ) {
    waitBypassBroadcast( "hub.modes.bypass_devices.added", modeId, deviceIds, codeLine );
}

void HouseModesBypassTests::waitBypassRemovedBroadcast( const std::string& modeId,
                                                        const DeviceIds& deviceIds,
                                                        int codeLine ) {
    waitBypassBroadcast( "hub.modes.bypass_devices.removed", modeId, deviceIds, codeLine );
}

void HouseModesBypassTests::waitBypassBroadcast( const std::string& broadcastName,
                                                 const std::string& modeId,
                                                 const DeviceIds& deviceIds,
                                                 int codeLine ) {
    auto getErrorString = [this, codeLine]( const Json& broadcast ) {
        return getCodeLineString( codeLine ) + ", broadcast: " + broadcast.dump();
    };

    auto broadcast = waitForBroadcast( broadcastName, __FILE__ + std::to_string( codeLine ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), broadcastName ) << getErrorString( broadcast );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), modeId ) << getErrorString( broadcast );

    JsonArray addedDevices;
    EXPECT_TRUE( json_get_arr( broadcast[ "result" ], "deviceIds", addedDevices ) ) << getErrorString( broadcast );
    EXPECT_EQ( addedDevices.dump(), Json( deviceIds ).dump() ) << getErrorString( broadcast );
}

TEST_F( HouseModesBypassTests, addBypassDevices ) {  // 1.
    const std::string houseModeId = "1";
    const bool expectedArmedState = false;
    const DeviceIds deviceIds = createDevices( 3, expectedArmedState, __LINE__ );
    addBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, deviceIds, __LINE__ );

    HouseModeToDevicesMap expected = getEmptyModeToDevicesMap();
    expected[ houseModeId ] = deviceIds;
    checkBypassDevices( expected, __LINE__ );
    cleanup( expected, __LINE__ );
}

TEST_F( HouseModesBypassTests, addBypassDevicesToAllHouseModes ) {  // 2.
    const bool expectedArmedState = false;
    const HouseModeToDevicesMap modeIdToDevices = { { "1", createDevices( 1, expectedArmedState, __LINE__ ) },
                                                    { "2", createDevices( 1, expectedArmedState, __LINE__ ) },
                                                    { "3", createDevices( 1, expectedArmedState, __LINE__ ) },
                                                    { "4", createDevices( 1, expectedArmedState, __LINE__ ) } };
    for( const auto& modeIdDeviceIdPair : modeIdToDevices ) {
        const std::string& modeId = modeIdDeviceIdPair.first;
        const DeviceIds& deviceIds = modeIdDeviceIdPair.second;
        addBypass( modeId, deviceIds, __LINE__ );
        waitBypassAddedBroadcast( modeId, deviceIds, __LINE__ );
    }

    checkBypassDevices( modeIdToDevices, __LINE__ );
    cleanup( modeIdToDevices, __LINE__ );
}

TEST_F( HouseModesBypassTests, removeBypassDevices ) {  // 3.
    const std::string houseModeId = "1";
    const bool expectedArmedState = false;
    const DeviceIds deviceIds = createDevices( 3, expectedArmedState, __LINE__ );
    addBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, deviceIds, __LINE__ );
    removeBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassRemovedBroadcast( houseModeId, deviceIds, __LINE__ );

    checkBypassDevices( getEmptyModeToDevicesMap(), __LINE__ );
}

TEST_F( HouseModesBypassTests, addValidAndInvalidDevices ) {  // 4.
    const bool expectedArmedState = false;
    const std::string deviceId1 = createSecurityDevice( expectedArmedState, __LINE__ );
    const std::string deviceId2 = "invalid";

    const std::string houseModeId = "1";
    const DeviceIds deviceIds = { deviceId1, deviceId2 };
    const auto answer = sendAddBypassRequest( houseModeId, deviceIds );
    EXPECT_EQ( answer[ "result" ], JSON_EMPTY_OBJ );

    const Json& error = answer[ "error" ];
    EXPECT_EQ( json_get_int( error, "code", 0 ), static_cast< int >( Errors::Codes::INVALID_PARAMS ) ) << error;
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" ) << error;

    // the device is added despite the fact, that the other one is invalid
    waitBypassAddedBroadcast( houseModeId, { deviceId1 }, __LINE__ );

    const HouseModeToDevicesMap expected = {
        { houseModeId, DeviceIds{ deviceId1 } }, { "2", DeviceIds() }, { "3", DeviceIds() }, { "4", DeviceIds() }
    };
    checkBypassDevices( expected, __LINE__ );
    cleanup( expected, __LINE__ );
}

TEST_F( HouseModesBypassTests, broadcastOnRemovalOfBypassedDevice ) {  // 5.
    const bool expectedArmedState = false;
    const DeviceIds deviceIds = createDevices( 1, expectedArmedState, __LINE__ );
    const std::string houseModeId = "1";
    addBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, deviceIds, __LINE__ );

    makeRemote().removeDevice( deviceIds.front() );
    waitBypassRemovedBroadcast( houseModeId, deviceIds, __LINE__ );

    JsonObject broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( HouseModesBypassTests, removeSecurityThreatFromBypassDevice ) {  // 6.
    auto& coredRemote = makeRemote();
    const std::string houseModeId = "2";  // using house mode "2" to keep device armed
    switchToHouseMode( coredRemote, houseModeId, __LINE__ );

    const bool expectedArmedState = true;
    const DeviceIds deviceIds = createDevices( 1, expectedArmedState, __LINE__ );
    addBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, deviceIds, __LINE__ );

    const std::string& itemId = m_deviceToItemMap[ deviceIds.front() ];
    ASSERT_FALSE( itemId.empty() );
    coredRemote.removeItem( itemId );

    waitDisarmedDeviceRemovedBroadcast( __LINE__ );
    waitBypassRemovedBroadcast( houseModeId, deviceIds, __LINE__ );

    const Json broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    waitDeviceUpdatedBroadcast( deviceIds.front(), !expectedArmedState, __LINE__ );

    checkBypassDevices( getEmptyModeToDevicesMap(), __LINE__ );
}

TEST_F( HouseModesBypassTests, renameSecurityThreatForBypassDevice ) {  // 7.
    auto& coredRemote = makeRemote();
    const std::string houseModeId = "2";  // using house mode "2" to keep device armed
    switchToHouseMode( coredRemote, houseModeId, __LINE__ );

    const bool expectedArmedState = true;
    const DeviceIds deviceIds = createDevices( 1, expectedArmedState, __LINE__ );
    addBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, deviceIds, __LINE__ );

    const std::string& deviceId = deviceIds.front();
    {
        const auto valueType = CoreD::Model::ValueType::Enum::Int;
        auto value = CoreD::Model::ValueFactory::createDefaultItemValue( valueType );
        CoreD::Model::Item newItem( deviceId, "another_name", valueType, false, true, std::move( value ) );

        ASSERT_FALSE( m_deviceToItemMap[ deviceId ].empty() );
        coredRemote.modifyItem( m_deviceToItemMap[ deviceId ], newItem );
    }

    const Json broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    waitDisarmedDeviceRemovedBroadcast( __LINE__ );
    waitBypassRemovedBroadcast( houseModeId, deviceIds, __LINE__ );
    waitDeviceUpdatedBroadcast( deviceId, !expectedArmedState, __LINE__ );

    checkBypassDevices( getEmptyModeToDevicesMap(), __LINE__ );
}

TEST_F( HouseModesBypassTests, protectNotificationNotSentForBypassDevice ) {  // 8.
    auto& coredRemote = makeRemote();
    const std::string houseModeId = "2";  // using house mode "2" to keep device armed
    switchToHouseMode( coredRemote, houseModeId, __LINE__ );
    coredRemote.setAlarmDelay( 0 );
    const bool expectedArmedState = true;
    const DeviceIds deviceIds = createDevices( 1, expectedArmedState, __LINE__ );
    const std::string& itemId = m_deviceToItemMap[ deviceIds.front() ];
    ASSERT_FALSE( itemId.empty() );

    addBypass( houseModeId, deviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, deviceIds, __LINE__ );

    ASSERT_TRUE( setHouseModeProtectOn( coredRemote, houseModeId, __LINE__ ) );
    triggerBypassedSecurityItem( coredRemote, itemId, __LINE__ );

    testHouseModeAlarmedPhaseStatusBroadcast( houseModeId, deviceIds.front(), "bypass", "done", __LINE__ );
    HouseModeToDevicesMap expected = getEmptyModeToDevicesMap();
    expected[ houseModeId ] = deviceIds;
    cleanup( expected, __LINE__ );
}

TEST_F( HouseModesBypassTests, alarmNotActivatedForBypassDevice ) {  // 9.
    auto& coredRemote = makeRemote();
    const std::string houseModeId = "2";  // using house mode "2" to keep device armed
    switchToHouseMode( coredRemote, houseModeId, __LINE__ );
    coredRemote.setAlarmDelay( 0 );
    const bool expectedArmedState = true;
    const DeviceIds bypassDeviceIds = createDevices( 1, expectedArmedState, __LINE__ );
    const std::string& securityThreatItemId = m_deviceToItemMap[ bypassDeviceIds.front() ];
    ASSERT_FALSE( securityThreatItemId.empty() );

    const std::string sirenDeviceId = createSirenDevice( __LINE__ );
    const std::string& sirenItemId = m_deviceToItemMap[ sirenDeviceId ];
    ASSERT_FALSE( sirenItemId.empty() );

    addBypass( houseModeId, bypassDeviceIds, __LINE__ );
    waitBypassAddedBroadcast( houseModeId, bypassDeviceIds, __LINE__ );

    ASSERT_TRUE( setHouseModeProtectOn( coredRemote, houseModeId, __LINE__ ) );
    triggerBypassedSecurityItem( coredRemote, securityThreatItemId, __LINE__ );
    testHouseModeAlarmedBroadcast( houseModeId, bypassDeviceIds.front(), __LINE__ );

    const JsonObject item = coredRemote.getItem( sirenItemId );
    const bool itemValue = json_get_bool( item, CoreD::Model::Resources::ItemFields::ValueFieldName, true );
    EXPECT_FALSE( itemValue );

    HouseModeToDevicesMap expected = getEmptyModeToDevicesMap();
    expected[ houseModeId ] = bypassDeviceIds;
    cleanup( expected, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
