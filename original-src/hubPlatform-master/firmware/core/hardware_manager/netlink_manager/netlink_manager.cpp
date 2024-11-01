#include "hardware_manager/netlink_manager/netlink_manager.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"
#include "system_utils/filesystem.h"
#include "common_utils/cpp14.h"

#include <array>
#include <algorithm>
#include <cstring>

#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/limits.h>
#include <unistd.h>

namespace HubPlatform {
namespace HardwareManager {

namespace {
using namespace EventsLoop;

std::string getHotPlugDeviceClass( HotPlugEventDeviceClass deviceClass ) {
    if( deviceClass == HotPlugEventDeviceClass::USB ) {
        return "usb";
    }
    else if( deviceClass == HotPlugEventDeviceClass::ETHERNET ) {
        return "ethernet";
    }

    return "unknown";
}

std::string getHotPlugEventType( HotPlugEventType type ) {
    if( type == HotPlugEventType::ADD ) {
        return "add";
    }
    else if( type == HotPlugEventType::REMOVE ) {
        return "remove";
    }

    return "unknown";
}

class NetlinkSocketsCreator : public INetlinkSocketsCreator {
public:
    ~NetlinkSocketsCreator() = default;

    int createNetlinkUeventSocket( uint32_t netlinkGroups ) override {
        int netlinkUeventSocketFd = socket( AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT );
        if( netlinkUeventSocketFd < 0 ) {
            Log::error( cc::error( "Failed to create a NETLINK_KOBJECT_UEVENT socket." ) );
            throw NetlinkManagerException( "Failed to create a NETLINK_KOBJECT_UEVENT socket." );
        }

        sockaddr_nl saddr;

        std::memset( &saddr, 0, sizeof( saddr ) );
        saddr.nl_family = AF_NETLINK;
        saddr.nl_groups = netlinkGroups;

        if( bind( netlinkUeventSocketFd, reinterpret_cast< sockaddr* >( &saddr ), sizeof( saddr ) ) < 0 ) {
            Log::error( cc::error( "Failed to bind a NETLINK_KOBJECT_UEVENT socket." ) );
            throw NetlinkManagerException( "Failed to bind a NETLINK_KOBJECT_UEVENT socket." );
        }

        return netlinkUeventSocketFd;
    }
};

bool isUSBDevicePath( const std::string& path ) {
    return filesystem::isExist( path + "/idVendor" ) && filesystem::isExist( path + "/idProduct" );
}
}  // namespace

const std::map< std::string, NetlinkManager::UEvent::Actions > NetlinkManager::UEvent::actionNames = {
    { "add", Actions::ADD },       { "bind", Actions::BIND },     { "change", Actions::CHANGE },
    { "move", Actions::MOVE },     { "remove", Actions::REMOVE }, { "unbind", Actions::UNBIND },
    { "online", Actions::ONLINE },
};

const std::map< std::string, NetlinkManager::UEvent::SubSystems > NetlinkManager::UEvent::subsystemNames = {
    { "usb", SubSystems::USB },
    { "net", SubSystems::NETWORK },
    { "i2c", SubSystems::I2C },
};

NetlinkManager::UEvent::UEvent( const std::string& uevent )
: action( Actions::FAILED )
, subsystem( SubSystems::FAILED ) {
    std::stringstream ueventStream( uevent );
    std::string ueventLine;

    auto checkAndInitMandatoryField = [this]( const std::pair< Key, Value >& pair ) {
        if( pair.first == "ACTION" ) {
            auto it = actionNames.find( pair.second );
            if( it != actionNames.end() ) {
                action = it->second;
            }
            else {
                action = Actions::UNKNOWN;
            }

            return true;
        }
        if( pair.first == "SUBSYSTEM" ) {
            auto it = subsystemNames.find( pair.second );
            if( it != subsystemNames.end() ) {
                subsystem = it->second;
            }
            else {
                subsystem = SubSystems::UNKNOWN;
            }

            return true;
        }
        if( pair.first == "DEVPATH" ) {
            devpath = pair.second;
            return true;
        }

        return false;
    };

    auto checkMandatoryFields = [this]() {
        if( action == Actions::FAILED || subsystem == SubSystems::FAILED || devpath.empty() ) {
            return false;
        }

        return true;
    };

    std::getline( ueventStream, header, '\0' );
    if( header.find( "@" ) == std::string::npos ) {
        throw NetlinkManagerException( "Failed to parse a uevent event, invalid header, should have an '@' symbol" );
    }
    LOGGER_TRACE( cc::trace( "netlink uevent header: " + header ) );

    while( std::getline( ueventStream, ueventLine, '\0' ) ) {
        std::string delimeter( "=" );

        const auto delimeterPos = ueventLine.find( delimeter );
        if( delimeterPos == std::string::npos ) {
            throw NetlinkManagerException( "Failed to parse a uevent event, invalid format" );
        }

        const auto keyValue =
            std::make_pair( ueventLine.substr( 0, delimeterPos ), ueventLine.substr( delimeterPos + 1 ) );

        if( !checkAndInitMandatoryField( keyValue ) ) {
            if( !optionalFields.insert( keyValue ).second ) {
                throw NetlinkManagerException( "Failed to parse a uevent event, a duplicate value" );
            }
        }
    }

    if( !checkMandatoryFields() ) {
        throw NetlinkManagerException( "Failed to parse a uevent event, lack of mandatory fields" );
    }
}

Json HotPlugEvent::toJson() const {
    return JSON( "deviceClass" << getHotPlugDeviceClass( deviceClass ) << "type" << getHotPlugEventType( type )
                               << "devicePath" << devicePath );
}

INetlinkEventsHandler::~INetlinkEventsHandler() = default;
INetlinkSocketsCreator::~INetlinkSocketsCreator() = default;

NetlinkManager::NetlinkManager( IDbClient& dbClient, INetlinkEventsHandler& netlinkEventsHandler )
: NetlinkManager( dbClient, netlinkEventsHandler, std::make_unique< NetlinkSocketsCreator >() ) {}

NetlinkManager::NetlinkManager( IDbClient& dbClient,
                                INetlinkEventsHandler& netlinkEventsHandler,
                                std::unique_ptr< INetlinkSocketsCreator > netlinkSocketsCreator )
: m_dbClient( dbClient )
, m_netlinkEventsHandler( netlinkEventsHandler ) {
    syncEventsEnvironment();

    m_netlinkUeventSocketFd =
        netlinkSocketsCreator->createNetlinkUeventSocket( -1 );  // listen for all broadcast events
    registerEventSources();
}

NetlinkManager::~NetlinkManager() {
    close( m_netlinkUeventSocketFd );  // it's safe to pass a negative value
}

/* syncs netlink manager's events with a current state;
 * they can differ in a case of a crash of hw at the time when such events are produced by kernel
 * or in a case of starting the firmware with already plugged devices
 * (kernel provides us by events occurred AFTER we bound socket(s)...) */
void NetlinkManager::syncEventsEnvironment() {
    syncUSBDeviceEvents();
}

void NetlinkManager::syncUSBDeviceEvents() {
    const auto realUSBDevices = getRealUSBDevices();
    const auto dbUSBDevices = getDbUSBDevices();

    for( const auto& usbDevice : realUSBDevices ) {
        if( dbUSBDevices.find( usbDevice ) == dbUSBDevices.end() ) {
            issueUSBAddHotPlugEvent( usbDevice );
        }
    }

    for( const auto& usbDevice : dbUSBDevices ) {
        if( realUSBDevices.find( usbDevice ) == realUSBDevices.end() ) {
            issueUSBRemoveHotPlugEvent( usbDevice );
        }
    }
}

std::set< std::string > NetlinkManager::getRealUSBDevices() {
    std::set< std::string > realUSBDevicesNames;
    std::string path( "/sys/bus/usb/devices/" );

    auto entryHandler = [&realUSBDevicesNames]( const std::string& basePath, const std::string& entryName ) {
        /* skip usb root devices and usb buses, but pass usb devices taking into account
         * a usb device tree (usb hubs + usb devices) */
        if( entryName.find( ":" ) == std::string::npos && entryName.find( "usb" ) == std::string::npos ) {
            const auto fullName = basePath + entryName;
            std::array< char, PATH_MAX > buff;

            auto retValue = readlink( fullName.c_str(), &buff[ 0 ], buff.size() );
            if( retValue < 0 ) {
                throw std::runtime_error( "Failed to list usb devices available in the system, failed to read a link" );
            }

            const auto absName =
                std::string( buff.begin(), buff.begin() + retValue ).replace( 0, std::strlen( "../../.." ), "/sys" );
            if( !filesystem::isDirectoryExist( absName ) ) {
                throw std::runtime_error( "Failed to list usb devices available in the system, invalid absolute name" );
            }

            realUSBDevicesNames.insert( absName.substr( std::strlen( "/sys" ) ) );
        }

        return true;
    };

    filesystem::iterateFileSystemEntry( path, entryHandler );

    return realUSBDevicesNames;
}

std::set< std::string > NetlinkManager::getDbUSBDevices() {
    std::set< std::string > dbUSBDevices;
    const JsonObject fieldsToReturn = JSON( db::ID_FIELD_NAME << "id" );
    const auto data = m_dbClient.findAll( NetlinkUSBDevicesDBName, JSON_EMPTY_OBJ, &fieldsToReturn );

    for( const auto& device : data ) {
        dbUSBDevices.insert( json_get_str( device, "id" ) );
    }

    return dbUSBDevices;
}

void NetlinkManager::registerEventSources() {
    m_epollLoop.addEventSource(
        m_netlinkUeventSocketFd,
        EventType::IN,
        [this]( EventType eventType, IEventsLoop::EventSourceHandle eventSourceHandler ) { handleNetlinkUevents(); } );
}

void NetlinkManager::handleNetlinkUevents() {
    std::array< char, 4096 > buff;

    try {
        const auto readAmount = read( m_netlinkUeventSocketFd, &buff[ 0 ], buff.size() );
        if( readAmount < 0 ) {
            if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ) {
                return;
            }

            throw NetlinkManagerException( "Failed to read from a NETLINK_KOBJECT_UEVENT netlink socket." );
        }

        const auto uevent = UEvent( std::string( buff.begin(), buff.begin() + readAmount ) );

        if( uevent.subsystem == UEvent::SubSystems::USB ) {
            handleUSBUevent( uevent );
        }
        else if( uevent.subsystem == UEvent::SubSystems::I2C ) {
            handleI2CUevent( uevent );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( std::string( "Failed to handle a netlink uevent event: " ) + ex.what() ) );
    }
}

void NetlinkManager::handleUSBUevent( const UEvent& uevent ) {
    if( uevent.action == UEvent::Actions::ADD ) {
        handleUSBAddUevent( uevent );
    }
    else if( uevent.action == UEvent::Actions::REMOVE ) {
        handleUSBRemoveUevent( uevent );
    }
}

void NetlinkManager::handleUSBAddUevent( const UEvent& uevent ) {
    const auto it = uevent.optionalFields.find( "DEVTYPE" );

    if( it != uevent.optionalFields.end() && it->second == "usb_device" ) {
        issueUSBAddHotPlugEvent( uevent.devpath );
    }
}

void NetlinkManager::handleUSBRemoveUevent( const UEvent& uevent ) {
    const auto it = uevent.optionalFields.find( "DEVTYPE" );

    if( it != uevent.optionalFields.end() && it->second == "usb_device" ) {
        issueUSBRemoveHotPlugEvent( uevent.devpath );
    }
}

void NetlinkManager::issueUSBAddHotPlugEvent( const std::string& devicePath ) {
    Log::debug( cc::debug( "A new USB device has been plugged: " + devicePath ) );

    m_netlinkEventsHandler.handleHotPlugEvent( { HotPlugEventDeviceClass::USB, HotPlugEventType::ADD, devicePath } );
    m_dbClient.insert( NetlinkUSBDevicesDBName, JSON( db::ID_FIELD_NAME << devicePath ) );
}

void NetlinkManager::issueUSBRemoveHotPlugEvent( const std::string& devicePath ) {
    Log::debug( cc::debug( "An USB device has been unplugged: " + devicePath ) );

    m_netlinkEventsHandler.handleHotPlugEvent( { HotPlugEventDeviceClass::USB, HotPlugEventType::REMOVE, devicePath } );
    m_dbClient.remove( NetlinkUSBDevicesDBName, JSON( db::ID_FIELD_NAME << devicePath ) );
}

void NetlinkManager::handleI2CUevent( const UEvent& uevent ) {
    if( uevent.action == UEvent::Actions::ONLINE ) {
        handleI2COnlineUevent( uevent );
    }
}

/* keep in sync with h2_common/fixes/target/linux/sunxi/patches-4.19/913-try-to-recover-locked-i2c-bus.patch */
void NetlinkManager::handleI2COnlineUevent( const UEvent& uevent ) {
    m_netlinkEventsHandler.handleI2CBusRecovered( uevent.devpath );
}

}  // namespace HardwareManager
}  // namespace HubPlatform
