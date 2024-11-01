#include "check_time_change.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include <sys/sysinfo.h>
#include <sys/timerfd.h>
#ifdef TFD_TIMER_CANCEL_ON_SET
#include <climits>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif

namespace HubPlatform {
namespace HardwareManager {
namespace Utils {

#ifdef TFD_TIMER_CANCEL_ON_SET
const bool HasTimerExitOnClose = true;
#else
const bool HasTimerExitOnClose = false;
#endif

/** @returns    -1 - not available in this build;
 *              0 - timer exited successfully;
 *              1 - timer exited on value set, time has changed */
int checkTimeChange() {
#ifdef TFD_TIMER_CANCEL_ON_SET
    int fd = timerfd_create( CLOCK_REALTIME, 0 );
    struct itimerspec timerSpec = {};
    timerSpec.it_value.tv_sec = INT_MAX;

    int result = 0;
    timerfd_settime( fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &timerSpec, NULL );
    char buffer[ 10 ];
    if( -1 == read( fd, &buffer, 10 ) ) {
        if( errno == ECANCELED ) {
            LOGGER_INFO( cc::info( "Timer cancelled - system clock changed" ) );
            result = 1;
        }
        else
            LOGGER_ERROR( cc::error( "Some error: " ) + strerror( errno ) );
    }
    close( fd );
    return result;
#else
    // TFD_TIMER_CANCEL_ON_SET is not defined for that toolchain version
    return -1;
#endif
}

long getSystemUptime() {
    struct sysinfo info = {};
    int error = sysinfo( &info );
    if( error != 0 ) {
        LOGGER_ERROR( cc::error( "Error = " + cc::num10( error ) ) );
    }
    return info.uptime;
}

}  // namespace Utils
}  // namespace HardwareManager
}  // namespace HubPlatform