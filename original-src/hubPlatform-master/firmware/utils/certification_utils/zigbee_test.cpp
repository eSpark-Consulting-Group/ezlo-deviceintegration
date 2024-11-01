#include "hardware_test/hardware_tests/zigbee_test.h"

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
#include <platform/base/phy/ieee802154mac.h>
#include <protocol/zigbee/stack/include/ember-types.h>
}

#include "certification_utils/man_page.h"
#include "certification_utils/common.h"

extern const char* __progname;

namespace HubPlatform {
namespace CertificationUtils {
namespace {

#define PROG_NAME "zigbee_test"
#define MIN_CHANNEL 11
#define MAX_CHANNEL 26
#define DEFAULT_CHANNEL 25

#define HAS_MFGLIB_RX_CALLBACK false

extern "C" void emAfResetAndInitNCP();
extern "C" void ezspChildJoinHandler( int8u, boolean, EmberNodeId, EmberEUI64, EmberNodeType ){ /* stub */ };
extern "C" void hubPlatformPollControlCheckInCallback( uint16_t, uint8_t, uint16_t ){ /* stub */ };

using Buffer = std::vector< uint8_t >;

struct Settings {
    std::string channel = std::to_string( DEFAULT_CHANNEL );
    std::string txPower;
};

const std::string ARG_CHANNEL = "--chan";
const std::string ARG_POWER = "--power";

void help() {
    ManPage page;

    const std::string optChannel = ARG_CHANNEL + " channel";
    const std::string optPower = ARG_POWER + " power";

    page.section( "NAME" );
    page.subSection( PROG_NAME );
    page.newLine();

    page.section( "SYNOPSIS" );
    page.subSection( ManPage::join( { PROG_NAME, ManPage::optional( optChannel ), ManPage::optional( optPower ) } ) );
    page.subSection( ManPage::join( { PROG_NAME, "{-h | --help}" } ) );
    page.newLine();

    page.section( "DESCRIPTION" );
    page.subSection( "Utility to control parameters of Zigbee module" );
    page.newLine();

    page.section( "OPTIONS" );
    page.subSection( optChannel );
    page.descr( "Set radio channel" );
    std::string minChannelStr = std::to_string( MIN_CHANNEL );
    std::string maxChannelStr = std::to_string( MAX_CHANNEL );
    page.descr( ManPage::join( { "Channel range:", minChannelStr, "<= N <=", maxChannelStr } ) );
    page.descr( "Default: " + std::to_string( DEFAULT_CHANNEL ) );
    page.subSection( optPower );
    page.descr( "Set radio TX power in dBm" );

    page.newLine();
    page.section( ManPage::join( { "Example:", ARG_CHANNEL, "25", ARG_POWER, "0" } ) );

    std::cout << page.content().str() << std::endl << std::flush;
}

Settings parseArgs( int argc, char* argv[] ) {
    Settings settings;
    std::vector< std::string > args;

    auto check_range = [&args]( std::vector< std::string >::const_iterator& it ) {
        if( it == args.cend() )
            throw std::runtime_error( "Argument missing or out of range" );
    };

    if( argc == 1 )
        return settings;

    for( size_t i = 1; i < argc; ++i )
        args.push_back( argv[ i ] );

    if( args.size() && ( args.front() == "-h" || args.front() == "--help" ) ) {
        help();
        exit( 0 );
    }

    try {
        for( auto it = args.cbegin(); it != args.cend(); ++it ) {
            if( *it == ARG_CHANNEL ) {
                check_range( ++it );
                if( !isNumber( *it ) )
                    throw std::runtime_error( "Channel is not a number" );

                size_t channel = std::stoi( *it );
                if( channel < MIN_CHANNEL || channel > MAX_CHANNEL ) {
                    std::stringstream ss;
                    ss << "Channel should be in range: ";
                    ss << MIN_CHANNEL << " <= " << channel << " <= " << MAX_CHANNEL;
                    throw std::runtime_error( ss.str() );
                }

                settings.channel = *it;
            }
            else if( *it == ARG_POWER ) {
                check_range( ++it );
                if( !isNumber( *it ) )
                    throw std::runtime_error( "Power is not an integer number" );
                settings.txPower = *it;
            }
            else
                throw std::runtime_error( "Option \"" + *it + "\" is not recognized" );
        }
    }
    catch( const std::exception& e ) {
        std::cout << "Error! " << e.what() << std::endl << std::flush;
        std::cout << std::endl;
        help();
        exit( -1 );
    }
    return settings;
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
    std::cout << "Zigbee device: " << device << " found!" << std::endl;

    HubPlatform::Environment::Zigbee::bootInit();
    HubPlatform::Environment::Zigbee::resetInit();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    HubPlatform::Environment::Zigbee::resetCycle();
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );

    std::cout << "Zigbee stack initialization started" << std::endl;
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
        std::cout << "Zigbee network is not configured. Configure network..." << std::endl;
        emberAfPluginNetworkCreatorStart( true );
    }

    std::cout << "Zigbee framework initialization success" << std::endl;
};

void zigbeeFrameworkCleanup() {
    HubPlatform::Environment::Zigbee::resetAssert();
    std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
    emberSerialCleanup();
    std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );
}

void setupRadioChannel( int channel ) {
    std::stringstream err_msg;

    auto channelStatus = ezspSetRadioChannel( channel );
    if( channelStatus != EMBER_SUCCESS ) {
        err_msg << "Radio channel setup failed with status: " << channelStatus << std::endl;
        throw std::runtime_error( err_msg.str() );
    }

    size_t radioChannel = emberAfGetRadioChannel();
    if( channel != radioChannel ) {
        err_msg << "Radio channel setup failed: expected channel - " << channel;
        err_msg << " : current channel - " << radioChannel << std::endl;
        throw std::runtime_error( err_msg.str() );
    }
    std::cout << "Zigbee radio channel set to: " << radioChannel << std::endl;
}

void setupTxPower( int8_t power ) {
    auto status = mfglibSetPower( EMBER_TX_POWER_MODE_DEFAULT, power );
    if( status != EMBER_SUCCESS ) {
        std::stringstream error;
        error << "Radio power setup failed with status: " << status << std::endl;
        throw std::runtime_error( error.str() );
    }
}

int8_t getTxPower() {
    return mfglibGetPower();
}

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
    while( true ) {
        mfglibSendPacket( macFrame.size(), macFrame.data() );
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    }
};

void startTxStream( const Settings& settings ) {
    try {
        initZigbeeFramework();

        int channel = std::stoi( settings.channel );
        setupRadioChannel( channel );

        auto mac = getMac();
        auto macString = getMacString( mac );
        std::cout << "Zigbee MAC address (EUI-64): " << macString << std::endl;

        size_t panId = getPanId();
        auto panIdString = getPanIdString( panId );
        std::cout << "PanId: " << panIdString << std::endl;

        ezspMfglibStart( HAS_MFGLIB_RX_CALLBACK );

        if( !settings.txPower.empty() ) {
            int8_t txPower = std::stoi( settings.txPower );
            setupTxPower( txPower );
        }
        std::cout << "Zigbee radio TX power: " << int( getTxPower() ) << " dBm" << std::endl;

        std::cout << "START TX stream" << std::endl << std::flush;
        auto macFrame = createMacFrame( panId, mac );
        sendMacFrames( macFrame );

        mfglibEnd();
    }
    catch( std::runtime_error& e ) {
        std::cout << e.what() << std::endl << std::flush;
    }

    zigbeeFrameworkCleanup();
}

}  // namespace
}  // namespace CertificationUtils
}  // namespace HubPlatform

int main( int argc, char* argv[] ) {
    using namespace HubPlatform::CertificationUtils;
    try {
        Settings settings = parseArgs( argc, argv );
        stopFirmware();
        startTxStream( settings );
    }
    catch( const std::exception& e ) {
        std::cout << "Error! " << e.what() << std::endl << std::flush;
        return -1;
    }
    catch( ... ) {
        std::cout << "Error! Unknown exception" << std::endl << std::flush;
    }
    std::cout << "STOP TX stream" << std::endl << std::flush;
    return 0;
}
