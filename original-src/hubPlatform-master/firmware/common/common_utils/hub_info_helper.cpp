#include "common_utils/hub_info_helper.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"
#include "provision_data_utils/provision_data_manager.h"
#include "log_utils/ezlo_log.h"

#include "tiny/tiny_consolecolors.h"
#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "tiny/include/yamc/alternate_shared_mutex.hpp"

#include <string.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/mman.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#define STRING( S ) STR( S )
#define STR( S ) #S

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
#include "vera_utils/provision_reader.h"

HubPlatform::VeraProvision::ProvisionReader& getVeraProvisionReader() {
    static std::unique_ptr< HubPlatform::VeraProvision::ProvisionReader > veraProvisionReader;
    if( !veraProvisionReader ) {
        veraProvisionReader.reset( new HubPlatform::VeraProvision::ProvisionReader() );
        veraProvisionReader->readAll();
    }
    return *veraProvisionReader.get();
}
#endif

namespace HubInfoHelper {

namespace {

using namespace HubPlatform;

// serial( 12345678 ) -> hex serial( BC614E )-> MAC( 00:00:00:BC:61:4E )
std::string macAddressFromSerial( const std::string& serial ) {
    char* end = nullptr;
    uint64_t mac = std::strtoul( serial.c_str(), &end, 10 );

    std::stringstream ss;
    for( int i = 5; i >= 0; --i ) {
        ss << std::uppercase << std::hex << std::setfill( '0' ) << std::setw( 2 ) << int( ( mac >> ( i * 8 ) ) & 0xFF );
        if( i > 0 )
            ss << ":";
    }
    return ss.str();
}

// Implements thread safe setExplicitValue / getValue
class HubValueHolder {
public:
    using SharedMutex = yamc::alternate::shared_mutex;

    void setExplicitValue( const std::string& value ) {
        std::unique_lock< SharedMutex > lock( m_mutex );

        m_value = value;
    }

    template < typename Reader >
    const std::string& getValue( Reader&& reader ) {
        {
            yamc::shared_lock< SharedMutex > lock( m_mutex );

            if( !m_value.empty() ) {
                return m_value;
            }
        }

        std::unique_lock< SharedMutex > lock( m_mutex );

        if( m_value.empty() ) {
            m_value = reader();
        }

        return m_value;
    }

private:
    std::string m_value;
    SharedMutex m_mutex;
};

HubValueHolder SerialHolder;
HubValueHolder UUIDHolder;
HubValueHolder PkDeviceHolder;
HubValueHolder UuidHolder;
HubValueHolder HubTypeHolder;
HubValueHolder HardwareRevisionHolder;
HubValueHolder MacAddressHolder;
HubValueHolder WifiMacAddressHolder;
HubValueHolder HardwareKeyHolder;
HubValueHolder CpuSidHolder;

template < typename Function >
void makeFewAttempts( Function&& function ) {
    constexpr const int maxTries = 10;
    int tries = 0;
    while( tries++ < maxTries ) {
        try {
            if( function( tries ) ) {
                break;
            }
        }
        catch( const std::exception& ex ) {
            Log::error( cc::error( "std::exception: " ) + cc::error( ex.what() ) );
        }
        catch( ... ) {
            Log::error( cc::error( "unknown exception" ) );
        }
    }
}

std::string readPipe( const std::string& command ) {
    FILE* pipe;
    char pipeBuffer[ 64 ];
    pipe = popen( command.c_str(), "r" );
    if( pipe != nullptr ) {
        fgets( pipeBuffer, sizeof( pipeBuffer ), pipe );
        fclose( pipe );
    }

    if( strlen( pipeBuffer ) <= 0 ) {
        return std::string();
    }
    else {
        std::string result( pipeBuffer, strlen( pipeBuffer ) );
        result.erase( std::remove( result.begin(), result.end(), '\n' ), result.end() );
        return result;
    }
}

bool validateSerial( const std::string& serial ) {
    if( serial.empty() ) {
        return false;
    }

    for( auto symbol : serial ) {
        if( !std::isdigit( symbol ) )  // serial must contain alphanumeric symbols only
            return false;
    }

    return true;
}

std::string readSerial() {
    std::string serial( __HUB_SERIAL_STR__ );

    try {
        std::string readResult;
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
        Log::trace( cc::trace( "Reading serial from nvram" ) );
        readResult = getVeraProvisionReader().getSerial();
#else
        Log::trace( cc::trace( "Reading serial from controller information" ) );
        auto& provisionData = ProvisionDataUtils::getManager().getProvisionData();
        if( provisionData.isValid() ) {
            readResult = std::to_string( provisionData.id );
        }
#endif
        if( validateSerial( readResult ) ) {
            serial = readResult;
        }
    }
    catch( const std::exception& ex ) {
        Log::warn( cc::info( "Exception: " ) + cc::warn( ex.what() ) );
    }
    return serial;
}

std::string readUUID() {
    std::string serial( "" );

    Log::trace( cc::trace( "Reading UUID from controller information" ) );
    auto& data = ProvisionDataUtils::getManager().getProvisionData();
    if( !data.uuid.empty() ) {
        serial = data.uuid;
    }
    else {
        Log::warn( cc::warn( "Cannot read UUID" ) );
    }
    return serial;
}

std::string readPkDevice() {
    return readSerial();
}

std::string readHubType() {
    std::string hubType( __HUB_TYPE_STR__ );
    return hubType;
}

std::string readHardwareRevision() {
    LOGGER_TRACE_RAW( cc::trace( "Reading hardware revision from controller information" ) );
    auto& data = ProvisionDataUtils::getManager().getProvisionData();
    return data.hardwareRevision;
}

bool validateMacAddress( const std::string& macAddress ) {
    // TODO validate MAC properly
    return !macAddress.empty();
}

std::map< std::string, std::string > readNetworkInterfaces() {
    Log::trace( cc::trace( "Reading MAC address from system" ) );
    struct ifaddrs *addrs, *next;
    int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
    getifaddrs( &addrs );
    next = addrs;
    std::map< std::string, std::string > interfaceMacAddresses;
    while( next ) {
        if( next->ifa_addr && next->ifa_addr->sa_family == AF_PACKET ) {
            struct ifreq ifr;
            strcpy( ifr.ifr_name, next->ifa_name );
            // skip loopback interface
            if( !( next->ifa_flags & IFF_LOOPBACK ) ) {
                if( ioctl( sock, SIOCGIFHWADDR, &ifr ) == 0 ) {
                    // one extra for terminating '\0'
                    char macString[ 18 ];
                    sprintf( macString,
                             "%02x:%02x:%02x:%02x:%02x:%02x",
                             (uint8_t) ifr.ifr_hwaddr.sa_data[ 0 ],
                             (uint8_t) ifr.ifr_hwaddr.sa_data[ 1 ],
                             (uint8_t) ifr.ifr_hwaddr.sa_data[ 2 ],
                             (uint8_t) ifr.ifr_hwaddr.sa_data[ 3 ],
                             (uint8_t) ifr.ifr_hwaddr.sa_data[ 4 ],
                             (uint8_t) ifr.ifr_hwaddr.sa_data[ 5 ] );
                    Log::trace( cc::trace( "Interface / MAC: " ) + cc::note( next->ifa_name ) + cc::trace( " / " ) +
                                cc::note( macString ) );
                    interfaceMacAddresses.emplace( std::string( next->ifa_name ), std::string( macString ) );
                }
            }
        }
        next = next->ifa_next;
    }
    freeifaddrs( addrs );

    return interfaceMacAddresses;
}

std::string readMacAddress() {
    Log::trace( cc::trace( "Reading MAC address from config" ) );

#if defined( __HUB_TYPE_VHUB__ )
    return macAddressFromSerial( getSerial() );
#endif

    try {
        ConfigReader configReader;
        if( configReader.hasValue( ConfigKeys::HubMac ) ) {
            const auto& macAddress = configReader.getString( ConfigKeys::HubMac );
            if( validateMacAddress( macAddress ) ) {
                return macAddress;
            }
        }
    }
    catch( const ConfigReader::ReadingError& exception ) {
        Log::warn( cc::info( "Config Reader: " ) + cc::warn( exception.what() ) );
    }

    Log::trace( cc::trace( "Reading MAC address from cmake option" ) );
    std::string macAddress( __MAC_ADDRESS_STR__ );
    if( validateMacAddress( macAddress ) ) {
        return macAddress;
    }

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    Log::trace( cc::trace( "Reading MAC address from nvram" ) );
    macAddress = getVeraProvisionReader().getMacAddressString();
#endif

    auto interfaceMacAddresses = readNetworkInterfaces();
    for( const auto& it : interfaceMacAddresses ) {
        if( ( it.first.find( "eth" ) == 0 || it.first.find( "enp" ) == 0 ) && validateMacAddress( it.second ) ) {
            macAddress = it.second;
            break;
        }
    }
    Log::trace( cc::trace( "Wired MAC address: " ) + cc::note( macAddress ) );
    return macAddress;
}

std::string readWifiMacAddress() {
    std::string macAddress;
    auto interfaceMacAddresses = readNetworkInterfaces();
    for( const auto& it : interfaceMacAddresses ) {
        // return ( ifName.find( "wlan" ) == 0 || ifName.find( "ra" ) == 0 || ifName.find( "apcli" ) == 0 );
        if( ( it.first.find( "wlan" ) == 0 || it.first.find( "ra" ) == 0 || it.first.find( "apcli" ) == 0 ) &&
            validateMacAddress( it.second ) ) {
            macAddress = it.second;
            break;
        }
    }
    Log::info( cc::info( "Wireless MAC address: " ) + cc::warn( macAddress ) );
    return macAddress;
}

std::string readHardwareKey() {
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    Log::trace( cc::trace( "Reading hardware key from nvram" ) );
    return getVeraProvisionReader().getHwKey();
#else
    Log::info( cc::info( "Try reading hardware key from somewhere" ) );
    return {};
#endif
}

std::string readCpuSid() {
    std::string cpuSid = "";
#if( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ ) )
    static const auto MapSize = getpagesize();
    static const auto MapMask = ( MapSize - 1 );
    const uintptr_t SidRegisterBase = 0x01c14200;

    int fd = -1;
    if( ( fd = open( "/dev/mem", O_RDWR | O_SYNC ) ) >= 0 ) {
        void *pMapBase, *pVirtAddr;
        pMapBase = mmap( 0, MapSize, PROT_READ, MAP_SHARED, fd, ( SidRegisterBase & ~MapMask ) );
        if( pMapBase != (void*) -1 ) {
            Log::trace( cc::trace( "Reading CPU SID for H2+/H2 Secure from SID register" ) );
            pVirtAddr = (void*) ( (uint64_t) pMapBase + ( SidRegisterBase & MapMask ) );
            char result[ 36 ];
            sprintf( result,
                     "%08x:%08x:%08x:%08x",
                     *( (uint32_t*) pVirtAddr ),
                     *( (uint32_t*) pVirtAddr + 1 ),
                     *( (uint32_t*) pVirtAddr + 2 ),
                     *( (uint32_t*) pVirtAddr + 3 ) );
            cpuSid = result;
        }
        munmap( (void*) pMapBase, MapSize );
        close( fd );
    }
    else {
        Log::warn( cc::warn( "Failed to read CPU SID for H2+\"/dev/mem\": " ) + cc::warn( strerror( fd ) ) );
    }
#else
    Log::info( cc::info( "Try reading CPU SID from somewhere" ) );
#endif
    return cpuSid;
}

}  // namespace

void setExplicitSerial( const std::string& serial ) {
    SerialHolder.setExplicitValue( serial );
}

void setExplicitHubType( const std::string& hubType ) {
    HubTypeHolder.setExplicitValue( hubType );
}

const std::string& getSerial() {
    return SerialHolder.getValue( readSerial );
}

const std::string& getControllerUUID() {
    return UUIDHolder.getValue( readUUID );
}

const std::string& getPkDevice() {
    return PkDeviceHolder.getValue( readPkDevice );
}

const std::string& getHubType() {
    return HubTypeHolder.getValue( readHubType );
}

const std::string& getHardwareRevision() {
    return HardwareRevisionHolder.getValue( readHardwareRevision );
}

const std::string& getMacAddress() {
    return MacAddressHolder.getValue( readMacAddress );
}

const std::string& getWifiMacAddress() {
    return WifiMacAddressHolder.getValue( readWifiMacAddress );
}

const std::string& getHardwareKey() {
    return HardwareKeyHolder.getValue( readHardwareKey );
}

const std::string& getCpuSid() {
    return CpuSidHolder.getValue( readCpuSid );
}

}  // namespace HubInfoHelper
