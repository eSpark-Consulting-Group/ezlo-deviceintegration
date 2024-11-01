#include "uci_utils/uci_dhcp_file.h"

#include "log_utils/ezlo_log.h"
#include "system_utils/system.h"
#include "tiny/tiny_consolecolors.h"

#include <algorithm>

namespace HubPlatform {
namespace UciUtils {

namespace {
constexpr char DhcpConfigFile[] = "/etc/config/dhcp";
}

UciDhcpFile::UciDhcpFile( std::string fileName )
: UciFile( std::move( fileName ) ) {}

UciDhcpFile::UciDhcpFile()
: UciFile( DhcpConfigFile ) {}

bool UciDhcpFile::apply() {
    auto result = System::getCommandOutput( "/etc/init.d/dnsmasq reload" );
    if( result.second != 0 ) {
        Log::error( cc::error( "Failed to reload network config: " ) + cc::note( result.first ) );
        return false;
    }

    result = System::getCommandOutput( "/etc/init.d/odhcpd reload" );
    if( result.second != 0 ) {
        Log::error( cc::error( "Failed to reload network config: " ) + cc::note( result.first ) );
        return false;
    }

    return true;
}

}  // namespace UciUtils
}  // namespace HubPlatform
