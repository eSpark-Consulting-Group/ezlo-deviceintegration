#include "uci_utils/uci_wireless_file.h"

#include "log_utils/ezlo_log.h"
#include "system_utils/system.h"
#include "tiny/tiny_consolecolors.h"

#include <thread>
#include <chrono>

namespace HubPlatform {
namespace UciUtils {

namespace {
constexpr char NetworkConfigFile[] = "/etc/config/wireless";
}

UciWirelessFile::UciWirelessFile( std::string fileName )
: UciFile( std::move( fileName ) ) {}

UciWirelessFile::UciWirelessFile()
: UciFile( NetworkConfigFile ) {}

bool UciWirelessFile::apply( bool hard ) {
    bool ok = true;
#if defined( __HUB_TYPE_G150__ )
    if( hard ) {
        system( "ifconfig apcli0 down" );
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    }
#endif
    auto result = System::getCommandOutput( "wifi" );
    if( result.second != 0 ) {
        Log::error( cc::error( "Failed to reload wireless config: " ) + cc::note( result.first ) );
        ok = false;
    }
#if defined( __HUB_TYPE_G150__ )
    if( hard ) {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        system( "ifconfig apcli0 up" );
    }
#endif
    return ok;
}

}  // namespace UciUtils
}  // namespace HubPlatform
