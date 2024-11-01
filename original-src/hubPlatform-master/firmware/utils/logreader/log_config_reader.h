#pragma once

#include "config_reader/config_reader.h"
#include <tiny/tiny_command_line_parser.h>

namespace HubPlatform {
namespace LogReader {

namespace UT {
class LogConfigReaderTestSuite;
}

constexpr const char AppName[] = "logreader";

inline bool stringToBool( std::string str ) {
    static const std::set< std::string > stringToBoolMap = { "true", "1", "yes" };
    std::transform( str.begin(), str.end(), str.begin(), []( unsigned char c ) { return std::tolower( c ); } );
    auto it = stringToBoolMap.find( str );
    return it != stringToBoolMap.end() ? true : false;
}

class LogConfigReader final {
    friend class UT::LogConfigReaderTestSuite;

public:
    LogConfigReader( int argc, char* argv[] );

    LogConfigReader( const LogConfigReader& ) = delete;
    LogConfigReader& operator=( const LogConfigReader& ) = delete;

    LogConfigReader( LogConfigReader&& ) noexcept;
    LogConfigReader& operator=( LogConfigReader&& ) noexcept;

    const std::string& getServiceName() const;
    const std::string& getLogFilePath() const;
    const std::string& getUbusServerName() const;
    const std::string& getUbusCallbackName() const;
    bool getStdOutLogState() const;
    bool getFileLogState() const;
    bool getUbusLogState() const;
    bool isValid();

    ~LogConfigReader() = default;

private:
    LogConfigReader( ConfigReader&& configReader, int argc, char* argv[] );
    void init( int argc, char* argv[] );
    inline void readSafe( std::function< void() > func );

    inline bool isFilePathValid( const std::string& filePath );
    void parseOptions( int argc, char* argv[] );
    void readerConfig();

private:
    const std::string optionSpace = "\n\t\t\t    ";
    std::string m_serviceName;
    std::string m_logFilePath;
    std::string m_ubusServerName;
    std::string m_ubusCallbackName;
    bool m_stdoutLogState = false;
    bool m_fileLogState = false;
    bool m_ubusLogState = false;
    tiny::command_line::parser m_parser;
    HubPlatform::ConfigReader m_configReader;
};

}  // namespace LogReader
}  // namespace HubPlatform
