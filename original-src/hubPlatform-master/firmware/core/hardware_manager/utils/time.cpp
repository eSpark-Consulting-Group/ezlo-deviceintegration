#include "hardware_manager/utils/time.h"

#include "system_utils/filesystem.h"
#include "system_utils/environment.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"

#include <cstdlib>
#include <fstream>
#include <typeinfo>

extern "C" {
#include <time.h>
}

namespace HubPlatform {
namespace HardwareManager {
namespace Utils {

namespace {
const std::string ntpRestartCommand = "/etc/init.d/sysntpd restart";

// NOTE: 8 hour NTP sync is a minimal sync interval required by T-Mobile certification
constexpr unsigned int ntpSyncIntervalSeconds = 8 * 60 * 60;

std::string ntpLastSyncFile() {
    return Environment::getTempDataFolder() + "/ntp_last_sync";
}

}  // namespace

void syncNTPTime( bool force ) {
#if !defined( __HUB_TYPE_H2_SECURE__ ) && !defined( __HUB_TYPE_H2__ ) && !defined( __HUB_TYPE_BEEWAVE__ )
    return;
#endif

    try {
        if( force ) {
            LOGGER_INFO( cc::info( "Run NTP time sync" ) );
            std::system( ntpRestartCommand.c_str() );
            return;
        }

        if( filesystem::isFileExist( ntpLastSyncFile() ) ) {
            size_t lastSync = 0;
            std::ifstream file( ntpLastSyncFile() );
            if( !file || !( file >> lastSync ) )
                return;

            struct timespec ts;
            clock_gettime( CLOCK_MONOTONIC, &ts );

            if( ts.tv_sec > lastSync + ntpSyncIntervalSeconds ) {
                LOGGER_INFO( cc::info( "Run NTP time sync" ) );
                std::system( ntpRestartCommand.c_str() );
            }
        }
    }
    catch( const std::exception& e ) {
        auto error = "( " + std::string( typeid( e ).name() ) + " ): " + e.what();
        LOGGER_ERROR( cc::error( "Failed to sync NTP time: " ) + cc::note( error ) );
    }
}

}  // namespace Utils
}  // namespace HardwareManager
}  // namespace HubPlatform
