#include "hardware_test/hardware_tests/zigbee_test.h"

#ifndef __ZIGBEE_TEST_SUPPORTED__
#error Macro __ZIGBEE_TEST_SUPPORTED__ should be defined in Makefile
#endif

#if __ZIGBEE_TEST_SUPPORTED__

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <exception>
#include <thread>
#include <chrono>
#include <sys/stat.h>

#include "json_utils/json_utils.h"
#include "base_utils/timeutils.h"
#include "base_utils/zigbee/zigbee_environment.h"
#include "base_utils/utils.h"

#include "hardware_test/utils/test_log.h"

extern "C" {
#include <Zigbee/definitions.h>
#include <app/builder/zigbeegw/znet-bookkeeping.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
#include <protocol/zigbee/app/framework/plugin/network-creator/network-creator.h>
#include <protocol/zigbee/app/framework/util/attribute-storage.h>
#include <protocol/zigbee/app/framework/util/af-main.h>
#include <protocol/zigbee/app/util/serial/linux-serial.h>
#include <protocol/zigbee/app/util/ezsp/ezsp.h>
#include <protocol/zigbee/app/util/ezsp/ezsp-enum.h>
#include <protocol/zigbee/stack/include/error-def.h>
#include <protocol/zigbee/app/framework/plugin/device-database/device-database.h>
#include <protocol/zigbee/app/framework/plugin/device-table/device-table.h>
#include <protocol/zigbee/app/framework/security/crypto-state.h>
#include <protocol/zigbee/stack/include/ember-types.h>

#include <platform/base/phy/ieee802154mac.h>
}

#define TEST_RADIO_CHANNEL 25
#define FRAMES_TO_SEND 5

extern const char* __progname;

extern "C" void emAfResetAndInitNCP();
extern "C" void ezspChildJoinHandler( int8u, boolean, EmberNodeId, EmberEUI64, EmberNodeType ){ /* stub */ };
extern "C" void hubPlatformPollControlCheckInCallback( uint16_t, uint8_t, uint16_t ){ /* stub */ };
extern "C" bool emberAfStackStatusCallback( EmberStatus status ) {
    return false;
}

namespace HubPlatform {
namespace HardwareTest {

#define TEST_NAME "Zigbee"
constexpr std::chrono::seconds TEST_TIMEOUT{ 80 };

namespace {
TestLog testLog( TEST_NAME );

using Buffer = std::vector< uint8_t >;

void emberZNetStackTick() {
    halResetWatchdog();
    ezspTick();
    while( ezspCallbackPending() )
        ezspCallback();

    if( emberAfNcpNeedsReset() )
        emAfResetAndInitNCP();

    if( emAfIsCryptoOperationInProgress() )
        return;

    emAfTick();
    emberSerialBufferTick();
    emberAfRunEvents();
    emAfAssertNetworkIndexStackIsEmpty();
}

void initZigbeeFramework() {
    const char* device = "/dev/zbstick";
    const std::vector< const char* > argv = { __progname, "-b", "115200", "-f", "x", "-p", device };
    std::stringstream err_msg;

    struct stat st;
    if( stat( device, &st ) == -1 ) {
        err_msg << "Zigbee device: " << device << " not found!" << std::endl;
        throw std::runtime_error( err_msg.str() );
    }
    testLog() << "Zigbee device: " << device << " found!";

    Environment::Zigbee::bootInit();
    Environment::Zigbee::resetInit();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    Environment::Zigbee::resetCycle();
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    testLog() << "Zigbee stack initialization started";
    // NOTE: See ZigbeeStackWrapper::init(...) or emberAfMain(...) for details

    halInit();
    INTERRUPTS_ON();
    int returnCode = EMBER_SUCCESS;
    if( emberAfMainStartCallback( &returnCode, argv.size(), const_cast< char** >( argv.data() ) ) ) {
        err_msg << "Failed emberAfMainStartCallback(): returnCode = " << returnCode;
        throw std::runtime_error( err_msg.str() );
    }

    auto serialInitStatus = emberSerialInit( APP_SERIAL, BAUD_115200, PARITY_NONE, 1 );
    if( EMBER_SUCCESS != serialInitStatus ) {
        err_msg << "Failed emberSerialInit(): returnCode = " << serialInitStatus;
        throw std::runtime_error( err_msg.str() );
    }

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    emberAfAppPrintln( "Reset info: %d (%p)", halGetResetInfo(), halGetResetString() );
    emberAfCoreFlush();
    emAfInitializeNetworkIndexStack();
    emberAfEndpointConfigure();
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    emAfResetAndInitNCP();
    emAfInit();
    emAfNetworkInit();

    if( emberNetworkState() == EMBER_NO_NETWORK ) {
        auto networkConfigured = [] {
            emberZNetStackTick();
            return emberNetworkState() != EMBER_NO_NETWORK;
        };

        testLog() << "Zigbee network is not configured. Configure network...";
        emberAfPluginNetworkCreatorStart( true );
        if( !waitForCondition( networkConfigured, std::chrono::seconds( 15 ), std::chrono::milliseconds( 50 ) ) )
            throw std::runtime_error( "Zigbee network configuration timeout" );
        testLog() << "Zigbee network configuration finished";
    }

    testLog() << "Zigbee framework initialization success";
};

void zigbeeFrameworkCleanup() {
    Environment::Zigbee::resetAssert();
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    emberSerialCleanup();
    std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );
}

void resetZigbeeNetwork() {
    testLog() << "Resetting zigbee network...";

    emberClearBindingTable();
    emberAfClearReportTableCallback();
    for( uint8_t endpointIndex = 0; endpointIndex < emberAfEndpointCount(); endpointIndex++ ) {
        uint8_t endpoint = emberAfEndpointFromIndex( endpointIndex );
        emberAfResetAttributes( endpoint );
        emberAfGroupsClusterClearGroupTableCallback( endpoint );
        emberAfScenesClusterClearSceneTableCallback( endpoint );
    }

    for( uint16_t index = 0; index < EMBER_AF_PLUGIN_DEVICE_DATABASE_MAX_DEVICES; ++index ) {
        auto pDeviceInfo = emberAfPluginDeviceDatabaseGetDeviceByIndex( index );
        if( pDeviceInfo ) {
            emberAfPluginDeviceDatabaseEraseDevice( (uint8_t*) pDeviceInfo->eui64 );
        }
    }
    emberAfDeviceTableClear();
    emberLeaveNetwork();

    auto networkNotPresent = [] { return emberNetworkState() == EMBER_NO_NETWORK; };

    if( !waitForCondition( networkNotPresent, std::chrono::seconds( 15 ), std::chrono::milliseconds( 50 ) ) )
        throw std::runtime_error( "Zigbee network reset timeout" );

    testLog() << "Zigbee network reset finished";
}

void setupRadioChannel() {
    std::stringstream err_msg;

    auto channelStatus = ezspSetRadioChannel( TEST_RADIO_CHANNEL );
    if( channelStatus != EMBER_SUCCESS ) {
        err_msg << "Radio channel setup failed with status: " << channelStatus << std::endl;
        throw std::runtime_error( err_msg.str() );
    }

    size_t radioChannel = emberAfGetRadioChannel();
    if( TEST_RADIO_CHANNEL != radioChannel ) {
        err_msg << "Radio channel setup failed: expected channel - " << TEST_RADIO_CHANNEL;
        err_msg << " : current channel - " << radioChannel << std::endl;
        throw std::runtime_error( err_msg.str() );
    }
    testLog() << "Zigbee radio channel set to: " << radioChannel;
};

Buffer getMac() {
    Buffer buffer( 8, 0 );
    emberAfGetEui64( buffer.data() );
    if( std::all_of( buffer.begin(), buffer.end(), []( int i ) { return i == 0; } ) )
        throw std::runtime_error( "Zigbee MAC is undefined" );
    return buffer;
};

std::string getMacString( const Buffer& mac ) {
    assert( mac.size() == 8 );
    std::stringstream ss;
    ss << std::hex;

    auto it = mac.crbegin();
    ss << std::setw( 2 ) << std::setfill( '0' ) << (int) *it++;
    for( ; it != mac.crend(); ++it )
        ss << ":" << std::setw( 2 ) << std::setfill( '0' ) << (int) *it;
    return ss.str();
}

size_t getPanId() {
    size_t panId = emberAfGetPanId();
    if( !panId )
        throw std::runtime_error( "Zigbee panId is undefined" );
    return panId;
}

std::string getPanIdString( const size_t panId ) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw( 4 ) << std::setfill( '0' ) << panId;
    return ss.str();
}

// Create IEEE 802.15.4 compatible MAC frame
// +---------------------------------------------------+
// | Frame Control | Seq. Number | Src. PAN | Src. MAC |
// +---------------------------------------------------+
Buffer createMacFrame( size_t panId, Buffer& mac ) {
    constexpr size_t FRAME_CONTROL_SIZE = 2;
    constexpr size_t SEQ_NUM_SIZE = 1;
    constexpr size_t SRC_PAN_SIZE = 2;
    constexpr size_t SRC_MAC_SIZE = 8;
    constexpr size_t FRAME_SIZE = FRAME_CONTROL_SIZE + SEQ_NUM_SIZE + SRC_PAN_SIZE + SRC_MAC_SIZE;
    assert( mac.size() == SRC_MAC_SIZE );

    Buffer macFrame( FRAME_SIZE, 0 );
    size_t index = 0;

    uint16_t frameControl = 0;
    frameControl |= MAC_FRAME_TYPE_DATA;
    frameControl |= MAC_FRAME_VERSION_2006;
    frameControl |= MAC_FRAME_SOURCE_MODE_LONG;
    memcpy( &macFrame[ index ], &frameControl, FRAME_CONTROL_SIZE );
    index += FRAME_CONTROL_SIZE;

    // NOTE: Sequence number is automatically set by Ember API
    index += SEQ_NUM_SIZE;

    memcpy( &macFrame[ index ], &panId, SRC_PAN_SIZE );
    index += SRC_PAN_SIZE;

    memcpy( &macFrame[ index ], mac.data(), SRC_MAC_SIZE );
    index += SRC_MAC_SIZE;

    return macFrame;
};

void sendMacFrames( Buffer& macFrame ) {
    testLog() << "Send MAC Frames: ";
    for( size_t i = 1; i <= FRAMES_TO_SEND; ++i ) {
        testLog() << "Frame-" << i;
        ezspSendRawMessage( macFrame.size(), macFrame.data() );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
};

}  // namespace

ZigbeeTest::ZigbeeTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( TEST_NAME, TEST_TIMEOUT, exitSignalRef ) {}

HardwareTestInterface::TestResult ZigbeeTest::run() {
    auto make_test = []( const char* name ) { return JSON( "name" << name << "result" << false ); };
    auto pass = []( JsonObject& j ) { j[ "result" ] = true; };
    TestResult result = HardwareTestInterface::TestResult::UNKNOWN;

    auto testChipInit = make_test( "chip init" );
    auto testSetChannel = make_test( "set channel" );
    auto testGetMac = make_test( "get mac" );
    auto testGetPanId = make_test( "get panId" );
    auto testSendFrames = make_test( "send frames" );
    auto testResults = make_test( m_name.c_str() );

    testLog() << "Starting " << m_name << " test...";
    auto startTestTime = TimeUtils::getMonotonicTime();
    try {
        initZigbeeFramework();
        pass( testChipInit );

        setupRadioChannel();
        pass( testSetChannel );

        auto mac = getMac();
        auto macString = getMacString( mac );
        testLog() << "Zigbee MAC address (EUI-64): " << macString;
        testGetMac[ "mac" ] = macString;
        m_uniqueEUI64 = JSON( "eui64_zigbee_0" << macString );
        pass( testGetMac );

        size_t panId = getPanId();
        auto panIdString = getPanIdString( panId );
        testLog() << "PanId: " << panIdString;
        testGetPanId[ "panId" ] = panIdString;
        pass( testGetPanId );

        auto macFrame = createMacFrame( panId, mac );
        sendMacFrames( macFrame );
        pass( testSendFrames );
        testSendFrames[ "totalFrames" ] = FRAMES_TO_SEND;

        result = TestResult::DONE;
    }
    catch( std::runtime_error& e ) {
        result = TestResult::FAILED;
        testLog() << e.what();
    }

    auto testTime = TimeUtils::getDiffMonotonicTime( startTestTime );
    testLog() << getTestResultReport( result, testTime );

    testResults[ "result" ] = ( result == TestResult::DONE ) ? true : false;
    JsonArray subtests = { testChipInit, testSetChannel, testGetMac, testGetPanId, testSendFrames };
    size_t passed = std::count_if( subtests.begin(), subtests.end(), []( JsonObject& j ) { return j[ "result" ]; } );
    testResults[ "value" ] = passed;
    testResults[ "threshold" ] = subtests.size();
    testResults[ "subtests" ] = std::move( subtests );

    testLog() << getTestResultJsonReport( testResults );

    std::string resultStr = ( result == TestResult::DONE ) ? "SUCCESS" : "FAIL";
    testLog() << "test finished with " << resultStr;

    resetZigbeeNetwork();
    zigbeeFrameworkCleanup();
    return result;
}

JsonObject ZigbeeTest::getUniqueEUI64() {
    return m_uniqueEUI64;
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#else  // __ZIGBEE_TEST_SUPPORTED__

namespace HubPlatform {
namespace HardwareTest {

HubPlatform::HardwareTest::ZigbeeTest::ZigbeeTest( const volatile sig_atomic_t& exitSignalRef )
: HardwareTestBase( "DummyZigbee", std::chrono::seconds( 1 ), exitSignalRef ) {}
HardwareTestInterface::TestResult ZigbeeTest::run() {
    return HardwareTestInterface::TestResult::DONE;
}

}  // namespace HardwareTest
}  // namespace HubPlatform

#endif  // __ZIGBEE_TEST_SUPPORTED__
