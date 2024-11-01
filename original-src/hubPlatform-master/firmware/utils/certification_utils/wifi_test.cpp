#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <exception>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <initializer_list>
#include <unistd.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <arpa/inet.h>

#include "certification_utils/man_page.h"
#include "certification_utils/common.h"

namespace HubPlatform {
namespace CertificationUtils {
namespace {

// Certification requirements
// 1) Ability to change between supported operating modes (a, b, g, n)
// 2) Ability to select the operating frequency channel
// 3) Ability to control the highest duty cycle greater than 98% for FCC/ISED and 10% for RED
// 4) Ability to control all supported Data Rates/Modulation/Modes
// 5) Ability to control the output power (maximum)
// 6) Ability to select the operating antenna, if MIMO is supported

#define PROG_NAME "wifi_test"
#if defined( __HUB_TYPE_G150__ )
#define INTERFACE_NAME "ra0"
#else
#define INTERFACE_NAME "wlan0"
#endif

#define TX_BUFF_SIZE 64  // NOTE duty cycle depends on buffer size

// NOTE modes 11a and 11b not working in OpenWRT with brcmfmac driver
// NOTE modes HT40 are not working in OpenWRT
const std::vector< std::string > supp_modes = { "11g", "11n", "11ac" };
const std::vector< std::string > supp_modul = { "HT20", "HT40", "HT40+", "NOHT" };
const std::vector< std::string > supp_rates = { "1000",  "2000",  "5500",  "6000",  "9000",  "11000",
                                                "12000", "18000", "24000", "36000", "48000", "54000" };  // kbps

class SocketGuard {
public:
    SocketGuard( const int& socket )
    : m_socket( socket ){};
    ~SocketGuard() {
        if( m_socket > 0 )
            close( m_socket );
    };

private:
    const int& m_socket;
};

std::string executeCmd( const std::string& cmd ) {
    std::array< char, 4096 > buffer;
    std::string result;
    std::unique_ptr< FILE, decltype( &pclose ) > pipe( popen( cmd.c_str(), "r" ), pclose );
    if( !pipe ) {
        throw std::runtime_error( "Can't open pipe!" );
    }
    while( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr ) {
        result += buffer.data();
    }
    return result;
}

struct Settings {
    std::string channel = "11";
    std::string mode = "11g";
    std::string power = "999";
    std::string modulation = "HT20";
    std::string rate;
};

const std::string ARG_CHANNEL = "--chan";
const std::string ARG_MODE = "--mode";
const std::string ARG_POWER = "--power";
const std::string ARG_MODULATION = "--modul";
const std::string ARG_RATE = "--rate";

void help() {
    ManPage page;

    const std::string optChannel = ARG_CHANNEL + " channel";
    const std::string optMode = ARG_MODE + " mode";
    const std::string optPower = ARG_POWER + " power";
    const std::string optModul = ARG_MODULATION + " modul";
    const std::string optRate = ARG_RATE + " rate";

    page.section( "NAME" );
    page.subSection( PROG_NAME );
    page.newLine();

    page.section( "SYNOPSIS" );
    page.subSection( ManPage::join( { PROG_NAME,
                                      ManPage::optional( optChannel ),
                                      ManPage::optional( optMode ),
                                      ManPage::optional( optPower ),
                                      ManPage::optional( optModul ),
                                      ManPage::optional( optRate ) } ) );
    page.subSection( ManPage::join( { PROG_NAME, "{-h | --help}" } ) );
    page.newLine();

    page.section( "DESCRIPTION" );
    page.subSection( "Utility to control parameters of Wi-Fi module" );
    page.newLine();

    page.section( "OPTIONS" );
    page.subSection( optChannel );
    page.descr( "Set radio channel" );
    page.descr( "Default: 11" );
    page.descr( ManPage::join( { "Example:", ARG_CHANNEL, "8" } ) );
    page.newLine();

    page.subSection( optMode );
    page.descr( "Set Wi-Fi mode" );
    page.descr( "Default: 11g" );
    page.descr( "All modes: " + ManPage::join( supp_modes ) );
    page.descr( ManPage::join( { "Example:", ARG_MODE, supp_modes.front() } ) );
    page.newLine();

    page.subSection( optPower );
    page.descr( "Set tx power in dbm" );
    page.descr( "Default: Power is max if arg not present" );
    page.descr( "Max power limit defined by the driver" );
    page.descr( ManPage::join( { "Example:", ARG_POWER, "20" } ) );
    page.newLine();

    page.subSection( optModul );
    page.descr( "Set modulation" );
    page.descr( "Default: HT20" );
    page.descr( "All modulations: " + ManPage::join( supp_modul ) );
    page.descr( ManPage::join( { "Example:", ARG_MODULATION, supp_modul.front() } ) );
    page.newLine();

    page.subSection( optRate );
    page.descr( "Set rate in kbps" );
    page.descr( "Rate auto defined by driver if arg not present" );
    page.descr( "All rates: " + ManPage::join( supp_rates ) );
    page.descr( ManPage::join( { "Example:", ARG_RATE, supp_rates.front() } ) );

    std::cout << page.content().str() << std::endl << std::flush;
}

Settings parseArgs( int argc, char* argv[] ) {
    Settings settings;
    std::vector< std::string > args;

    auto checkRange = [&args]( std::vector< std::string >::const_iterator& it ) {
        if( it == args.cend() )
            throw std::runtime_error( "argument missing or out of range" );
    };
    auto found = []( const std::string& value, const std::vector< std::string >& arr ) {
        return std::find( arr.cbegin(), arr.cend(), value ) != arr.cend();
    };

    if( argc == 1 )
        return settings;

    args.reserve( argc );
    for( size_t i = 1; i < argc; ++i )
        args.emplace_back( argv[ i ] );

    if( args.size() == 1 && ( args[ 0 ] == "-h" || args[ 0 ] == "--help" ) ) {
        help();
        exit( 0 );
    }

    try {
        for( auto it = args.cbegin(); it != args.cend(); ++it ) {
            if( *it == ARG_CHANNEL ) {
                checkRange( ++it );
                if( !isNumber( *it ) )
                    throw std::runtime_error( "channel is not a number" );
                settings.channel = *it;
            }
            else if( *it == ARG_MODE ) {
                checkRange( ++it );
                if( !found( *it, supp_modes ) )
                    throw std::runtime_error( "mode is not recognized" );
                settings.mode = *it;
            }
            else if( *it == ARG_POWER ) {
                checkRange( ++it );
                if( !isNumber( *it ) )
                    throw std::runtime_error( "power is not a number" );
                settings.power = *it;
            }
            else if( *it == ARG_MODULATION ) {
                checkRange( ++it );
                if( !found( *it, supp_modul ) )
                    throw std::runtime_error( "modulation is not recognized" );
                settings.modulation = *it;
            }
            else if( *it == ARG_RATE ) {
                checkRange( ++it );
                if( !found( *it, supp_rates ) )
                    throw std::runtime_error( "rate is not recognized" );
                settings.rate = *it;
            }
            else
                throw std::runtime_error( "option \"" + *it + "\" is not recognized" );
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

void reloadSettings( const Settings& settings ) {
    const std::string shift = "        ";
    std::stringstream ss;

    auto config = [&ss]( const std::string& option, const std::string& value ) {
        ss << option << " " << value << std::endl;
    };
    auto set = [&ss, &shift]( const std::string& option, const std::string& value ) {
        ss << shift << option << " " << value << std::endl;
    };

    config( "config wifi-device", "radio0" );
    set( "option channel", settings.channel );
    set( "option disabled", "0" );
    set( "option htmode", settings.modulation );
    set( "option hwmode", settings.mode );
    if( !settings.power.empty() )
        set( "option txpower", settings.power );
    if( !settings.rate.empty() ) {
        set( "list supported_rates", "'" + settings.rate + "'" );
        set( "list basic_rate", "'" + settings.rate + "'" );
    }
    set( "option path", "platform/soc/1c10000.mmc/mmc_host/mmc0/mmc0:0001/mmc0:0001:1" );
    set( "option type", "mac80211" );

    // NOTE Signal does not transmitting if encryption is enabled
    ss << std::endl;
    config( "config wifi-iface", "ap_wifi" );
    set( "option device", "radio0" );
    set( "option key", "12345678" );
    set( "option mode", "ap" );
    set( "option network", "ap_wifi" );
    set( "option ssid", "wifi_test" );

    std::cout << "Settings" << std::endl;
    std::cout << ss.str() << std::endl;

    executeCmd( "wifi down" );
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

    std::ofstream file( "/etc/config/wireless" );
    file << ss.str();
    file.close();

    std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) );
    executeCmd( "wifi up" );
}

void startTxStream() {
    std::vector< uint8_t > txBuff( TX_BUFF_SIZE, 0xAA );

    // NOTE TX stream based on raw socket doesn't transmit signal
    int udpSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( udpSocket < 0 )
        throw std::runtime_error( "Can't create socket" );
    SocketGuard socketGuard( udpSocket );

    int canBroadcast = 1;
    if( setsockopt( udpSocket, SOL_SOCKET, SO_BROADCAST, &canBroadcast, sizeof( canBroadcast ) ) < 0 )
        throw std::runtime_error( "Can't set socket to broadcast" );

    if( setsockopt( udpSocket, SOL_SOCKET, SO_BINDTODEVICE, INTERFACE_NAME, strlen( INTERFACE_NAME ) + 1 ) < 0 )
        throw std::runtime_error( "Can't bind socket to device" );

    struct sockaddr_in broadcastAddr;
    memset( &broadcastAddr, 0, sizeof( struct sockaddr_in ) );
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = inet_addr( "255.255.255.255" );
    broadcastAddr.sin_port = htons( 9999 );

    size_t errors = 0;
    size_t success = 0;
    while( true ) {
        int sendLen = sendto( udpSocket,
                              txBuff.data(),
                              txBuff.size(),
                              0,
                              (struct sockaddr*) &broadcastAddr,
                              sizeof( struct sockaddr_in ) );
        if( sendLen < 0 ) {
            std::cout << "Can't send packet" << std::endl << std::flush;
            ++errors;
            success = 0;
        }
        else
            ++success;
        if( errors > 0 && success == 10 )
            std::cout << "Continue sending packets" << std::endl << std::flush;
    }
}

}  // namespace
}  // namespace CertificationUtils
}  // namespace HubPlatform

int main( int argc, char* argv[] ) {
    using namespace HubPlatform::CertificationUtils;

    try {
        Settings settings = parseArgs( argc, argv );
        reloadSettings( settings );

        std::this_thread::sleep_for( std::chrono::milliseconds( 3000 ) );
        std::cout << "START TX stream" << std::endl << std::flush;
        startTxStream();
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
