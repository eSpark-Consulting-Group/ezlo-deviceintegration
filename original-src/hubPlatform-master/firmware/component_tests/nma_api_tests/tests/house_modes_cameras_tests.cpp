#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "nma_api_tests/tests/fixtures/house_mode_fixture.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1 Cameras in House Modes
DONE    1.0 Check hub.modes.cameras_off.added/removed broadcasts
DONE    1.1 Modify a make_recording item: change item name
DONE    1.2 Remove a camera device
DONE    1.3 Remove a make_recording item
DONE    1.4 Add a camera device with house_modes_options
DONE    1.5 Modify a camera device with house_modes_options
DONE    1.6 DisarmedDefault=true for cameras
DONE    1.7 cameras_off bad params checking
DONE    1.8 alarms_off bad params checking

*/

class CamerasHouseModeTests : public TestsHouseModeFixture {
public:
    void SetUp() override;
    void TearDown() override;
    void createCameraDevice( int codeLine, Json houseModesOptions );

    void checkModesCamerasOffAdded( const std::string& houseModeId,
                                    const std::string& deviceId,
                                    const std::string& message );
    void checkModesCamerasOffRemoved( const std::string& houseModeId,
                                      const std::string& deviceId,
                                      const std::string& message );

    void camerasOffAdd( const std::string& modeId, const std::string& deviceId, const std::string& message );

    void camerasOffRemove( const std::string& modeId, const std::string& deviceId, const std::string& message );
    JsonArray fetchCameras( const Json& answer );
    JsonArray fetchModeCamerasOff( const Json& answer, std::string modeId );
    void commandWithError( const std::string& command,
                           const std::string& modeId,
                           const std::string& deviceId,
                           const Json& expectedError,
                           const std::string& message );

protected:
    std::string m_deviceId;
    std::string m_makeRecordingItemId;
    std::unique_ptr< CoreD::Model::Device > m_deviceUPtr;
};

using namespace CoreDHouseModes::Resources;
using namespace CoreD::Model::Resources;

void CamerasHouseModeTests::SetUp() {
    TestsHouseModeFixture::SetUp();
}

void CamerasHouseModeTests::TearDown() {
    clearExistingDevices( __LINE__ );
    TestsHouseModeFixture::TearDown();
    m_deviceId.clear();
    m_makeRecordingItemId.clear();
    m_deviceUPtr.reset();
}

void CamerasHouseModeTests::createCameraDevice( int codeLine, Json houseModesOptions ) {
    CoreDRemoteInterface::CoreDRemoteInterface remote( nma() );
    m_deviceUPtr = std::make_unique< CoreD::Model::Device >( createDevice(
        remote, "Generic camera device", "test_camera", "camera.indoor", "camera", "indoor_cam", false, codeLine ) );
    if( houseModesOptions.is_object() && !houseModesOptions.empty() ) {
        m_deviceUPtr->setHouseModesOptions( std::move( houseModesOptions ) );
    }

    m_deviceId = addDevice( remote, *m_deviceUPtr, codeLine );
    m_makeRecordingItemId =
        addItem( remote, m_deviceId, "make_recording", CoreD::Model::ValueType::Enum::Int, false, true, codeLine );
}

void CamerasHouseModeTests::checkModesCamerasOffAdded( const std::string& houseModeId,
                                                       const std::string& deviceId,
                                                       const std::string& message ) {
    auto broadcast = waitForBroadcast( "hub.modes.cameras_off.added", message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.cameras_off.added" ) << message;
    if( !houseModeId.empty() )
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), houseModeId ) << message;
    if( !deviceId.empty() )
        EXPECT_EQ( json_get_str( broadcast[ "result" ], BroadcastFields::CamerasOffDeviceFieldName ), deviceId )
            << message;
}

void CamerasHouseModeTests::checkModesCamerasOffRemoved( const std::string& houseModeId,
                                                         const std::string& deviceId,
                                                         const std::string& message ) {
    auto broadcast = waitForBroadcast( "hub.modes.cameras_off.removed", message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.cameras_off.removed" ) << message;
    if( !houseModeId.empty() )
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), houseModeId ) << message;
    if( !deviceId.empty() )
        EXPECT_EQ( json_get_str( broadcast[ "result" ], BroadcastFields::CamerasOffDeviceFieldName ), deviceId )
            << message;
}

void CamerasHouseModeTests::camerasOffAdd( const std::string& modeId,
                                           const std::string& deviceId,
                                           const std::string& message ) {
    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.cameras_off.add" },
                                       { "params", JSON( "modeId" << modeId << "deviceId" << m_deviceId ) } } );

    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    EXPECT_TRUE( answer[ "error" ].empty() ) << message << ", error: " << answer[ "error" ].dump();
}

void CamerasHouseModeTests::camerasOffRemove( const std::string& modeId,
                                              const std::string& deviceId,
                                              const std::string& message ) {
    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.cameras_off.remove" },
                                       { "params", JSON( "modeId" << modeId << "deviceId" << m_deviceId ) } } );

    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    EXPECT_TRUE( answer[ "error" ].empty() ) << message << ", error: " << answer[ "error" ].dump();
}

JsonArray CamerasHouseModeTests::fetchCameras( const Json& answer ) {
    return fetchCollection( answer, "cameras" );
}

JsonArray CamerasHouseModeTests::fetchModeCamerasOff( const Json& answer, std::string modeId ) {
    Json modes;
    if( json_get( answer, { "result", "modes" }, modes ) ) {
        auto foundIt = find_if( begin( modes ), end( modes ), [&modeId]( const Json& mode ) {
            return modeId == json_get_str( mode, "_id" );
        } );
        JsonArray cameras;
        if( foundIt != end( modes ) && json_get_arr( *foundIt, "cameras", cameras ) )
            return cameras;
    }
    return JsonArray();
}

void CamerasHouseModeTests::commandWithError( const std::string& command,
                                              const std::string& modeId,
                                              const std::string& deviceId,
                                              const Json& expectedError,
                                              const std::string& message ) {
    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", command }, { "params", JSON( "modeId" << modeId << "deviceId" << deviceId ) } } );

    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    Json error;
    EXPECT_TRUE( json_get( answer, "error", error ) ) << message;
    EXPECT_FALSE( error.empty() ) << message << ", error: " << error.dump() << message;
    EXPECT_EQ( error[ "code" ], json_get_int( expectedError, "code", -1 ) ) << message;
    EXPECT_EQ( error[ "data" ], json_get_str( expectedError, "data" ) ) << message;
    EXPECT_EQ( error[ "message" ], json_get_str( expectedError, "message" ) ) << message;
}

TEST_F( CamerasHouseModeTests, checkBroadcastsCamerasOff /*1.0*/ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    createCameraDevice( __LINE__, nullptr );

    auto checkEmptyCamerasOff = [houseModeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        if( json_get_str( mode, "_id" ) == houseModeId ) {
            JsonArray deviceIds;
            EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
            EXPECT_TRUE( deviceIds.empty() ) << deviceIds.dump() << errorMessage;
        }
    };
    checkHouseModesGet( checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), nullptr );

    {
        camerasOffAdd( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesCamerasOffAdded( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
    }

    Json modesGetAnswer;
    checkHouseModesGet(
        [this, houseModeId]( const Json& mode, const std::string& errorMessage ) {
            LOGGER_TRACE_RAW( "checkHouseModesGet: after cameras_off add houseMode: " + cc::j( mode ) );
            if( json_get_str( mode, "_id" ) == houseModeId ) {
                JsonArray deviceIds;
                EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
                EXPECT_EQ( deviceIds.size(), 1 ) << errorMessage;
                EXPECT_EQ( deviceIds.at( 0 ), m_deviceId ) << errorMessage;
            }
        },
        NMA_TEST_MAKE_MESSAGE( "Should have camerasOff array filled with the deviceId " + m_deviceId ),
        &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";

    // hub.modes.cameras_off.removed
    {
        camerasOffRemove( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesChangedDisarmedDefault( false, NMA_TEST_MAKE_MESSAGE( "disarmedDefault should change to false" ) );
        checkModesCamerasOffRemoved( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
    }

    checkHouseModesGet( checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), nullptr );

    removeDevice( coreRemoteInterface, m_deviceId, 1, 0, __LINE__ );

    checkHouseModesGet(
        checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), &modesGetAnswer );
    EXPECT_FALSE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) )
        << "cameras array should be empty again after device is removed";
}

TEST_F( CamerasHouseModeTests, camerasModifyItemName ) /* 1.1 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    createCameraDevice( __LINE__, nullptr );

    auto checkEmptyCamerasOff = [houseModeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        if( json_get_str( mode, "_id" ) == houseModeId ) {
            JsonArray deviceIds;
            EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
            EXPECT_TRUE( deviceIds.empty() ) << deviceIds.dump() << errorMessage;
        }
    };
    Json modesGetAnswer;
    checkHouseModesGet(
        checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";

    {
        camerasOffAdd( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesCamerasOffAdded( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesChangedDisarmedDefault( false, NMA_TEST_MAKE_MESSAGE( "disarmedDefault should change to false" ) );
    }

    checkHouseModesGet(
        [this, houseModeId]( const Json& mode, const std::string& errorMessage ) {
            LOGGER_TRACE_RAW( "checkHouseModesGet: after cameras_off add houseMode: " + cc::j( mode ) );
            if( json_get_str( mode, "_id" ) == houseModeId ) {
                JsonArray deviceIds;
                EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
                EXPECT_EQ( deviceIds.size(), 1 ) << errorMessage;
                EXPECT_EQ( deviceIds.at( 0 ), m_deviceId ) << errorMessage;
            }
        },
        NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ),
        &modesGetAnswer );

    auto valueType = CoreD::Model::ValueType::Enum::Int;
    CoreD::Model::Item newItem( m_deviceId,
                                "another_name",
                                valueType,
                                false,
                                true,
                                CoreD::Model::ValueFactory::createDefaultItemValue( valueType ) );
    coreRemoteInterface.modifyItem( m_makeRecordingItemId, newItem );

    Json broadcast;

    auto checkers = CommonNmaTests::BroadcastCheckers{
        { "hub.item.updated",
          []( const Json& broadcast, int line ) {
              EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" ) << line;
          } },
        { "hub.modes.cameras_off.removed",
          []( const Json& broadcast, int line ) {
              EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.cameras_off.removed" ) << line;
          } }
    };

    checkBroadcasts( checkers, __LINE__ );

    checkHouseModesGet(
        checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no cameras devices" ), &modesGetAnswer );

    EXPECT_FALSE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should not be filled";
}

TEST_F( CamerasHouseModeTests, camerasRemoveDevice ) /* 1.2 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    createCameraDevice( __LINE__, nullptr );

    auto checkEmptyCamerasOff = [houseModeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        if( json_get_str( mode, "_id" ) == houseModeId ) {
            JsonArray deviceIds;
            EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
            EXPECT_TRUE( deviceIds.empty() ) << deviceIds.dump() << errorMessage;
        }
    };
    Json modesGetAnswer;
    checkHouseModesGet(
        checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";

    {
        camerasOffAdd( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesCamerasOffAdded( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesChangedDisarmedDefault( false, NMA_TEST_MAKE_MESSAGE( "disarmedDefault=false" ) );
    }

    checkHouseModesGet(
        [this, houseModeId]( const Json& mode, const std::string& errorMessage ) {
            LOGGER_TRACE_RAW( "checkHouseModesGet: after cameras_off add houseMode: " + cc::j( mode ) );
            if( json_get_str( mode, "_id" ) == houseModeId ) {
                JsonArray deviceIds;
                EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
                EXPECT_EQ( deviceIds.size(), 1 ) << errorMessage;
                EXPECT_EQ( deviceIds.at( 0 ), m_deviceId ) << errorMessage;
            }
        },
        NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ),
        &modesGetAnswer );

    coreRemoteInterface.removeDevice( m_deviceId );

    Json broadcast;

    /** Broadcast "hub.modes.cameras_off.removed" should not be raised if the device is removed. It's like that for all
     * collection inside of a house mode: disarmed, alarmsOff, camerasOff. */
    auto checkers = CommonNmaTests::BroadcastCheckers{
        { "hub.item.removed",
          []( const Json& broadcast, int line ) {
              EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" ) << line;
          } },
        { "hub.device.removed",
          []( const Json& broadcast, int line ) {
              EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" ) << line;
          } },
    };

    checkBroadcasts( checkers, __LINE__ );

    checkHouseModesGet(
        checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no cameras devices" ), &modesGetAnswer );

    EXPECT_FALSE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should not be filled";
}

TEST_F( CamerasHouseModeTests, camerasRemoveMakeRecordingItem ) /* 1.3 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    const int expectedCamerasOff = 4;
    Json houseModesOptions = { { DeviceFields::HouseModesOptions::CamerasOffFieldName,
                                 { { houseModeId, true }, { "2", true }, { "3", true }, { "4", true } } } };
    createCameraDevice( __LINE__, houseModesOptions );

    auto checkAllModesCamerasOff = [houseModeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        JsonArray deviceIds;
        EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
        EXPECT_FALSE( deviceIds.empty() ) << deviceIds.dump() << errorMessage;
    };
    Json modesGetAnswer;
    Json broadcast;

    checkHouseModesGet(
        checkAllModesCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";

    coreRemoteInterface.removeItem( m_makeRecordingItemId );

    for( int i = 0; i < expectedCamerasOff; i++ ) {
        checkModesCamerasOffRemoved( "", m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesChangedDisarmedDefault( false, NMA_TEST_MAKE_MESSAGE( "" ) );
    }

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.removed" );
    LOGGER_TRACE_RAW( cc::j( broadcast ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.removed" );

    auto checkEmptyCamerasOff = []( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        JsonArray deviceIds;
        EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
        EXPECT_TRUE( deviceIds.empty() ) << deviceIds.dump() << errorMessage;
    };

    checkHouseModesGet(
        checkEmptyCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no cameras devices" ), &modesGetAnswer );

    EXPECT_FALSE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should not be filled";

    coreRemoteInterface.removeDevice( m_deviceId );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.removed" );
    LOGGER_TRACE_RAW( cc::j( broadcast ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( CamerasHouseModeTests, camerasAddDeviceWithHouseModeOptions ) /* 1.4 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    Json houseModesOptions = { { DeviceFields::HouseModesOptions::CamerasOffFieldName,
                                 { { houseModeId, true }, { "2", false }, { "3", false }, { "4", false } } } };
    createCameraDevice( __LINE__, houseModesOptions );

    auto checkCamerasOffInHomeModeOnly = [houseModeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        JsonArray deviceIds;
        EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
        const std::string& modeId = json_get_str( mode, "_id" );
        if( modeId == houseModeId ) {
            EXPECT_FALSE( deviceIds.empty() )
                << "modeId=" << modeId << ", camerasOffDevices=" << deviceIds.dump() << ", " << errorMessage;
        }
        else {
            EXPECT_TRUE( deviceIds.empty() )
                << "modeId=" << modeId << ", camerasOffDevices=" << deviceIds.dump() << ", " << errorMessage;
        }
    };
    Json modesGetAnswer;
    checkHouseModesGet( checkCamerasOffInHomeModeOnly,
                        NMA_TEST_MAKE_MESSAGE( "Should have camerasOff devices only in Home house mode" ),
                        &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";
}

TEST_F( CamerasHouseModeTests, camerasModifyDeviceWithHouseModeOptions ) /* 1.5 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    const std::string houseModeHomeId( "1" );

    // SetUp: unsure current house mode is Home (id=1), disarmedDefault=true for all house modes.
    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeHomeId != houseModeId ) {
        coreRemoteInterface.setHouseMode( "1" );
        checkHouseModeSwitched( NMA_TEST_MAKE_MESSAGE( "" ) );
    }

    Json houseModesOptions = { { DeviceFields::HouseModesOptions::CamerasOffFieldName,
                                 { { houseModeHomeId, true }, { "2", false }, { "3", false }, { "4", false } } } };
    createCameraDevice( __LINE__, houseModesOptions );

    auto checkCamerasOffInHomeModeOnly = [houseModeHomeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        JsonArray deviceIds;
        EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
        const std::string& modeId = json_get_str( mode, "_id" );
        if( modeId == houseModeHomeId ) {
            EXPECT_FALSE( deviceIds.empty() )
                << "modeId=" << modeId << ", camerasOffDevices=" << deviceIds.dump() << ", " << errorMessage;
        }
        else {
            EXPECT_TRUE( deviceIds.empty() )
                << "modeId=" << modeId << ", camerasOffDevices=" << deviceIds.dump() << ", " << errorMessage;
        }
    };
    Json modesGetAnswer;
    checkHouseModesGet( checkCamerasOffInHomeModeOnly,
                        NMA_TEST_MAKE_MESSAGE( "Should have camerasOff devices only in Home house mode" ),
                        &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";

    Json houseModesOptionsInverted = houseModesOptions;
    Json& camerasOff = houseModesOptionsInverted[ DeviceFields::HouseModesOptions::CamerasOffFieldName ];
    for( auto it = camerasOff.begin(); it != camerasOff.end(); it++ ) {
        auto modeId = it.key();
        bool value = it.value().get< bool >();
        json_add_bool( camerasOff, modeId, !value );
    }
    m_deviceUPtr->setHouseModesOptions( houseModesOptionsInverted );

    coreRemoteInterface.modifyDevice( m_deviceId, *m_deviceUPtr );
    checkModesCamerasOffRemoved( "", m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
    checkModesCamerasOffAdded( "", m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
    checkModesCamerasOffAdded( "", m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
    checkModesCamerasOffAdded( "", m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
}

TEST_F( CamerasHouseModeTests, camerasDisarmedDefault ) /* 6.6 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    const int expectedCamerasOff = 4;
    Json houseModesOptions = { { DeviceFields::HouseModesOptions::CamerasOffFieldName,
                                 { { houseModeId, true }, { "2", true }, { "3", true }, { "4", true } } } };
    createCameraDevice( __LINE__, houseModesOptions );

    auto checkAllModesCamerasOff = [houseModeId]( const Json& mode, const std::string& errorMessage ) {
        LOGGER_TRACE_RAW( "checkHouseModesGet: houseMode: " + cc::j( mode ) );
        JsonArray deviceIds;
        EXPECT_TRUE( json_get_arr( mode, "camerasOffDevices", deviceIds ) ) << errorMessage;
        EXPECT_FALSE( deviceIds.empty() ) << deviceIds.dump() << errorMessage;
    };
    Json modesGetAnswer;
    Json broadcast;

    checkHouseModesGet(
        checkAllModesCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ), &modesGetAnswer );

    EXPECT_TRUE( searchInArray( fetchCameras( modesGetAnswer ), m_deviceId ) ) << "cameras array should be filled";

    {
        camerasOffRemove( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );

        checkModesCamerasOffRemoved( houseModeId, m_deviceId, NMA_TEST_MAKE_MESSAGE( "" ) );
        checkModesChangedDisarmedDefault( false, NMA_TEST_MAKE_MESSAGE( "disarmedDefault should change to false" ) );
    }

    setDisarmedDefault( houseModeId, true, NMA_TEST_MAKE_MESSAGE( "" ) );
    checkHouseModesGet( checkAllModesCamerasOff, NMA_TEST_MAKE_MESSAGE( "Should have no camerasOff devices" ) );
}

namespace {
const auto houseModeId = "1", nonExistingModeId = "1234";

const Json modeIdError = { { "code", -32600 },
                           { "data", "rpc.params.empty.modeId" },
                           { "message", "Bad request, modeId is empty" } };
const Json deviceIdError = { { "code", -32600 },
                             { "data", "rpc.params.empty.deviceId" },
                             { "message", "Bad request, deviceId is empty" } };
const Json nonExistingModeIdError = { { "code", -32602 },
                                      { "data", "ezlo.house_mode.not.exist" },
                                      { "message", "The house mode with this id does not exist" } };
}  // namespace

TEST_F( CamerasHouseModeTests, camerasOffBadParams ) /* 1.7 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    createCameraDevice( __LINE__, {} );

    commandWithError(
        "hub.modes.cameras_off.add", "", m_deviceId, modeIdError, NMA_TEST_MAKE_MESSAGE( "empty modeId error check" ) );
    commandWithError( "hub.modes.cameras_off.add",
                      houseModeId,
                      "",
                      deviceIdError,
                      NMA_TEST_MAKE_MESSAGE( "empty deviceId error check" ) );
    commandWithError( "hub.modes.cameras_off.add",
                      nonExistingModeId,
                      m_deviceId,
                      nonExistingModeIdError,
                      NMA_TEST_MAKE_MESSAGE( "non-existing modeId error check" ) );

    commandWithError( "hub.modes.cameras_off.remove",
                      "",
                      m_deviceId,
                      modeIdError,
                      NMA_TEST_MAKE_MESSAGE( "empty modeId error check" ) );
    commandWithError( "hub.modes.cameras_off.remove",
                      houseModeId,
                      "",
                      deviceIdError,
                      NMA_TEST_MAKE_MESSAGE( "empty deviceId error check" ) );
    commandWithError( "hub.modes.cameras_off.remove",
                      nonExistingModeId,
                      m_deviceId,
                      nonExistingModeIdError,
                      NMA_TEST_MAKE_MESSAGE( "non-existing modeId error check" ) );
}

TEST_F( CamerasHouseModeTests, alarmsOffBadParams ) /* 1.8 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    createCameraDevice( __LINE__, {} );

    commandWithError(
        "hub.modes.alarms_off.add", "", m_deviceId, modeIdError, NMA_TEST_MAKE_MESSAGE( "empty modeId error check" ) );
    commandWithError( "hub.modes.alarms_off.add",
                      houseModeId,
                      "",
                      deviceIdError,
                      NMA_TEST_MAKE_MESSAGE( "empty deviceId error check" ) );
    commandWithError( "hub.modes.alarms_off.add",
                      nonExistingModeId,
                      m_deviceId,
                      nonExistingModeIdError,
                      NMA_TEST_MAKE_MESSAGE( "non-existing modeId error check" ) );

    commandWithError( "hub.modes.alarms_off.remove",
                      "",
                      m_deviceId,
                      modeIdError,
                      NMA_TEST_MAKE_MESSAGE( "empty modeId error check" ) );
    commandWithError( "hub.modes.alarms_off.remove",
                      houseModeId,
                      "",
                      deviceIdError,
                      NMA_TEST_MAKE_MESSAGE( "empty deviceId error check" ) );
    commandWithError( "hub.modes.alarms_off.remove",
                      nonExistingModeId,
                      m_deviceId,
                      nonExistingModeIdError,
                      NMA_TEST_MAKE_MESSAGE( "non-existing modeId error check" ) );
}

}  // namespace Tests
}  // namespace HubPlatform
