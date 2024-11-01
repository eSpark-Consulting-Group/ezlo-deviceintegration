#include "base_utils/signal_handler.h"
#include "base_utils/timeutils.h"
#include "base_utils/utils.h"
#include "hardware_tester.h"

#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <tiny/tiny_consolecolors.h>

namespace {
const std::string SetLogLevelCriticalCmd = "dmesg -n 2";
const std::string SetLogLevelDebugCmd = "dmesg -n 7";
}  // namespace
extern std::string git_version();

int main( int argc, char* argv[] ) {
    std::vector< std::string > args;
    for( size_t i = 1; i < argc; ++i )
        args.emplace_back( argv[ i ] );

    int result = EXIT_FAILURE;
    auto startTime = TimeUtils::getMonotonicTime();

    SignalHandler sigHandler;
    sigHandler.setupSignalHandlers();

    HubPlatform::HardwareTest::HardwareTester tester( sigHandler.gotExitSignalRef() );

    try {
        utils::readPipe( SetLogLevelCriticalCmd );
        tester.setUp( args );
        result = tester.runTestLoop();
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
    std::cout << "Test execution time: " << TimeUtils::getDiffMonotonicTime( startTime ) << " seconds" << std::endl;

    utils::readPipe( SetLogLevelDebugCmd );
    return result;
}
