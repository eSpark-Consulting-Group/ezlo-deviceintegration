#pragma once

#include <string>

namespace HubPlatform {
namespace SystemMonitor {
namespace Utils {

int launchCommand( std::string command );
bool isProcessRunning( int pid );
void sendTerminateSignal( int pid );

void storeProcessPid( int pid, std::string pidfile );
int loadProcessPid( std::string pidfile );

void addLogHeader( std::string pidfile );

}  // namespace Utils
}  // namespace SystemMonitor
}  // namespace HubPlatform
