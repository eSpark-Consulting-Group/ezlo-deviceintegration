#include "nma_api_tests/application.h"
#include "jsonrpc/jsonrpc.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include <database_client/dbqueryparser.h>

/*

TESTPLAN

DONE    1. Check mark as 'deleted' swinger shutdown record after device removing
DONE    2. Check mark as 'deleted' swinger shutdown record after item updating
DONE    3. Check mark as 'deleted' swinger shutdown record after item removing
DONE    4. Check swinger shutdown hub.modes.get defaults
DONE    5. Check swinger shutdown list command
DONE    6. Check swinger shutdown reset command
DONE    7. Add removed devices error
DONE    8. Set device limits
DONE    9. Reset device limits
DONE    10. Set device limits and check list
DONE    11. Set device limits error (set limits to  disarmed devices)

*/

namespace HubPlatform {
namespace Tests {

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

class HubModesSwingerShutdownTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    bool isMarkedDeviceAsDeleted( const std::string& deviceId, const std::string& dbName );
    JsonObject addDisableDevice( const std::string& deviceId );
    JsonObject removeDisableDevice( const std::string& deviceId );
    JsonObject setLimits( const std::string& deviceId, int hits, int inactivityWindow );
    JsonObject resetLimits( const std::string& deviceId );
    void addDevice( const std::string& deviceId );
};

void HubModesSwingerShutdownTests::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void HubModesSwingerShutdownTests::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );
    CommonNmaTests::TearDown();
}

bool HubModesSwingerShutdownTests::isMarkedDeviceAsDeleted( const std::string& deviceId, const std::string& dbName ) {
    auto record = nma().getDBClient().findOne( dbName, JSON( "_id" << deviceId ), nullptr );
    return json_get_bool( record, "deleted" );
}

JsonObject HubModesSwingerShutdownTests::addDisableDevice( const std::string& deviceId ) {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.swinger.shutdown.disable.add" }, { "params", JSON( "deviceId" << deviceId ) } } );

    return answer;
}

JsonObject HubModesSwingerShutdownTests::removeDisableDevice( const std::string& deviceId ) {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.swinger.shutdown.disable.remove" }, { "params", JSON( "deviceId" << deviceId ) } } );

    return answer;
}

JsonObject HubModesSwingerShutdownTests::setLimits( const std::string& deviceId, int hits, int inactivityWindow ) {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.swinger.shutdown.limits.set" },
          { "params",
            JSON( "deviceId" << deviceId << "hitsLimit" << hits << "inactivityWindow" << inactivityWindow ) } } );

    return answer;
}

JsonObject HubModesSwingerShutdownTests::resetLimits( const std::string& deviceId ) {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.swinger.shutdown.limits.reset" }, { "params", JSON( "deviceId" << deviceId ) } } );

    return answer;
}

void HubModesSwingerShutdownTests::addDevice( const std::string& deviceId ) {
    auto& dbClient = nma().getDBClient();
    Json query;
    query[ db::ID_FIELD_NAME ] = deviceId;
    query[ db::ID_FIELD_NAME ] = deviceId;
    query[ "hits" ] = 0;
    query[ "added" ] = 1;
    query[ "updated" ] = 0;
    query[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownDevices, query, true );
    dbClient.insert( TempFirmwareCoredSwingerShutdownDevices, query, true );
}

TEST_F( HubModesSwingerShutdownTests, checkMarkAsDeletedSwingerShutdownRecordesAfterDeviceRemoving ) {  // 1
    auto& dbClient = nma().getDBClient();
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto deviceId1 = device1.m_deviceId;

    auto device2 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto deviceId2 = device2.m_deviceId;

    Json query;
    query[ db::ID_FIELD_NAME ] = deviceId1;
    query[ "hits" ] = 0;
    query[ "added" ] = 1;
    query[ "updated" ] = 0;
    query[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownDevices, query, true );
    dbClient.insert( TempFirmwareCoredSwingerShutdownDevices, query, true );

    makeRemote().removeDevice( device1.m_deviceId );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    sleep( 1 );

    EXPECT_TRUE( isMarkedDeviceAsDeleted( deviceId1, FirmwareCoredSwingerShutdownDevices ) );
    EXPECT_TRUE( isMarkedDeviceAsDeleted( deviceId1, TempFirmwareCoredSwingerShutdownDevices ) );
}

TEST_F( HubModesSwingerShutdownTests, checkMarkAsDeletedSwingerShutdownRecordesAfterItemUpdating ) {  // 2
    auto& dbClient = nma().getDBClient();
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto deviceId1 = device1.m_deviceId;
    auto device2 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto deviceId2 = device2.m_deviceId;

    Json query;
    query[ db::ID_FIELD_NAME ] = deviceId1;
    query[ "hits" ] = 0;
    query[ "added" ] = 1;
    query[ "updated" ] = 0;
    query[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownDevices, query, true );
    dbClient.insert( TempFirmwareCoredSwingerShutdownDevices, query, true );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    const CoreD::Model::ValueType& valueType = CoreD::Model::ValueType::Enum::Bool;
    CoreD::Model::Item newItem( deviceId1,
                                "newItemName",
                                valueType,
                                true,
                                false,
                                CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );

    ASSERT_TRUE( remoteInterface.modifyItem( device1.m_itemId, newItem ) );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

    CoreD::Model::Item newItem2( deviceId2,
                                 "newItemName",
                                 valueType,
                                 true,
                                 false,
                                 CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );

    sleep( 1 );

    EXPECT_TRUE( isMarkedDeviceAsDeleted( deviceId1, FirmwareCoredSwingerShutdownDevices ) );
    EXPECT_TRUE( isMarkedDeviceAsDeleted( deviceId1, TempFirmwareCoredSwingerShutdownDevices ) );
}

TEST_F( HubModesSwingerShutdownTests, checkMarkAsDeletedSwingerShutdownRecordesAfterItemRemoving ) {  // 3
    auto& dbClient = nma().getDBClient();
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto deviceId1 = device1.m_deviceId;
    auto device2 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto deviceId2 = device2.m_deviceId;

    Json query;
    query[ db::ID_FIELD_NAME ] = deviceId1;
    query[ "hits" ] = 0;
    query[ "added" ] = 1;
    query[ "updated" ] = 0;
    query[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownDevices, query, true );
    dbClient.insert( TempFirmwareCoredSwingerShutdownDevices, query, true );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    remoteInterface.removeItem( device1.m_itemId );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

    sleep( 1 );

    EXPECT_TRUE( isMarkedDeviceAsDeleted( deviceId1, FirmwareCoredSwingerShutdownDevices ) );
    EXPECT_TRUE( isMarkedDeviceAsDeleted( deviceId1, TempFirmwareCoredSwingerShutdownDevices ) );
}

TEST_F( HubModesSwingerShutdownTests, checkHouseModesGetGlobalLimits ) {  // 4
    auto& dbClient = nma().getDBClient();
    const int HitsLimit = 2;
    const int InactivityWindow = 8;
    JsonObject defaults =
        JSON( "devices" << JSON_EMPTY_ARR << "defaults" << JSON( "hitsLimit" << 2 << "inactivityWindow" << 8 ) );
    dbClient.clear( HouseModesSwingerShutdown );
    dbClient.insert( HouseModesSwingerShutdown, defaults, true );
    auto res = dbClient.findAll( HouseModesSwingerShutdown, JSON_EMPTY_OBJ, nullptr );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "api", "2.0" } } );
    auto swingerShutdownLimits = answer[ "result" ][ "swingerShutdown" ];
    if( !swingerShutdownLimits.empty() ) {
        JsonObject global;
        EXPECT_TRUE( json_get_obj( swingerShutdownLimits, "global", global ) );
        EXPECT_EQ( json_get_int( global, "hitsLimit" ), HitsLimit );
        EXPECT_EQ( json_get_int( global, "inactivityWindow" ), InactivityWindow );
    }
}

TEST_F( HubModesSwingerShutdownTests, getSwingerShutdownList ) {  // 5
    auto& dbClient = nma().getDBClient();

    Json query;
    query[ db::ID_FIELD_NAME ] = "deviceId1";
    query[ "hits" ] = 0;
    query[ "added" ] = 1;
    query[ "updated" ] = 0;
    query[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownDevices, query, true );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.swinger.shutdown.list" }, { "api", "1.0" } } );

    JsonObject result;
    json_get_obj( answer, "result", result );
    JsonArray devices;
    EXPECT_TRUE( json_get_arr( result, "devices", devices ) );
    JsonArray disabledDevices;
    EXPECT_TRUE( json_get_arr( result, "disabledDevices", disabledDevices ) );
    EXPECT_TRUE( !json_get_str( result, "version" ).empty() );
}

TEST_F( HubModesSwingerShutdownTests, resetSwingerShutdown ) {  // 5
    auto& dbClient = nma().getDBClient();
    const std::string DeviceId1 = "deviceId1";
    Json query;
    query[ db::ID_FIELD_NAME ] = DeviceId1;
    query[ "hits" ] = 2;
    query[ "added" ] = 1;
    query[ "updated" ] = 0;
    query[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownDevices, query, true );
    dbClient.insert( TempFirmwareCoredSwingerShutdownDevices, query, true );

    Json querySeries;
    querySeries[ db::ID_FIELD_NAME ] = "Id1";
    querySeries[ "DeviceId" ] = DeviceId1;
    querySeries[ "hits" ] = 2;
    querySeries[ "begin" ] = 1;
    querySeries[ "updated" ] = 0;
    querySeries[ "deleted" ] = false;

    dbClient.insert( FirmwareCoredSwingerShutdownTimeSeries, querySeries, true );
    dbClient.insert( TempFirmwareCoredSwingerShutdownTimeSeries, querySeries, true );
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.swinger.shutdown.reset" }, { "params", JSON( "deviceId" << DeviceId1 ) } } );

    JsonObject result;
    json_get_obj( answer, "result", result );

    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.reset.done" );

    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), DeviceId1 );

    auto records = dbClient.findAll( FirmwareCoredSwingerShutdownDevices, JSON_EMPTY_OBJ, nullptr );
    EXPECT_TRUE( records.size() > 0 );
    auto record = records.front();
    EXPECT_EQ( json_get_str( record, "_id" ), DeviceId1 );
}

TEST_F( HubModesSwingerShutdownTests, addAndRemoveDisabledDevices ) {  // 6
    auto& dbClient = nma().getDBClient();
    const std::string DeviceId1 = "deviceId1";

    auto answer = addDisableDevice( DeviceId1 );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.disable.added" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.swinger.shutdown.disable.added" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), DeviceId1 );

    auto records = dbClient.findAll( HouseModesSwingerShutdown, JSON_EMPTY_OBJ, nullptr );
    EXPECT_TRUE( records.size() > 0 );
    auto record = records.front();
    JsonArray disabledDevicesArray;
    EXPECT_TRUE( json_get_arr( record, "disabledDevices", disabledDevicesArray ) );
    EXPECT_EQ( disabledDevicesArray.size(), 1 );
    EXPECT_EQ( disabledDevicesArray.front(), DeviceId1 );

    answer = removeDisableDevice( DeviceId1 );
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    records = dbClient.findAll( HouseModesSwingerShutdown, JSON_EMPTY_OBJ, nullptr );
    EXPECT_TRUE( records.size() > 0 );
    record = records.front();
    JsonArray disabledDevicesArrayAfterDelete;
    EXPECT_TRUE( json_get_arr( record, "disabledDevices", disabledDevicesArrayAfterDelete ) );
    EXPECT_EQ( disabledDevicesArrayAfterDelete.size(), 0 );

    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.disable.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.swinger.shutdown.disable.removed" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), DeviceId1 );
}

TEST_F( HubModesSwingerShutdownTests, addAndRemoveDisabledDevicesErrors ) {  // 7
    auto& dbClient = nma().getDBClient();
    const std::string DeviceId1 = "deviceId1";
    auto answer = addDisableDevice( DeviceId1 );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.disable.added" );
    answer = addDisableDevice( DeviceId1 );
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    JsonObject error;
    EXPECT_TRUE( json_get_obj( result, "error", error ) );
    ASSERT_EQ( json_get_int( error, "code", 0 ), static_cast< int >( Errors::Codes::INVALID_PARAMS ) );
    ASSERT_EQ( json_get_str( error, "data" ), "ezlo.already.exists" );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );

    answer = removeDisableDevice( DeviceId1 );
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.disable.removed" );
    answer = removeDisableDevice( DeviceId1 );
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( json_get_obj( result, "error", error ) );
    ASSERT_LE( json_get_int( error, "code", 0 ), static_cast< int >( Errors::Codes::INVALID_PARAMS ) );
    ASSERT_LE( json_get_str( error, "data" ), "ezlo.not.exists" );
    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
}

TEST_F( HubModesSwingerShutdownTests, setDeviceLimits ) {  // 8
    auto& dbClient = nma().getDBClient();
    const std::string DeviceId1 = "deviceId1";
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );

    auto answer = setLimits( device1.m_deviceId, 2, 3 );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), device1.m_deviceId );
}

TEST_F( HubModesSwingerShutdownTests, resetDeviceLimits ) {  // 9
    auto& dbClient = nma().getDBClient();
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );
    auto answer = setLimits( device1.m_deviceId, 2, 3 );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.swinger.shutdown.limits.changed" );

    answer = resetLimits( device1.m_deviceId );
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), device1.m_deviceId );
}

TEST_F( HubModesSwingerShutdownTests, setDeviceLimitsAndCheckList ) {  // 10
    auto& dbClient = nma().getDBClient();
    auto device1 = addDeviceWithSecurityThreatItem( __LINE__ );
    const int HitsLimit = 10;
    const int InactivityWindow = 3;
    auto answer = setLimits( device1.m_deviceId, 10, 3 );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( result.empty() );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.swinger.shutdown.limits.changed" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, "deviceId" ), device1.m_deviceId );
    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.swinger.shutdown.list" }, { "api", "1.0" } } );
    json_get_obj( answer, "result", result );
    JsonArray devices;
    EXPECT_TRUE( json_get_arr( result, "devices", devices ) );

    for( auto& device : devices ) {
        if( json_get_str( device, "deviceId" ) == device1.m_deviceId ) {
            Json limits;
            EXPECT_TRUE( json_get_obj( device, "limits", limits ) );
            EXPECT_EQ( json_get_int( limits, "hitsLimit" ), HitsLimit );
            EXPECT_EQ( json_get_int( limits, "inactivityWindow" ), InactivityWindow );
        }
    }
}

TEST_F( HubModesSwingerShutdownTests, setDeviceLimitsError ) {  // 11
    auto& dbClient = nma().getDBClient();
    const std::string DeviceId1 = "deviceId1";
    addDevice( DeviceId1 );

    auto answer = setLimits( DeviceId1, 2, 3 );
    JsonObject result;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    JsonObject error;
    EXPECT_TRUE( json_get_obj( result, "error", error ) );
    ASSERT_EQ( json_get_int( error, "code", 0 ), static_cast< int >( Errors::Codes::INVALID_PARAMS ) );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    ASSERT_EQ( json_get_str( error, "message" ), "Wrong params" );
    ASSERT_EQ( json_get_str( error, "reason" ), "No armed device by this ID" );

    checkNoBroadcastReceived( "", __LINE__, std::chrono::seconds( 5 ) );
}

}  // namespace Tests
}  // namespace HubPlatform
