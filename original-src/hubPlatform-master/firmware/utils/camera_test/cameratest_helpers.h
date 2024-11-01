#pragma once

#include <cstdarg>
#include <vector>
#include <string>

class IDbClient;
struct Command;

std::vector< std::string > collectActiveStreams( const IDbClient& dbClient );
std::string findUrlForTesting( const Command& command );
std::vector< char > buildFfmpegLogMessage( const char* format, va_list args );
