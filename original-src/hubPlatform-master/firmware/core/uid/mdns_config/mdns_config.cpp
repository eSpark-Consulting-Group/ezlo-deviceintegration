#include "uid/mdns_config/mdns_config.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include <fstream>

namespace HubPlatform {

bool MdnsConfig::replaceString( std::string& str, const std::string& toReplace, const std::string& replaceWith ) {
    bool replaced = false;
    size_t pos;
    while( ( pos = str.find( toReplace ) ) != std::string::npos ) {
        str.replace( pos, toReplace.size(), replaceWith );
        replaced = true;
    }
    return replaced;
}

bool MdnsConfig::setAvahiHost( const std::string& fileName ) {
    std::string config;
    {
        std::ifstream configFile( fileName );
        if( !configFile.is_open() ) {
            Log::warn( cc::warn( "Can not open file " ) + cc::note( fileName ) );
            return false;
        }

        configFile.seekg( 0, std::ios::end );
        config.reserve( size_t( configFile.tellg() ) );
        configFile.seekg( 0, std::ios::beg );
        config.assign( ( std::istreambuf_iterator< char >( configFile ) ), std::istreambuf_iterator< char >() );
    }

    // [server]
    // host-name=hub -> host-name=HUB70010195
    if( !replaceString( config, "hub", "HUB" + HubInfoHelper::getSerial() ) ) {
        return true;
    }

    {
        std::ofstream configFile( fileName, std::ofstream::trunc );
        if( !configFile.is_open() ) {
            Log::warn( cc::warn( "Can not open file " ) + cc::note( fileName ) );
            return false;
        }
        configFile << config;
    }
    return true;
}

bool MdnsConfig::setAvahiService( int localServerPort, const std::string& avahiServicesPath ) {
    const std::string& avahiHubServiceFile = avahiServicesPath + "/hub.service";
    std::string avahiConfig = "<?xml version=\"1.0\" standalone=\'no\'?><!--*-nxml-*-->\n"
                              "<!DOCTYPE service-group SYSTEM \"avahi-service.dtd\">\n"
                              "<service-group>\n"
                              "  <name replace-wildcards=\"yes\">${vendor} ${hubType} controller (${serial})</name>\n"
                              "  <service protocol=\"ipv4\">\n"
                              "    <type>_ezlo._tcp</type>\n"
                              "    <domain-name>local</domain-name>\n"
                              "    <port>${port}</port>\n"
                              "    <txt-record>Serial=${serial}</txt-record>\n"
                              "    <txt-record>Firmware Version=${version}</txt-record>\n"
                              "    <txt-record>Vendor=${vendor}</txt-record>\n"
                              "    <txt-record>Hub Type=${hubType}</txt-record>\n"
                              "    <txt-record>Mac=${mac}</txt-record>\n"
                              "  </service>\n"
                              "</service-group>\n";
    replaceString( avahiConfig, "${vendor}", "eZLO" );
    replaceString( avahiConfig, "${serial}", HubInfoHelper::getSerial() );
    replaceString( avahiConfig, "${port}", std::to_string( localServerPort ) );
    replaceString( avahiConfig, "${version}", FirmwareInfoHelper::getFirmwareVersionString() );
    replaceString( avahiConfig, "${hubType}", HubInfoHelper::getHubType() );
    replaceString( avahiConfig, "${mac}", HubInfoHelper::getMacAddress() );
    std::ofstream avahiFile( avahiHubServiceFile );
    if( !avahiFile.good() ) {
        LOGGER_WARN( "avahi configuration write fail" );
        return false;
    }
    avahiFile << avahiConfig;
    LOGGER_WARN( "avahi configuration write success" );
    return true;
}

bool MdnsConfig::runMdns( int localServerPort ) {
#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    const std::string& avahiServicesPath = Environment::getConfigsFolder() + "/avahi/services";
    filesystem::createFolder( avahiServicesPath );
    setAvahiService( localServerPort, avahiServicesPath );
#else
    const std::string& avahiHostConfigPath = "/etc/avahi/avahi-daemon.conf";
    const std::string& avahiServicesPath = "/etc/avahi/services";
    setAvahiHost( avahiHostConfigPath );
    setAvahiService( localServerPort, avahiServicesPath );
    const std::string command = "/etc/init.d/avahi-daemon restart";
    if( std::system( command.c_str() ) != 0 ) {
        LOGGER_WARN( "avahi restart failed" );
        return false;
    }
    LOGGER_INFO( "avahi is restarted" );
#endif
    return true;
}

}  // namespace HubPlatform
