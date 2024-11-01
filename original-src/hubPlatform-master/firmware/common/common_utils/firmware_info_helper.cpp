#include "common_utils/firmware_info_helper.h"
#include "common_utils/firmware_package_version.h"
#include "provision_data_utils/provision_data_manager.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"

#include <fstream>
#include <unistd.h>
#include <iostream>
#include <regex>

namespace FirmwareInfoHelper {

constexpr const char UnknownVersionStr[] = "unknown";

const HubPlatform::Firmware::Version& getFirmwareVersion() {
    static HubPlatform::Firmware::Version version( getFirmwareVersionString() );
    return version;
}

std::string getFirmwareVersionString() {
    std::string ver{ UnknownVersionStr };
    try {
        HubPlatform::ConfigReader configReader;
        if( configReader.hasValue( HubPlatform::ConfigKeys::fwVersion ) ) {
            ver = configReader.getString( HubPlatform::ConfigKeys::fwVersion );
        }
    }
    catch( const std::exception& exception ) {
        std::cerr << "Exception occured while reading firmware version(" << typeid( exception ).name()
                  << "): " << exception.what() << std::endl;
    }
    catch( ... ) {
        std::cerr << "Unknown exception occured while reading firmware version" << std::endl;
    }

    // try to read it from firmware.cfg settings file ^^^ , if failed and still unknown - read it from disk
    if( ver == UnknownVersionStr ) {
        std::ifstream fi( "/etc/firmware_version" );
        if( fi ) {
            std::string line;
            if( getline( fi, line ) )
                ver = line;
            fi.close();
        }
    }

    if( ver == UnknownVersionStr ) {
        ver = HubPlatform::CommonUtils::Firmware::getVersion();
    }
    return ver;
}

std::string getSystemVersionString() {
    std::ifstream file( "/etc/openwrt_release" );
    if( file ) {
        std::regex regex( "DISTRIB_RELEASE='(.*)'" );
        std::string line;
        std::smatch match;

        while( file >> line ) {
            std::regex_match( line, match, regex );
            if( !match.empty() ) {
                return match[ 1 ];
            }
        }
    }

    return "unknown";
}

std::string getFirmwareRegion() {
    auto& data = HubPlatform::ProvisionDataUtils::getManager().getProvisionData();
    if( !data.zwaveRegion.empty() ) {
        return data.zwaveRegion;
    }
    std::cerr << "Firmware region not available" << std::endl;
    return { "unknown" };
}

}  // namespace FirmwareInfoHelper
