
#include "uid/ezlo.h"
#include "uid/firmware.h"
#include "uid/nma_client.h"
#include "uid/ui_exceptions.h"
#include "uid/ui_service.h"

#include "log_utils/ezlo_log.h"

#include "system_utils/environment.h"

#include "base_utils/timeutils.h"
#include "database_client/db_tables_names.h"
#include "base_utils/utils.h"
#include "bus/spread_bus.h"
#include "base_utils/ezlodb.h"

#include "common_utils/hub_info_helper.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"

#include <fstream>
#include <vector>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#if( !defined SIOCGIFHWADDR )
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if_dl.h>
#endif
#include <unistd.h>

using namespace HubPlatform;
using namespace HubPlatform::HardwareManager;

BaseBus* Ezlo::m_bus = nullptr;

BaseBus& Ezlo::getBus() {
    return *m_bus;
}

BaseBus* Ezlo::getBusPtr() {
    return m_bus;
}

void Ezlo::setBus( BaseBus* bus ) {
    m_bus = bus;
}

void Ezlo::setDescription( const std::string& description ) {
    Db::update( EzloPropertyDBName, JSON_EMPTY_OBJ, JSON( "description" << description ), true );
}

std::string Ezlo::getDescription() {
    Json description = Db::findOne( EzloPropertyDBName, JSON_EMPTY_OBJ );
    if( description.count( "description" ) < 1 )
        description = Db::findOne( SysOwnerDBName, JSON_EMPTY_OBJ );
    return json_get_str( description, "description" );
}

std::string Ezlo::getZwaveCalibration() {
    std::string calibrationInfo;
    static const int defaultNvrSize = 247;
    static const std::string nvrFilename = "/opt/firmware/zw-flash/nvr.bin.factory";
    std::ifstream nvrFactory( nvrFilename, std::ios::binary );
    std::streampos fileSize;
    nvrFactory.seekg( 0, std::ios::end );
    fileSize = nvrFactory.tellg();
    nvrFactory.seekg( 0, std::ios::beg );
    if( fileSize < defaultNvrSize )
        return "no NVR factory data";
    std::vector< char > fileData( fileSize );
    nvrFactory.read( (char*) &fileData[ 0 ], fileSize );
    calibrationInfo += "CCAL: 0x" + intToHexString( fileData[ 0x08 ], 2 );
    calibrationInfo += ", TXCAL1: 0x" + intToHexString( fileData[ 0x28 ], 2 );
    calibrationInfo += ", TXCAL2: 0x" + intToHexString( fileData[ 0x29 ], 2 );
    return calibrationInfo;
}

JsonArray Ezlo::getHubErrors() {
    auto arr = JSON_EMPTY_ARR;

    auto result = Db::findAll( HubErrorsDBName, JSON_EMPTY_OBJ );
    for( auto& element : result ) {
        auto errorIt = element.find( "_id" );
        element[ "id" ] = errorIt.value();
        element.erase( errorIt );
        arr.push_back( element );
    }
    return arr;
}

void Ezlo::killSSH( uint time ) {
    sleep( time * 60 );
    system( "killall ssh" );
    Log::debug( cc::debug( "All SSH processes were" ) + cc::error( "killed" ) );
}

JsonObject Ezlo::getRuleInfo( const std::string ruleId ) {
    Json fieldsToReturn = JSON( "name" << 1 << "parent_id" << 1 << "_id" << 0 );
    Json ruleJson = Db::findOne( "db.firmware.scenes_manager.rules", Db::getIdObj( ruleId ), &fieldsToReturn );
    return ruleJson;
}

bool Ezlo::runScript( const std::string name, JsonObject& o ) {
    std::string request = "/opt/firmware/scripts/" + name + ".sh";
    Log::debug( cc::debug( "Run script: " ) + cc::warn( request ) );
    int result = std::system( request.c_str() );
    if( result ) {
        Log::error( cc::error( "Script error code: " ) + cc::c( result ) );
        return false;
    }
    return true;
}

std::string Ezlo::serial() {
    return HubInfoHelper::getSerial();
}

std::string Ezlo::uuid() {
    return HubInfoHelper::getControllerUUID();
}

const std::string& Ezlo::hubType() {
    static const auto hubType = evalHubType();
    return hubType;
}

const std::string& Ezlo::macAddress() {
    static const auto macAddress = HubInfoHelper::getMacAddress();
    return macAddress;
}

const std::string& Ezlo::getModel() {
    // TODO: map hub type to model
    return hubType();
}

int Ezlo::getTimeOffset() {
    return TimeUtils::getCurrentOffset();
}

JsonObject Ezlo::getMacs() {
    return JSON( "ethMac" << getMac( "eth0" ) << "wlanMac" << getMac( "wlan0" ) );
}

bool Ezlo::isConnected() {
    auto nmaClient = HubPlatform::Uid::Service::getInstance().getNmaClient();
    return nmaClient && nmaClient->isConnected() && nmaClient->isHubRegistered();
}

std::string Ezlo::getMedia() {
    std::ifstream carrier( "/sys/class/net/eth0/carrier" );
    char carrierType = '\0';
    carrier >> carrierType;
    const std::string media( carrierType == '1' ? "cable" : "radio" );

    return media;
}

std::string Ezlo::evalHubType() {
    return HubInfoHelper::getHubType();
}

// FIXME DEPRECATED
std::string Ezlo::getMac( const std::string& interface ) {
    std::string mac;
#if( defined SIOCGIFHWADDR )
    struct ifreq ifreq;
    std::vector< char > buf( 3 );
    int fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
    strcpy( ifreq.ifr_name, interface.c_str() );
    if( ioctl( fd, SIOCGIFHWADDR, &ifreq ) == 0 ) {
        for( int i = 0; i < 6; ++i ) {
            sprintf( buf.data(), "%02x:", (unsigned char) ifreq.ifr_addr.sa_data[ i ] );
            mac.append( std::string( buf.begin(), buf.end() ) );
        }
        mac.pop_back();
    }
    close( fd );
#else
    ifaddrs* iflist;
    // bool found = false;
    if( getifaddrs( &iflist ) == 0 ) {
        for( ifaddrs* cur = iflist; cur; cur = cur->ifa_next ) {
            if( ( cur->ifa_addr->sa_family == AF_LINK ) && ( cur->ifa_name == interface ) && cur->ifa_addr ) {
                sockaddr_dl* sdl = (sockaddr_dl*) cur->ifa_addr;
                mac.insert( mac.end(), LLADDR( sdl ), ( LLADDR( sdl ) ) + sdl->sdl_alen );
                // found = true;
                break;
            }
        }

        freeifaddrs( iflist );
    }
#endif
    return mac;
}

void Ezlo::reboot() {
    system( "/sbin/reboot" );
}
