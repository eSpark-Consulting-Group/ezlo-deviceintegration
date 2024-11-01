#include "migration_tool/system_repair/hostname_fix.h"

#include "common_utils/hub_info_helper.h"
#include "system_utils/system.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace MigrationTool {

const char* HostnameFix::getName() const {
    return "hostname fix";
}

bool HostnameFix::shouldApply() const {
    m_expectedHostname = getExpectedHostname();
    if( m_expectedHostname.empty() )
        return false;

    if( !m_systemSettings.load() )
        return false;

    auto currentHostname = m_systemSettings.getHostname();
    if( currentHostname == m_expectedHostname ) {
        Log::debug( cc::debug( "System hostname is good" ) );
        return false;
    };

    Log::debug( cc::debug( "System hostname differs from expected" ) );
    return true;
}

bool HostnameFix::apply() {
    if( !m_systemSettings.setHostname( m_expectedHostname ) )
        return false;

    if( !m_systemSettings.save() )
        return false;

    if( !m_systemSettings.apply() )
        return false;

    std::string command = "echo \"" + m_expectedHostname + "\" > /proc/sys/kernel/hostname";
    return ( System::getCommandOutput( command.c_str() ).second == 0 );
}

std::string HostnameFix::getExpectedHostname() const {
    return "HUB" + HubInfoHelper::getSerial();
}

}  // namespace MigrationTool
}  // namespace HubPlatform
