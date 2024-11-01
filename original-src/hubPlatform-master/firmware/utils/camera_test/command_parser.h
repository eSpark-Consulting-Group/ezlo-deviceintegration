#pragma once

#include "camera_test/command.h"

#include <stdexcept>

class CommandParseException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class CommandParser {
public:
    CommandParser( int argc, char* argv[] );
    const Command& getCommand() noexcept;

private:
    static Command parseArgs( int argc, char* argv[] );
    static int parseLogLevel( const std::string& logLevelStr );

    static void validateKeyFrameTestCommand( const Command& command );
    static void validateContinuousStreamingTestCommand( const Command& command );
    static void validateTestStreamCommand( const Command& command );

    Command m_command;
};
