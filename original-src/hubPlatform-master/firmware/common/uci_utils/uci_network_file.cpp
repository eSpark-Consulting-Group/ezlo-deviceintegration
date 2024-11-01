#include "uci_utils/uci_network_file.h"

#include "log_utils/ezlo_log.h"
#include "system_utils/system.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace UciUtils {

namespace {
constexpr char NetworkConfigFile[] = "/etc/config/network";
}

UciNetworkFile::UciNetworkFile( std::string fileName )
: UciFile( std::move( fileName ) ) {}

UciNetworkFile::UciNetworkFile()
: UciFile( NetworkConfigFile ) {}

bool UciNetworkFile::apply() {
    auto result = System::getCommandOutput( "/etc/init.d/network reload" );
    if( result.second != 0 ) {
        Log::error( cc::error( "Failed to reload network config: " ) + cc::note( result.first ) );
        return false;
    }
    return true;
}

}  // namespace UciUtils
}  // namespace HubPlatform
