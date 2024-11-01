#include "log_arg_parse.h"

#include <cstdlib>
#include <sstream>
#include <utility>
#include <iostream>
#include <algorithm>

#include "logcmd/log_exceptions.h"
#include "system_utils/environment.h"

namespace HubPlatform {
namespace LogCmd {

const char* const ARG_ALL_LOGS = "--all";
const char* const ARG_ROTATED_LOGS = "--rotated";
const char* const ARG_SELECTED_LOGS = "--select";
const char* const ARG_UPLOAD = "--upload";
const char* const ARG_UPLOAD_ARCHIVED = "--upload-archived";
const char* const ARG_UPLOAD_POSTPONED = "--upload-postponed";
const char* const ARG_STORE = "--store";
const char* const ARG_STORE_TO_POSTPONED = "--store-to-postponed";
const char* const ARG_ROTATE = "--rotate";
const char* const ARG_SLICE = "--slice";
const char* const ARG_ROTATE_ON_SIZE_LIMIT = "--rotate-on-size-limit";
const char* const ARG_UPDATE_SETTINGS = "--update-settings";
const char* const ARG_AUTOREMOVE = "--autoremove";
const char* const ARG_LOG_DIR = "--log-dir";
const char* const ARG_OUTPUT_NAME = "--output-name";
const char* const ARG_ADD_TO_NAME = "--add-to-name";
const char* const ARG_OUTPUT_DIR = "--output-dir";
const char* const ARG_CREATE_DIR = "--create-dir";
const char* const ARG_ROTATE_ARCHIVES = "--rotate-archives";
const char* const ARG_NO_SYSTEM_LOGS = "--no-system-logs";
const char* const ARG_IDLE = "--idle";
const char* const ARG_FORCE_UPLOAD = "--force-upload";
const char* const ARG_LOG_LEVEL = "--log-level";
const char* const ARG_LOG_TRACE_ID = "--trace-id";
const char* const ARG_HELP_FULL = "--help";
const char* const ARG_HELP_SHORT = "-h";

namespace Impl {

void addFlag( Json& json, const Key& key, const Value& value ) {
    if( !value.empty() )
        throw LogArgError( "Flag \"" + key + "\" should not contain any value" );
    json_add_bool( json, key, true );
}

void addValue( Json& json, const Key& key, const Value& value ) {
    if( value.empty() )
        throw LogArgError( "Empty value for argument: " + key );
    json_add_str( json, key, value );
}

}  // namespace Impl

namespace {

using Key = std::string;
using Value = std::string;
using KeyValue = std::pair< Key, Value >;

KeyValue splitArg( const std::string& arg ) {
    size_t delimPos = arg.find( '=' );
    bool hasValue = delimPos != std::string::npos;
    return { arg.substr( 0, delimPos ), ( hasValue ) ? arg.substr( delimPos + 1 ) : "" };
}

JsonArray parseArrayValue( const Value& arrayValue ) {
    JsonArray array;
    const char delim = ',';

    std::stringstream ss( arrayValue );
    std::string value;
    while( std::getline( ss, value, delim ) ) {
        array.push_back( value );
    }
    return array;
}

bool isNumber( const Value& value ) {
    return !value.empty() && std::all_of( value.cbegin(), value.cend(), ::isdigit );
}

void addArray( Json& json, const Key& key, const Value& value ) {
    if( value.empty() )
        throw LogArgError( "Empty value for argument: " + key );
    auto array = parseArrayValue( value );
    if( array.empty() )
        throw LogArgError( "Invalid value for argument: " + key );
    json[ key ] = std::move( array );
}

void addDirectoryName( Json& json, const Key& key, const Value& value ) {
    if( value.empty() )
        throw LogArgError( "Empty directory name for argument: " + key );
    if( value.front() != '/' )
        throw LogArgError( "Directory path is not absolute for argument: " + key );
    if( value.size() > 1 && value.back() == '/' )
        json_add_str( json, key, value.substr( 0, value.size() - 1 ) );
    else
        json_add_str( json, key, value );
}

void addArchiveName( Json& json, const Key& key, const Value& value ) {
    if( value.empty() )
        throw LogArgError( "Empty archive name" );
    if( value.find( '/' ) != Value::npos )
        throw LogArgError( "Archive name should not contain slashes ('/')" );
    if( value.find( '*' ) != Value::npos )
        throw LogArgError( "Archive name should not contain asterisks ('*')" );
    if( value.size() < 4 )
        throw LogArgError( "Archive name should contain '.tgz' extension" );
    if( value.compare( value.size() - 4, 4, ".tgz" ) != 0 )
        throw LogArgError( "Archive extension is not '.tgz'" );
    json_add_str( json, key, value );
}

void addArchiveRotation( Json& json, const Key& key, const Value& value ) {
    if( value.empty() )
        throw LogArgError( "Empty value for argument: " + key );
    if( !isNumber( value ) )
        throw LogArgError( "Value for argument \"" + key + "\" is not an integer" );
    int rotationValue = std::stoi( value );
    if( rotationValue <= 0 )
        throw LogArgError( "Value for argument \"" + key + "\" should be greater than 0" );
    json_add_int( json, key, rotationValue );
}

void setLogLevel( Json& /*json*/, const Key& /*key*/, const Value& value ) {
    if( value == "DEBUG" ) {
        Log::setLocalLogLevel( Log::DEBUG );
    }
    else if( value == "TRACE" ) {
        Log::setLocalLogLevel( Log::TRACE );
    }
}

void setTraceId( Json& json, const Key& key, const Value& value ) {
    json_add_str( json, key, value );
}

void printHelp( Json& json, const Key& key, const Value& value ) {
    help();
    exit( 0 );
}

void processArgs( const Args& args, Json& parsed ) {
    const std::map< Key, std::function< void( Json&, const Key&, const Value& ) > > argHandler = {
        { ARG_ALL_LOGS, Impl::addFlag },
        { ARG_ROTATED_LOGS, Impl::addFlag },
        { ARG_SELECTED_LOGS, addArray },
        { ARG_UPLOAD, Impl::addFlag },
        { ARG_UPLOAD_ARCHIVED, Impl::addFlag },
        { ARG_UPLOAD_POSTPONED, Impl::addFlag },
        { ARG_STORE, Impl::addFlag },
        { ARG_STORE_TO_POSTPONED, Impl::addFlag },
        { ARG_ROTATE, Impl::addFlag },
        { ARG_SLICE, Impl::addFlag },
        { ARG_ROTATE_ON_SIZE_LIMIT, Impl::addFlag },
        { ARG_UPDATE_SETTINGS, Impl::addFlag },
        { ARG_AUTOREMOVE, Impl::addFlag },
        { ARG_FORCE_UPLOAD, Impl::addFlag },
        { ARG_OUTPUT_NAME, addArchiveName },
        { ARG_ADD_TO_NAME, Impl::addValue },
        { ARG_OUTPUT_DIR, addDirectoryName },
        { ARG_CREATE_DIR, Impl::addFlag },
        { ARG_LOG_DIR, addDirectoryName },
        { ARG_ROTATE_ARCHIVES, addArchiveRotation },
        { ARG_NO_SYSTEM_LOGS, Impl::addFlag },
        { ARG_IDLE, Impl::addFlag },
        { ARG_LOG_LEVEL, setLogLevel },
        { ARG_LOG_TRACE_ID, setTraceId },
        { ARG_HELP_FULL, printHelp },
        { ARG_HELP_SHORT, printHelp },
    };

    for( auto& arg : args ) {
        auto keyValue = splitArg( arg );
        auto it = argHandler.find( keyValue.first );
        if( it == argHandler.cend() )
            throw LogArgError( "Unknown argument: " + keyValue.first );
        it->second( parsed, keyValue.first, keyValue.second );
    }
}

size_t countKeys( const std::vector< Key >& keys, const Json& args ) {
    auto hasKey = [&args]( const Key& key ) { return json_contains( args, key ); };
    return std::count_if( keys.begin(), keys.end(), hasKey );
}

}  // namespace

Json parseArgs( const Args& args ) {
    Json parsed;

    json_add_str( parsed, ARG_LOG_DIR, Environment::getLogsFolder() );
    json_add_str( parsed, ARG_OUTPUT_DIR, Environment::getTempDataFolder() );

    processArgs( args, parsed );

    if( countKeys( { ARG_ALL_LOGS, ARG_ROTATED_LOGS, ARG_SELECTED_LOGS }, parsed ) > 1 )
        throw LogArgError( "Only one type of logs is allowed" );

    if( countKeys( { ARG_UPLOAD,
                     ARG_STORE,
                     ARG_UPLOAD_ARCHIVED,
                     ARG_STORE_TO_POSTPONED,
                     ARG_ROTATE,
                     ARG_SLICE,
                     ARG_ROTATE_ON_SIZE_LIMIT,
                     ARG_UPDATE_SETTINGS },
                   parsed ) > 1 )
        throw LogArgError( "Only one action is allowed" );

    if( json_contains( parsed, ARG_STORE_TO_POSTPONED ) ) {
        Impl::addFlag( parsed, ARG_IDLE, "" );
        Impl::addFlag( parsed, ARG_CREATE_DIR, "" );
        addDirectoryName( parsed, ARG_OUTPUT_DIR, Environment::getPostponedLogsFolder() );
    }

    if( json_contains( parsed, ARG_ROTATE ) ) {
        Impl::addFlag( parsed, ARG_IDLE, "" );
        Impl::addFlag( parsed, ARG_AUTOREMOVE, "" );
        Impl::addFlag( parsed, ARG_ROTATED_LOGS, "" );
        if( parsed.find( ARG_ADD_TO_NAME ) == parsed.end() ) {
            Impl::addValue( parsed, ARG_ADD_TO_NAME, "rotated" );
        }
    }

    if( json_contains( parsed, ARG_UPLOAD_POSTPONED ) ) {
        Impl::addFlag( parsed, ARG_IDLE, "" );
        Impl::addFlag( parsed, ARG_AUTOREMOVE, "" );
        addDirectoryName( parsed, ARG_LOG_DIR, Environment::getPostponedLogsFolder() );
    }

    return parsed;
}

void help() {
    using std::cout;
    using std::endl;

    const char* const progName = "logcmd";
    const char* const sp4 = "    ";
    cout << "Usage:\n";
    cout << sp4 << progName << " [<ACTION>] [<LOG-TYPE>] [<OPTIONS>]" << std::endl;
    cout << sp4 << progName << " "
         << "[" << ARG_UPLOAD << "|" << ARG_UPLOAD_ARCHIVED << "|" << ARG_STORE << "]";
    cout << " [<LOG-TYPE>] "
         << "[OPTIONS]" << '\n';
    cout << sp4 << progName << " "
         << "[" << ARG_STORE_TO_POSTPONED << "|" << ARG_UPLOAD_POSTPONED << "|" << ARG_ROTATE << "|" << ARG_SLICE << "|"
         << ARG_ROTATE_ON_SIZE_LIMIT << "|" << ARG_UPDATE_SETTINGS << "]" << endl;
    cout << sp4 << progName << " "
         << "[" << ARG_HELP_SHORT << "|" << ARG_HELP_FULL << "]" << '\n';
    cout << '\n';

    cout << "Actions (only one action is allowed):\n";
    cout << sp4 << ARG_UPLOAD << " - Upload archive to cloud storage (default behaviour).\n";
    cout << sp4 << ARG_UPLOAD_ARCHIVED << " - Upload already archived logs to cloud storage.\n";
    cout << sp4 << ARG_STORE << " - Stores archive on filesystem.\n";
    cout << sp4 << ARG_STORE_TO_POSTPONED << " - Stores archive to postpone directory.\n";
    cout << sp4 << ARG_ROTATE << " - Rotate logs. Rotation policy defined in controller settings.\n";
    cout << sp4 << ARG_SLICE
         << " - Slice logs by file size limit. Sliced logs are moved out of RAM to permanent storage.\n";
    cout << sp4 << ARG_ROTATE_ON_SIZE_LIMIT << " - Rotate or slice: rotates logs if pending folder reaches the limit,\n"
         << sp4 << "      otherwise slices large files by size.\n";
    cout << sp4 << ARG_UPDATE_SETTINGS << " - Apply new controller settings.\n";
    cout << sp4 << ARG_HELP_SHORT << "|" << ARG_HELP_FULL << " - Print help screen and exit.\n";
    cout << '\n';

    cout << "Options:\n";
    cout << sp4 << ARG_UPLOAD_POSTPONED << " - Upload postponed logs to cloud storage.\n";
    cout << sp4 << ARG_AUTOREMOVE << " - Delete uploaded/stored logs from log directory.\n";
    cout << sp4 << ARG_LOG_DIR << " - Absolute path to log directory (default: /var/log/firmware).\n";
    cout << sp4 << ARG_OUTPUT_DIR << " - Absolute path to directory where archive should be stored (default: /tmp).\n";
    cout << sp4 << ARG_OUTPUT_NAME << " - Override automatic generated archive name.\n";
    cout << sp4 << ARG_ADD_TO_NAME << " - Add string to autogenerated archive name.\n";
    cout << sp4 << ARG_CREATE_DIR << " - Create output directory.\n";
    cout << sp4 << ARG_ROTATE_ARCHIVES << "=N"
         << " - Rotate archives if output directory contains more than N archives.\n";
    cout << sp4 << ARG_NO_SYSTEM_LOGS << " - Do not dump system logs.\n";
    cout << sp4 << ARG_IDLE << " - Report success if there are no log files to process.\n";
    cout << sp4 << ARG_FORCE_UPLOAD << " - Only for --rotate: ignore storing policy, upload rotated logs to cloud.\n";
    cout << '\n';

    cout << "LOG-TYPE\n";
    cout << sp4 << ARG_ALL_LOGS << " - Select any logs in log directory.\n";
    cout << sp4 << ARG_ROTATED_LOGS << " - Select rotated logs in log directory.\n";
    cout << sp4 << ARG_SELECTED_LOGS << "=A.log,B.log..."
         << " - Select list of logs in log directory.\n";
    cout << sp4 << ARG_LOG_LEVEL << " - Set log level, one of DEBUG, TRACE. (Default: DEBUG).\n";
    cout << sp4 << ARG_LOG_TRACE_ID << " - Pass trace ID to a command call to be included in log messages.\n";
    cout.flush();
}

}  // namespace LogCmd
}  // namespace HubPlatform
