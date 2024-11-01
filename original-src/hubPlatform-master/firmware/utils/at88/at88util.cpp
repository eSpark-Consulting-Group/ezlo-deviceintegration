#include "at88/at88_options.h"

#include "base_utils/utils.h"

using namespace utils;

int main( int argc, char* argv[] ) {
    int exitResult = EXIT_SUCCESS;

    try {
        Log::setLocalLogLevel( Log::PriorityLevel::WARN );
        At88Options cliOptions;
        cliOptions.processCliOptions( argc, argv );
    }
    catch( const std::exception& ex ) {
        std::cerr << stringify( "AT88Util Exception caught: ", ex.what(), "\n" ) << std::flush;
        exitResult = EXIT_FAILURE;
    }
    catch( ... ) {
        std::cerr << "AT88Util Exception caught: default\n" << std::flush;
        exitResult = EXIT_FAILURE;
    }
    exit( exitResult );
}
