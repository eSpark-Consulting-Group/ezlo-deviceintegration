#include "camera_test/command_parser.h"

#include <cstring>

extern "C" {
#include <libavutil/log.h>
}

constexpr std::chrono::seconds Command::MaxDuration;

CommandParser::CommandParser( int argc, char* argv[] )
: m_command( parseArgs( argc, argv ) ) {
    switch( m_command.type ) {
    case Command::Type::KEY_FRAME_TEST:
        validateKeyFrameTestCommand( m_command );
        break;
    case Command::Type::CONTINUOUS_STREAMING_TEST:
        validateContinuousStreamingTestCommand( m_command );
        break;
    default:
        // no validation needed
        break;
    }
}

const Command& CommandParser::getCommand() noexcept {
    return m_command;
}

Command CommandParser::parseArgs( int argc, char* argv[] ) {
    auto readIntArg = [argc, argv]( int idx ) -> int {
        if( idx < argc ) {
            const long value = strtol( argv[ idx ], nullptr, 10 );
            return static_cast< int >( value );
        }
        return 0;
    };

    Command command;
    command.logLevel = AV_LOG_QUIET;

    for( auto i = 1; i < argc; ++i ) {
        if( strcmp( argv[ i ], "help" ) == 0 ) {
            command.type = Command::Type::PRINT_HELP;
        }
        else if( strcmp( argv[ i ], "list" ) == 0 ) {
            command.type = Command::Type::PRINT_STREAMS_LIST;
        }
        else if( strcmp( argv[ i ], "key-frame" ) == 0 ) {
            command.type = Command::Type::KEY_FRAME_TEST;

            command.connectionDuration = Command::DefaultKeyFrameTestConnectionDuration;
            command.delayDuration = Command::DefaultKeyFrameTestDelayDuration;
            command.totalTestDuration = Command::DefaultKeyFrameTestDuration;
            command.packetDurationThreshold = Command::MaxDuration;
        }
        else if( strcmp( argv[ i ], "continuous-streaming" ) == 0 ) {
            command.type = Command::Type::CONTINUOUS_STREAMING_TEST;

            command.connectionDuration = Command::MaxDuration;
            command.delayDuration = std::chrono::seconds( 0 );
            command.totalTestDuration = Command::MaxDuration;
            command.packetDurationThreshold = Command::DefaultStreamingTestPacketDurationThreshold;
        }

        else if( strcmp( argv[ i ], "-i" ) == 0 || strcmp( argv[ i ], "--index" ) == 0 ) {
            command.streamIdx = readIntArg( ++i );
        }
        else if( strcmp( argv[ i ], "-u" ) == 0 || strcmp( argv[ i ], "--url" ) == 0 ) {
            i++;
            std::string url = i < argc ? argv[ i ] : "";
            command.streamUrl = std::move( url );
        }

        else if( strcmp( argv[ i ], "-c" ) == 0 ) {
            const int value = readIntArg( ++i );
            command.connectionDuration = std::chrono::seconds( value );
        }
        else if( strcmp( argv[ i ], "-d" ) == 0 ) {
            const int value = readIntArg( ++i );
            command.delayDuration = std::chrono::seconds( value );
        }
        else if( strcmp( argv[ i ], "-t" ) == 0 ) {
            const int value = readIntArg( ++i );
            command.totalTestDuration = std::chrono::seconds( value );
        }

        else if( strcmp( argv[ i ], "--packet-duration" ) == 0 ) {
            const int value = readIntArg( ++i );
            command.packetDurationThreshold = std::chrono::milliseconds( value );
        }

        else if( strcmp( argv[ i ], "--log" ) == 0 ) {
            i++;
            std::string logLevelStr = i < argc ? argv[ i ] : "";
            command.logLevel = parseLogLevel( logLevelStr );
        }

        else {
            throw CommandParseException( std::string( "Unknown argument: " ) + argv[ i ] );
        }
    }

    return command;
}

int CommandParser::parseLogLevel( const std::string& logLevelStr ) {
    if( logLevelStr == "quiet" )
        return AV_LOG_QUIET;
    else if( logLevelStr == "panic" )
        return AV_LOG_PANIC;
    else if( logLevelStr == "fatal" )
        return AV_LOG_FATAL;
    else if( logLevelStr == "error" )
        return AV_LOG_ERROR;
    else if( logLevelStr == "warning" )
        return AV_LOG_WARNING;
    else if( logLevelStr == "info" )
        return AV_LOG_INFO;
    else if( logLevelStr == "verbose" )
        return AV_LOG_VERBOSE;
    else if( logLevelStr == "debug" )
        return AV_LOG_DEBUG;
    else if( logLevelStr == "trace" )
        return AV_LOG_TRACE;

    throw CommandParseException( "Unknown log level: " + logLevelStr );
}

void CommandParser::validateKeyFrameTestCommand( const Command& command ) {
    validateTestStreamCommand( command );

    if( command.connectionDuration <= std::chrono::seconds( 0 ) )
        throw CommandParseException( "Connection duration should be greater than 0" );
    if( command.delayDuration < std::chrono::seconds( 0 ) )
        throw CommandParseException( "Delay duration should be equal or greater than 0" );
    if( command.totalTestDuration <= std::chrono::seconds( 0 ) )
        throw CommandParseException( "Total test duration should be greater than 0" );
}

void CommandParser::validateContinuousStreamingTestCommand( const Command& command ) {
    validateTestStreamCommand( command );

    if( command.connectionDuration != Command::MaxDuration )
        throw CommandParseException( "\"-c\" option can be used only with \"key-frame\" command" );
    if( command.delayDuration != std::chrono::seconds( 0 ) )
        throw CommandParseException( "\"-d\" option can be used only with \"key-frame\" command" );
    if( command.totalTestDuration != Command::MaxDuration )
        throw CommandParseException( "\"-t\" option can be used only with \"key-frame\" command" );

    if( command.packetDurationThreshold <= std::chrono::milliseconds( 0 ) )
        throw CommandParseException( "Packet duration warning threshold should be greater than 0" );
}

void CommandParser::validateTestStreamCommand( const Command& command ) {
    const bool hasStreamIdx = command.streamIdx >= 0;
    const bool hasStreamUrl = !command.streamUrl.empty();

    if( hasStreamIdx == hasStreamUrl )
        throw CommandParseException( "Specify either valid index or url" );
}
