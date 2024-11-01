#include "migration_tool/system_repair/dhcp_fix.h"

#include "system_utils/system.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {
namespace MigrationTool {

const char* DhcpFix::getName() const {
    return "DHCP fix";
}

bool DhcpFix::shouldApply() const {
    return ( System::getCommandOutput( "uci get dhcp.lan.dhcpv6" ).second == 0 );
}

bool DhcpFix::apply() {
    if( System::getCommandOutput( "uci -q batch <<-EOF >/dev/null\n"
                                  "        delete dhcp.lan\n"
                                  "        set dhcp.lan=dhcp\n"
                                  "        set dhcp.lan.ignore=1\n"
                                  "        set dhcp.lan.interface=lan\n"
                                  "        reorder dhcp.lan=1\n"
                                  "        commit dhcp\n"
                                  "EOF" )
            .second != 0 ) {
        return false;
    }
    return ( System::getCommandOutput( "/etc/init.d/odhcpd restart" ).second == 0 );
}

}  // namespace MigrationTool
}  // namespace HubPlatform
