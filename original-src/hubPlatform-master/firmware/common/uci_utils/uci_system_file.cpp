#include "uci_utils/uci_system_file.h"

#include "log_utils/ezlo_log.h"
#include "system_utils/system.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace UciUtils {

namespace {
constexpr char SystemConfigFile[] = "/etc/config/system";
}

UciSystemFile::UciSystemFile( std::string fileName )
: UciFile( std::move( fileName ) ) {}

UciSystemFile::UciSystemFile()
: UciFile( SystemConfigFile ) {}

bool UciSystemFile::setHostname( const std::string& hostname ) {
    auto systemSectionIt = getSystemSection();
    if( systemSectionIt == m_settings.end() ) {
        return false;
    }
    auto& options = systemSectionIt->m_singleOptions;
    options[ "hostname" ] = hostname;
    return true;
}

bool UciSystemFile::setTimeZone( const std::string& zonename ) {
    auto systemSectionIt = getSystemSection();
    if( systemSectionIt == m_settings.end() ) {
        return false;
    }
    auto& options = systemSectionIt->m_singleOptions;
    options[ "zonename" ] = zonename;
    options.erase( "timezone" );
    return true;
}

bool UciSystemFile::apply() {
    auto result = System::getCommandOutput( "/etc/init.d/system reload" );
    if( result.second != 0 ) {
        Log::error( cc::error( "Failed to reload system config: " ) + cc::note( result.first ) );
        return false;
    }
    return true;
}

bool UciSystemFile::isTimezoneDefault() const {
    auto systemSectionIt = getSystemSection();
    if( systemSectionIt == m_settings.end() ) {
        return true;
    }
    auto& options = systemSectionIt->m_singleOptions;
    return options.find( "timezone" ) != options.end();
}

std::string UciSystemFile::getHostname() const {
    auto systemSectionIt = getSystemSection();
    if( systemSectionIt == m_settings.end() ) {
        return {};
    }
    auto& options = systemSectionIt->m_singleOptions;
    auto hostnameIt = options.find( "hostname" );
    if( hostnameIt != options.end() )
        return hostnameIt->second;

    Log::error( cc::note( getFileName() ) + cc::error( " contains no hostname system option" ) );
    return {};
}

std::string UciSystemFile::getTimezone() const {
    auto systemSectionIt = getSystemSection();
    if( systemSectionIt == m_settings.end() ) {
        return {};
    }
    auto& options = systemSectionIt->m_singleOptions;
    auto timezoneIt = options.find( "timezone" );
    if( timezoneIt != options.end() )
        return timezoneIt->second;

    auto zonenameIt = options.find( "zonename" );
    if( zonenameIt != options.end() )
        return zonenameIt->second;

    Log::error( cc::note( getFileName() ) + cc::error( " contains neither timezone nor zonename system option" ) );
    return {};
}

UciSettings::iterator UciSystemFile::getSystemSection() {
    return getSection( "system", "" );
}

UciSettings::const_iterator UciSystemFile::getSystemSection() const {
    return getSection( "system", "" );
}

}  // namespace UciUtils
}  // namespace HubPlatform
