#include "camera_test/command.h"
#include "camera_test/command_parser.h"
#include "camera_test/cameratest_helpers.h"
#include "camera_test/stream_tester_manager.h"

#include "database_client/lmdb_client.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbtools.h"

extern "C" {
#include <libavutil/log.h>
}

std::string getHelpString() {
    return R"(camera_test help
    print help

camera_test list
    list hub's local RTSP steams

camera_test key-frame | continuous-streaming [-i <index>] [-u <url>] [--log <level>]
    the target stream specified either by an index or by a custom url
    -i --index - an index of the camera stream from the list command
    -u --url - an url of any camera stream
    --log - sets the FFMPEG's log level, possible values are:
            quiet, panic, fatal, error, warning, info, verbose, debug, trace
            default is "quiet"

camera_test key-frame [-c <sec>] [-d <sec>] [-t <sec>]
    start testing the receiving of a key packet from the camera stream
    -c - duration of each connection in seconds, default is 2 sec
    -d - delay between connections to the camera stream in seconds, default is 0 sec
    -t - total duration of the test in seconds, default is 10 days

camera_test continuous-streaming [--packet-duration <ms>]
    start continuous testing of the camera stream
    --packet-duration - upper threshold of expected packet duration in milliseconds
                        when the threshold is exceeded warning is printed, default is 300 ms
)";
}

int main( int argc, char* argv[] ) {
    if( argc < 2 ) {
        std::cout << "Wrong params count" << std::endl;
        std::cout << getHelpString() << std::endl;
        return -1;
    }

    Command command;

    try {
        CommandParser parser( argc, argv );
        command = parser.getCommand();
    }
    catch( const CommandParseException& ex ) {
        std::cerr << ex.what() << std::endl;
        return -1;
    }

    switch( command.type ) {
    case Command::Type::PRINT_HELP: {
        std::cout << getHelpString() << std::endl;
        break;
    }

    case Command::Type::PRINT_STREAMS_LIST: {
        std::vector< std::string > activeStreams;
        try {
            LMDBClient dbClient( getStorageFilePath(), getTemporaryStorageFilePath() );
            activeStreams = collectActiveStreams( dbClient );
        }
        catch( const std::runtime_error& ex ) {
            std::cerr << ex.what() << std::endl;
            return -1;
        }

        int counter = 0;
        for( const auto& stream : activeStreams ) {
            std::cout << std::to_string( counter++ ) + '\t' + stream + '\n';
        }
        break;
    }

    case Command::Type::CONTINUOUS_STREAMING_TEST:
    case Command::Type::KEY_FRAME_TEST: {
        std::string urlForTesting;
        try {
            urlForTesting = findUrlForTesting( command );
        }
        catch( const std::exception& ex ) {
            std::cerr << ex.what() << std::endl;
            return -1;
        }

        if( !urlForTesting.empty() ) {
            std::cout << "Running test for " << urlForTesting << '\n' << std::endl;

            auto ffmpegLogCallback = []( void*, int logLevel, const char* format, va_list args ) {
                if( av_log_get_level() >= logLevel ) {
                    const std::vector< char > message = buildFfmpegLogMessage( format, args );
                    std::cout.write( message.data(), static_cast< std::streamsize >( message.size() ) );
                    std::cout.flush();
                }
            };
            av_log_set_level( command.logLevel );
            av_log_set_callback( ffmpegLogCallback );

            StreamTesterManager testersManager( std::move( urlForTesting ),
                                                command.connectionDuration,
                                                command.delayDuration,
                                                command.totalTestDuration,
                                                command.packetDurationThreshold,
                                                std::cout );

            if( command.type == Command::Type::KEY_FRAME_TEST )
                testersManager.runKeyFrameTest();
            else
                testersManager.runContinuousStreamingTest();
        }
        break;
    }

    case Command::Type::NONE:
        std::cerr << "You have to specify a command to run the tool, see help" << std::endl;
        break;
    }

    return EXIT_SUCCESS;
}
