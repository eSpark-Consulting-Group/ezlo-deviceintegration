#include "system_monitor/service.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

int main( int argc, char* argv[] ) {
    LOGGER_INFO_RAW( cc::info( "Monitoring started" ) );
    auto exitcode = HubPlatform::SystemMonitor::Service::serviceInstance().run();
    LOGGER_INFO_RAW( cc::info( "Monitoring finished" ) );
    return exitcode;
}
