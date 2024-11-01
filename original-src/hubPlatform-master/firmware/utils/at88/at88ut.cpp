#include "at88/at88ut.h"
#include "at88/at88sc.h"

#include "base_utils/hubtypes.h"
#include "base_utils/utils.h"

#include "common_utils/firmware_info_helper.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>

using namespace utils;

// use this option for development purposes only
#define __DEBUG_IS_NEEDED__ 0
//

#if( __DEBUG_IS_NEEDED__ )
#define DEBUG_OUT( x ) std::cout << ( x ) << std::endl << std::flush;
#else
#define DEBUG_OUT( x ) (void) ( x )
#endif

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

std::string getVeraBoxSerial() {
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    return getVeraProvisionReader().getSerial();
#else
    return {};
#endif
}

std::string getVeraBoxMac() {
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    return getVeraProvisionReader().getMacAddressString();
#else
    return {};
#endif
}

std::string getVeraHwKey() {
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    return getVeraProvisionReader().getHwKey();
#else
    return {};
#endif
}

std::string getBoxA20SerialVersion1() {
    At88sc at88sc;
    const std::string hubSerial = at88sc.getSerial();
    return hubSerial;
}

std::string getBoxA20MACVersion1() {
    At88sc at88sc;
    const std::string hubMAC = at88sc.getMac();
    return hubMAC;
}

std::string getBoxH2PlusSerialVersion1() {
    std::string serialResult = readPipe( "cat /proc/cpuinfo | grep Serial | awk '{print $3}'" );
    constexpr int SERIAL_LENGTH_ON_ARMBIAN = 20;
    constexpr int HUB_SERIAL_LENGTH = 16;
    constexpr char HUB_SERIAL_PREFIX[] = "h2p1";
    if( serialResult.length() > 0 ) {
        // remove last character after readPipe - \n symbol
        serialResult.pop_back();
        if( serialResult.length() > SERIAL_LENGTH_ON_ARMBIAN )
            serialResult.resize( SERIAL_LENGTH_ON_ARMBIAN );
        serialResult.erase( 0, serialResult.length() - ( HUB_SERIAL_LENGTH - strlen( HUB_SERIAL_PREFIX ) ) );
        serialResult.insert( 0, HUB_SERIAL_PREFIX );
    }
    return serialResult;
}

std::string getBoxH2PlusMACVersion1() {
    std::string macResult = readPipe( "ifconfig eth0 | awk '/HWaddr/ {print $NF}'" );
    macResult.resize( 17 );
    return macResult;
}

std::string getHubType() {
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    DEBUG_OUT( stringify( "Guess hubtype: ", BoxHubTypeBuildVersion ) );
    return BoxHubTypeBuildVersion;
#endif

    const std::string hubtypeFilePath = "/etc/hubtype";
    std::ifstream hubtypeFile( hubtypeFilePath );
    std::string hubType;

    // try to determine hub type by file
    if( hubtypeFile.good() ) {
        std::getline( hubtypeFile, hubType );
        DEBUG_OUT( stringify( "Hub type detected by file: ", hubType ) );
    }
    else {
        // try to determine hub type indirectly
        const std::string popenCpuPartCommand = "cat /proc/cpuinfo | grep part | awk '{print $4}'";
        const std::string popenCpuRevisionCommand = "cat /proc/cpuinfo | grep revision | awk '{print $4}'";
        const std::string popenCpuHardwareCommand = "cat /proc/cpuinfo | grep Hardware | awk '{print $3 $4}'";
        const std::string popenCpuSerialCommand = "cat /proc/cpuinfo | grep Serial | awk '{print $3}'";

        const std::string cpuPart = readPipe( popenCpuPartCommand );
        const std::string cpuRevision = readPipe( popenCpuRevisionCommand );
        const std::string cpuHardware = readPipe( popenCpuHardwareCommand );
        const std::string cpuSerial = readPipe( popenCpuSerialCommand );
        DEBUG_OUT( "Try to detect hub type by /proc/cpuinfo..." );
        DEBUG_OUT( stringify( "CPU part: ", cpuPart ) );
        DEBUG_OUT( stringify( "CPU revision: ", cpuRevision ) );
        DEBUG_OUT( stringify( "CPU hardware: ", cpuHardware ) );
        DEBUG_OUT( stringify( "CPU serial: ", cpuSerial ) );

        // TODO rework with more deterministic approach here later
        if( cpuPart == "0xc07" && cpuRevision == "4" && cpuHardware == "sun7i" ) {
            hubType = BoxA20HubTypeVersion1;
        }
        else if( cpuPart == "0xc07" && cpuRevision == "5" &&
                 ( cpuHardware == "sun8i" || cpuHardware == "Allwinnersun8i" ) ) {
            hubType = BoxH2PlusHubTypeVersion1;
        }

        DEBUG_OUT( stringify( "Guess hubtype: ", hubType ) );
    }
    return hubType;
}

std::string getHubSerial( const std::string& hubType ) {
    if( hubType == BoxH2PlusHubTypeVersion1 ) {
        return getBoxH2PlusSerialVersion1();
    }
    else if( hubType == BoxA20HubTypeVersion1 ) {
        return getBoxA20SerialVersion1();
    }
    else if( hubType == BoxG150HubTypeVersion1 || hubType == BoxG450HubTypeVersion1 ||
             hubType == BoxG550HubTypeVersion1 ) {
        return getVeraBoxSerial();
    }

    return std::string();
}

std::string getHubMAC( const std::string& hubType ) {
    if( hubType == BoxH2PlusHubTypeVersion1 ) {
        return getBoxH2PlusMACVersion1();
    }
    else if( hubType == BoxA20HubTypeVersion1 ) {
        return getBoxA20MACVersion1();
    }
    else if( hubType == BoxG150HubTypeVersion1 || hubType == BoxG450HubTypeVersion1 ||
             hubType == BoxG550HubTypeVersion1 ) {
        return getVeraBoxMac();
    }

    return std::string();
}

std::string getFirmwareVersion() {
    return FirmwareInfoHelper::getFirmwareVersionString();
}

std::string getHubHwKey( const std::string& hubType ) {
    if( hubType == BoxG150HubTypeVersion1 || hubType == BoxG450HubTypeVersion1 || hubType == BoxG550HubTypeVersion1 ) {
        return getVeraHwKey();
    }
    return std::string();
}
