#include "utils.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "base_utils/timeutils.h"

#include <fstream>

#include <unistd.h>
#include <signal.h>

namespace HubPlatform {
namespace SystemMonitor {
namespace Utils {

int launchCommand( std::string command ) {
    const int ExeclExitError{ 127 };

    int pid = ::fork();
    if( pid == 0 ) {
        ::execl( "/bin/sh", "sh", "-c", command.c_str(), nullptr );
        LOGGER_ERROR( cc::error( "Unable to run exec for command: " ) + cc::note( command ) );
        ::_exit( ExeclExitError );  // will be called only in case of execl() error
    }

    return pid;
}

bool isProcessRunning( int pid ) {
    if( pid <= 0 )
        return false;

    if(::kill( pid, 0 ) == 0 )
        return true;

    // Exists but does not have permissions for signals
    if( errno == EPERM )
        return true;

    return false;
}

void sendTerminateSignal( int pid ) {
    ::kill( pid, SIGTERM );
}

void storeProcessPid( int pid, std::string pidfile ) {
    std::ofstream file( pidfile );
    if( !file.is_open() )
        LOGGER_ERROR( cc::error( "Unable to store PID to: " ) + cc::note( pidfile ) );
    file << pid;
    file.close();
}

int loadProcessPid( std::string pidfile ) {
    int pid = 0;
    std::ifstream file( pidfile );
    if( !file.is_open() )
        LOGGER_ERROR( cc::error( "Unable to load PID from: " ) + cc::note( pidfile ) );
    file >> pid;
    file.close();
    return pid;
}

void addLogHeader( std::string logfile ) {
    std::ofstream file( logfile, std::ios::app );
    if( !file.is_open() )
        LOGGER_ERROR( cc::error( "Unable to add timestamp to log: " ) + cc::note( logfile ) );
    file << "\n----------PROCESS_STARTED_AT=" << TimeUtils::getCurrentIsoDate() << "----------\n";
    file.close();
}

}  // namespace Utils
}  // namespace SystemMonitor
}  // namespace HubPlatform
