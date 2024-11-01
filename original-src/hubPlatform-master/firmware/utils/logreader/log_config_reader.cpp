#include "log_config_reader.h"
#include "log_utils/ezlo_log.h"
#include "system_utils/environment.h"
#include "config_reader/config_keys.h"

#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {
namespace LogReader {

inline void LogConfigReader::readSafe( std::function< void() > func ) {
    try {
        if( func ) {
            func();
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Error caught while calling function :" ) + cc::note( ex.what() ) );
    }
}

inline bool LogConfigReader::isFilePathValid( const std::string& filePath ) {
    const char bad_chars[] = "!@%^*~|";
    if( filePath.empty() )
        return false;

    for( const auto& it : bad_chars ) {
        if( filePath.find( it ) != std::string::npos ) {
            LOGGER_DEBUG_RAW( cc::debug( "File path contains bad char (" + std::to_string( it ) + ")" ) );
            return false;
        }
    }

    if( filePath.find_first_of( '/' ) == std::string::npos || filePath.find_last_of( '/' ) + 1 == filePath.size() ) {
        return false;
    }

    return true;
}

LogConfigReader::LogConfigReader( ConfigReader&& configReader, int argc, char* argv[] )
: m_configReader( std::move( configReader ) ) {
    init( argc, argv );
}

LogConfigReader::LogConfigReader( int argc, char* argv[] )
: m_parser( AppName ) {
    init( argc, argv );
}

void LogConfigReader::init( int argc, char* argv[] ) {
    readerConfig();
    parseOptions( argc, argv );
}

const std::string& LogConfigReader::getServiceName() const {
    return m_serviceName;
}

const std::string& LogConfigReader::getLogFilePath() const {
    return m_logFilePath;
}

const std::string& LogConfigReader::getUbusServerName() const {
    return m_ubusServerName;
}

const std::string& LogConfigReader::getUbusCallbackName() const {
    return m_ubusCallbackName;
}
bool LogConfigReader::getStdOutLogState() const {
    return m_stdoutLogState;
}
bool LogConfigReader::getFileLogState() const {
    return m_fileLogState;
}
bool LogConfigReader::getUbusLogState() const {
    return m_ubusLogState;
}

bool LogConfigReader::isValid() {
    if( m_serviceName.empty() ) {
        LOGGER_INFO_RAW( cc::info( "Service name is required" ) );
        return false;
    }

    if( !( m_stdoutLogState || m_fileLogState || m_ubusLogState ) ) {
        LOGGER_INFO_RAW( cc::info( "At least one of stdout, file and ubus must be enabled" ) );
        return false;
    }

    if( m_fileLogState && !isFilePathValid( m_logFilePath ) ) {
        m_logFilePath = HubPlatform::Environment::getLogsFolder() + "/" + m_serviceName + ".log";
        LOGGER_INFO_RAW( cc::info( "Log file path updated to " + m_logFilePath ) );
    }

    if( m_ubusLogState ) {
        if( m_ubusServerName.empty() || m_ubusCallbackName.empty() ) {
            LOGGER_INFO_RAW( cc::info( "Server name and callback name are required for Ubus" ) );
            return false;
        }
    }

    return true;
}

void LogConfigReader::readerConfig() {
    if( m_configReader.hasValue( ConfigKeys::LogReaderStdOutState ) ) {
        readSafe( [this] { m_stdoutLogState = m_configReader.getBool( ConfigKeys::LogReaderStdOutState ); } );
    }

    if( m_configReader.hasValue( ConfigKeys::LogReaderFileState ) ) {
        readSafe( [this] { m_fileLogState = m_configReader.getBool( ConfigKeys::LogReaderFileState ); } );
    }

    if( m_configReader.hasValue( ConfigKeys::LogReaderFileLogFilePath ) ) {
        readSafe( [this] { m_logFilePath = m_configReader.getString( ConfigKeys::LogReaderFileLogFilePath ); } );
    }

    if( m_configReader.hasValue( ConfigKeys::LogReaderUbusState ) ) {
        readSafe( [this] { m_ubusLogState = m_configReader.getBool( ConfigKeys::LogReaderUbusState ); } );
    }

    if( m_configReader.hasValue( ConfigKeys::LogReaderUbusServerName ) ) {
        readSafe( [this] { m_ubusServerName = m_configReader.getString( ConfigKeys::LogReaderUbusServerName ); } );
    }
    if( m_configReader.hasValue( ConfigKeys::LogReaderUbusCallbackName ) ) {
        readSafe( [this] { m_ubusCallbackName = m_configReader.getString( ConfigKeys::LogReaderUbusCallbackName ); } );
    }
}

void LogConfigReader::parseOptions( int argc, char* argv[] ) {
    m_parser.on( "service-name",
                 "Name of the service whose log is read" + optionSpace + "type: required, string ",
                 [this]( const std::string& in_serviceName ) { m_serviceName = in_serviceName; } );
    m_parser.on(
        "StdOut Log\nstdout-state",
        "stdout log state," + optionSpace + "type: bool" + optionSpace + "default: false",
        [this]( const std::string& in_stdOutLogState ) { m_stdoutLogState = stringToBool( in_stdOutLogState ); } );
    m_parser.on( "File Log\nfile-state",
                 "file log state," + optionSpace + "type: bool" + optionSpace + "default: false",
                 [this]( const std::string& infileLogState ) { m_fileLogState = stringToBool( infileLogState ); } );
    m_parser.on( "File Log\nlog-file-path",
                 "Custom log file path," + optionSpace + "type: optional, file-path" + optionSpace +
                     "default: " + HubPlatform::Environment::getLogsFolder() + "/{service-name}.log",
                 [this]( const std::string& in_logFilePath ) { m_logFilePath = in_logFilePath; } );
    m_parser.on( "Ubus Log\nubus-state",
                 "ubus log state," + optionSpace + "type: bool" + optionSpace + "default: false",
                 [this]( const std::string& in_ubusLogState ) { m_ubusLogState = stringToBool( in_ubusLogState ); } );
    m_parser.on( "Ubus Log\nserver-name",
                 "Ubus server name" + optionSpace + "type: required for ubus, string",
                 [this]( const std::string& in_ubusServerName ) { m_ubusServerName = in_ubusServerName; } );
    m_parser.on( "Ubus Log\ncallback-name",
                 "Ubus callback name" + optionSpace + "type: required for ubus, string",
                 [this]( const std::string& in_ubusCallbackName ) { m_ubusCallbackName = in_ubusCallbackName; } );
    m_parser.on( "help", "Show usage", [this]() {
        std::cout << m_parser.help_text() << std::endl;
        exit( EXIT_SUCCESS );
    } );
    m_parser.parse( argc, argv );
}

}  // namespace LogReader
}  // namespace HubPlatform
