#include "migration_tool/system_repair/firewall_fix.h"

#include "system_utils/system.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace MigrationTool {

const char* FirewallFix::getName() const {
    return "firewall fix";
}

bool FirewallFix::shouldApply() const {
    return ( System::getCommandOutput( "grep -e'input.*REJECT' /etc/config/firewall" ).second == 0 );
}

bool FirewallFix::apply() {
    if( System::getCommandOutput( "sed -i 's/\\(input.*\\)REJECT/\\1ACCEPT/g' /etc/config/firewall" ).second != 0 ) {
        return false;
    }
    return ( System::getCommandOutput( "/etc/init.d/firewall restart" ).second == 0 );
}

}  // namespace MigrationTool
}  // namespace HubPlatform
