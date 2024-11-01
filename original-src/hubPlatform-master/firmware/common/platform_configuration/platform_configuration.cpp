#include "platform_configuration/platform_configuration.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "json_utils/json_utils.h"

#include <fstream>
#include <cassert>

namespace HubPlatform {

namespace {
constexpr const char PlatformConfigurationFile[] = "/etc/config/hub_platform/platform_configuration.json";
}

const PlatformConfiguration& PlatformConfiguration::getPlatformConfiguration() {
    static const PlatformConfiguration platformConfig;
    return platformConfig;
}

PlatformConfiguration::PlatformConfiguration() {
    try {
        std::ifstream input( PlatformConfigurationFile );
        m_platformConfiguration = Json::parse( input );
    }
    catch( std::exception const& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Could not parse platform configuration file " ) +
                          cc::note( PlatformConfigurationFile ) + cc::error( ", " ) + cc::note( ex.what() ) );
    }

    if( m_platformConfiguration.empty() ) {
        LOGGER_FATAL_RAW( cc::fatal( "Platform configuration is absent" ) );
    }
}

PlatformConfiguration::DeviceName PlatformConfiguration::getZwaveDeviceName() const {
    return json_get_str( m_platformConfiguration, "zwave.stick.device.name" );
}

PlatformConfiguration::DeviceName PlatformConfiguration::getZigBeeDeviceName() const {
    return json_get_str( m_platformConfiguration, "zigbee.stick.device.name" );
}

PlatformConfiguration::DeviceName PlatformConfiguration::getUserPartitionDeviceName() const {
    return json_get_str( m_platformConfiguration, "user.partition.device.name" );
}

PlatformConfiguration::ProgramPath PlatformConfiguration::getBootloaderUpdaterPath() const {
    return json_get_str( m_platformConfiguration, "hub.bootloader.updater.path" );
}

PlatformConfiguration::ProgramPath PlatformConfiguration::getRecoveryUpdaterPath() const {
    return json_get_str( m_platformConfiguration, "hub.recovery.updater.path" );
}

}  // namespace HubPlatform
