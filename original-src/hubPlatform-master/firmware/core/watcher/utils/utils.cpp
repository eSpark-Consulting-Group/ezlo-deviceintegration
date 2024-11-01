#include "watcher/utils/utils.h"

#include "watcher/types/process_info.h"

#include "config_reader/config_reader.h"
#include "system_utils/system.h"
#include "system_utils/filesystem.h"
#include "system_utils/environment.h"
#include "common_utils/exception_utils.h"
#include "base_utils/string_utils.h"
#include "base_utils/utils.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

#include <algorithm>

namespace HubPlatform {
namespace Watcher {
namespace Utils {

static inline bool isSpaceChar( unsigned char c ) {
    return std::isspace( c );
};

std::string readOpenwrtPsCommandOutput() {
    int result = 0;
    std::string output;
    std::tie( output, result ) = System::getCommandOutput( "ps -w" );
    auto unpackedExitCode = System::unpackExitCode( result );
    if( unpackedExitCode.exitCode == 0 ) {
        return output;
    }

    LOGGER_WARN( cc::warn( "Failed to read ps output:" ) + cc::warn( "code: " ) +
                 cc::num10( unpackedExitCode.exitCode ) + cc::warn( ", err: " ) + cc::note( output ) );
    return {};
}

std::vector< Types::ProcessInfo > parseOpenwrtPsOutput( const std::string& output ) {
    static const std::vector< std::string > expectedHeader{ "PID", "USER", "VSZ", "STAT", "COMMAND" };

    auto list = StringUtils::splitByStrippedLinesWithText( output );
    if( list.empty() )
        return {};

    auto header = StringUtils::split( list.front() );
    if( header != expectedHeader ) {
        LOGGER_WARN( cc::warn( "Invalid ps output format: " ) + cc::note( list.front() ) );
        return {};
    }
    auto it = list.begin();
    ++it;

    std::vector< Types::ProcessInfo > result;
    result.reserve( list.size() );

    for( ; it != list.end(); ++it ) {
        auto& line = *it;
        Types::ProcessInfo element;

        // find PID
        auto posBeg = std::find_if_not( line.cbegin(), line.cend(), isSpaceChar );
        auto posEnd = std::find_if( posBeg, line.cend(), isSpaceChar );
        if( posBeg == line.cend() )
            continue;
        try {
            element.pid = std::stoi( std::string( posBeg, posEnd ) );
        }
        catch( ... ) {
            continue;
        }

        // find USER
        posBeg = std::find_if_not( posEnd, line.cend(), isSpaceChar );
        posEnd = std::find_if( posBeg, line.cend(), isSpaceChar );
        if( posBeg == line.cend() )
            continue;

        // find VSZ
        posBeg = std::find_if_not( posEnd, line.cend(), isSpaceChar );
        posEnd = std::find_if( posBeg, line.cend(), isSpaceChar );
        if( posBeg == line.cend() )
            continue;

        // find STAT
        posBeg = std::find_if_not( posEnd, line.cend(), isSpaceChar );
        posEnd = std::find_if( posBeg, line.cend(), isSpaceChar );
        if( posBeg == line.cend() )
            continue;
        element.state.assign( posBeg, posEnd );

        // find CMD
        posBeg = std::find_if_not( posEnd, line.cend(), isSpaceChar );
        if( posBeg == line.cend() )
            continue;
        element.cmd.assign( posBeg, line.cend() );

        result.emplace_back( std::move( element ) );
    }

    return result;
}

std::vector< Types::ProcessInfo > readProcessInfoListFromProcFs() {
    std::vector< Types::ProcessInfo > processInfoList;

    auto pidList = filesystem::getDirectoryFileNames( "/proc" );
    for( auto& pid : pidList ) {
        if( pid.empty() || !std::isdigit( pid.at( 0 ) ) ) {
            continue;
        }
        std::string path = "/proc/" + pid + "/status";
        if( !filesystem::isExist( path ) ) {
            continue;
        }

        auto content = System::readFile( path );
        if( content.empty() )
            continue;
        processInfoList.emplace_back( parseProcFsStatus( content ) );
    }
    return processInfoList;
}

Types::ProcessInfo parseProcFsStatus( const std::string& output ) {
    Types::ProcessInfo info;

    auto lines = StringUtils::splitByStrippedLinesWithText( output );
    bool found = false;

    auto lineIt =
        std::find_if( lines.begin(), lines.end(), []( const std::string& s ) { return s.find( "Name:" ) == 0; } );
    if( lineIt == lines.end() )
        return {};
    info.cmd = StringUtils::split( *lineIt ).at( 1 );

    lineIt = std::find_if( lineIt, lines.end(), []( const std::string& s ) { return s.find( "State:" ) == 0; } );
    if( lineIt == lines.end() )
        return {};
    info.state = StringUtils::split( *lineIt ).at( 1 );

    lineIt = std::find_if( lineIt, lines.end(), []( const std::string& s ) { return s.find( "Pid:" ) == 0; } );
    if( lineIt == lines.end() )
        return {};
    info.pid = std::atoi( StringUtils::split( *lineIt ).at( 1 ).c_str() );

    return info;
}

int readConfigIntValue( ConfigReader& reader, const std::string& path, int defaultValue ) {
    try {
        return reader.getInt( path.c_str() );
    }
    catch( const std::exception& e ) {
        LOGGER_WARN( cc::warn( "Unable to read config value: " ) + cc::note( path ) + cc::warn( ", error: " ) +
                     cc::note( CommonUtils::whatError( e ) ) );
    }
    return defaultValue;
}

JsonObject readConfigObjectValue( ConfigReader& reader, const std::string& path, const JsonObject& defaultValue ) {
    try {
        auto pathObj = ConfigReader::Path( reader, path.c_str() );
        return reader.getJsonObject( pathObj );
    }
    catch( const std::exception& e ) {
        LOGGER_WARN( cc::warn( "Unable to read config value: " ) + cc::note( path ) + cc::warn( ", error: " ) +
                     cc::note( CommonUtils::whatError( e ) ) );
    }
    return defaultValue;
}

void storeLogsStuckSafe( std::vector< std::string > args ) {
    std::string logsCmd = HubPlatform::Environment::getBinariesFolder() + "/logcmd --store-to-postponed ";

    for( auto& arg : args ) {
        logsCmd += arg + " ";
    }

    auto future = createDetachedTask< void >( [logsCmd]() {
        if( std::system( logsCmd.c_str() ) ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to store postponed logs" ) );
        }
    } );

    if( future.wait_for( std::chrono::seconds( 10 ) ) != std::future_status::ready ) {
        LOGGER_WARN_RAW( cc::warn( "Timeout for storing firmware logs" ) );
    }
}

}  // namespace Utils
}  // namespace Watcher
}  // namespace HubPlatform
