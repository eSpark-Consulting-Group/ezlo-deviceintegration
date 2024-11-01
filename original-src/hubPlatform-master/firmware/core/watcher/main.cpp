#include "watcher/service.h"
#include <thread>
#include <csignal>

int main( int argc, char* argv[] ) {
    std::shared_ptr< HubPlatform::Watcher::Service > watcher = nullptr;
    bool exitStatus;

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = HubPlatform::Watcher::signalHandler;
    sigemptyset( &sigIntHandler.sa_mask );
    sigIntHandler.sa_flags = 0;
    sigaction( SIGINT, &sigIntHandler, nullptr );
    sigaction( SIGTERM, &sigIntHandler, nullptr );

    while( true ) {
        if( watcher == nullptr ) {
            watcher = std::make_shared< HubPlatform::Watcher::Service >();
        }
        if( !watcher->uBusThreadRunning ) {
            Log::debug( cc::debug( "Running ubus server instance!" ) );
            watcher->ubusRunThread = std::thread( [watcher]() { watcher->uBusRun(); } );
            watcher->uBusThreadRunning = true;
        }
        if( watcher->getWatcherUbusConnectionStatus() != 0 ) {
            Log::error( cc::error( "Watcher server's uBus connection lost, reconnecting.." ) );
            stopUBusThread( watcher );
        }
        if( HubPlatform::Watcher::getWatcherExitSignalStatus() == true ) {
            Log::debug( cc::debug( "Exit signal caught, finishing!" ) );
            stopUBusThread( watcher );
            break;
        }
        if( HubPlatform::Watcher::getWatcherUnhandledStatus() ) {
            Log::debug( cc::debug( "Unhandled signal caught!" ) );
            HubPlatform::Watcher::setWatcherUnhandledSignal( false );
        }
        std::this_thread::sleep_for( std::chrono::seconds( WAIT_FOR ) );
    }

    return 0;
}
