#include "vhub_manager/vhub_manager.h"

#include "system_utils/environment.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

void storeLogsToFile() {
    auto logPath = HubPlatform::Environment::getLogsFolder() + "/vhub_manager.log";

    if( !freopen( logPath.c_str(), "w", stderr ) ) {
        Log::warn( cc::warn( "Failed to redirect stderr to logfile" ) );
        return;
    }
    if( !freopen( logPath.c_str(), "w", stdout ) ) {
        Log::warn( cc::warn( "Failed to redirect stdout to logfile" ) );
        return;
    }
}

int main() {
    storeLogsToFile();
    HubPlatform::VhubManager::VhubManager vhub_manager;
    vhub_manager.run();
    return 0;
}
