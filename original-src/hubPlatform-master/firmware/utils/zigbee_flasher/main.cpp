#include "zigbee_flasher/zigbee_flasher.h"

#include "base_utils/signal_handler.h"

#include <iostream>

int main( int argc, char* argv[] ) {
    SignalHandler sigHandler;
    sigHandler.setupSignalHandlers();

    HubPlatform::ZigbeeFlasher::ZigbeeFlasher flasher( sigHandler.gotExitSignalRef() );

    int result = EXIT_FAILURE;
    try {
        flasher.setUp( argc, argv );
        result = flasher.run();
    }
    catch( const SignalException& ex ) {
        std::cerr << "SignalException: " << ex.what() << std::endl;
    }
    catch( const std::exception& ex ) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch( ... ) {
        std::cerr << "Unhandled exception! Something very bad happened!" << std::endl;
    }

    return result;
}
