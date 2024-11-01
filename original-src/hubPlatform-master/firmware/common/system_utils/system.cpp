#include "system_utils/system.h"
#include "system_utils/logger.h"
#include "system_utils/environment.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <array>
#include <functional>
#include <sstream>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include "json_utils/json_utils.h"

extern "C" {
#include <sys/sysinfo.h>
#include <sys/mount.h>
}

namespace HubPlatform {
namespace System {

PopenFailed::PopenFailed( const std::string& message )
: SystemError( message ) {}

UntarFailed::UntarFailed( const std::string& message )
: SystemError( message ) {}

JsonObject getConfigValue( const std::string& key ) {
    std::ifstream configStream( Environment::getSystemConfigsFolder() +
                                "/config/hub_platform/logs_configuration.json" );
    if( configStream.is_open() ) {
        try {
            auto config = Json::parse( configStream );
            return config[ key ];
        }
        catch( std::exception& ex ) {
            SystemUtils::logError() << ex.what();
            return JSON_EMPTY_OBJ;
        }
    }

    return JSON_EMPTY_OBJ;
}

bool areAdvancedLogsSet() {
    static JsonObject logsEnabled = getConfigValue( "additional_networkd_logs" );
    return logsEnabled.is_boolean() && logsEnabled.get< bool >() == true;
}

std::pair< std::string, int > getCommandOutput( std::string command, bool catchStderr, bool stderrToDevNull ) {
    std::array< char, 128 > buffer;
    std::stringstream ss;

    if( command.empty() ) {
        SystemUtils::logInfo() << "Unable to get command output. Empty command provided"
                               << "\n";
    }

    static bool showAdvancedLogs = areAdvancedLogsSet();

    if( showAdvancedLogs ) {
        SystemUtils::logInfo() << "Command: " << command << "\n";
    }

    int result = 0;
    {
        if( catchStderr ) {
            command += " 2>&1";
        }
        else if( stderrToDevNull ) {  // use it if you have no stderr catching in a command passed
            command += " 2>/dev/null";
        }

        std::unique_ptr< FILE, std::function< void( FILE* ) > > pipe( popen( command.c_str(), "r" ),
                                                                      [&]( FILE* file ) { result = pclose( file ); } );
        if( !pipe ) {
            SystemUtils::logError() << "popen() failed, command: '" << command << "', error: '" << strerror( errno )
                                    << "'\n";
            throw PopenFailed( std::string{ "popen() failed, error: " } + strerror( errno ) );
        }

        while( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr ) {
            ss << buffer.data();
        }

        if( showAdvancedLogs ) {
            SystemUtils::logInfo() << "Output: " << ss.str() << "\n";
        }
    }
    return { std::piecewise_construct, std::forward_as_tuple( ss.str() ), std::forward_as_tuple( result ) };
}

std::string getAwkOutput( const std::string& awkCommand ) {
    return getAwkOutput( awkCommand.c_str() );
}
std::string getAwkOutput( const char* command ) {
    std::string result;
    std::tie( result, std::ignore ) = System::getCommandOutput( command );
    if( result.size() > 1 /*\n*/ ) {
        result.resize( result.size() - 1 );
    }
    return result;
}

USBDescription getUSBDescription( const std::string& usbDevicePath ) {
    std::ifstream idVendor( usbDevicePath + "/idVendor" );
    std::ifstream idProduct( usbDevicePath + "/idProduct" );
    USBDescription usbDescription;

    if( !idVendor.is_open() || !idProduct.is_open() ) {
        return {};
    }

    idVendor >> usbDescription.idVendor;
    idProduct >> usbDescription.idProduct;

    return usbDescription;
}

std::string formatTimeoutCommand( const std::string& command, int timeoutSeconds ) {
    std::string duration = std::to_string( timeoutSeconds ) + "s";
    std::string options = "--preserve-status -s 9 " + duration;  // SIGKILL == 9
    std::string safeCommand = "timeout " + options;
    return safeCommand + " " + command;
}

void untarArchive( const std::string& archivePath, const std::string& directoryToExtractPath ) {
    std::string command;
    std::string commandOutput;
    int result;

    if( directoryToExtractPath.empty() ) {
        command = "tar -xzf " + archivePath;
    }
    else {
        command = "tar -C " + directoryToExtractPath + " -xzf " + archivePath;
    }

    std::tie( commandOutput, result ) = getCommandOutput( command.c_str(), true, false );
    if( result != 0 ) {
        throw UntarFailed( commandOutput );
    }
}

void untarUnCompressedArchive( const std::string& archivePath, const std::string& directoryToExtractPath ) {
    std::string command;
    std::string commandOutput;
    int result;

    if( directoryToExtractPath.empty() ) {
        command = "tar -xf " + archivePath;
    }
    else {
        command = "tar -C " + directoryToExtractPath + " -xf " + archivePath;
    }

    std::tie( commandOutput, result ) = getCommandOutput( command.c_str(), true, false );
    if( result != 0 ) {
        throw UntarFailed( commandOutput );
    }
}

MemoryInfo getMemoryInfo() {
    struct sysinfo info;
    auto ret = sysinfo( &info );
    if( ret != 0 )
        throw SystemError( "Unable to get memory info: " + std::string( std::strerror( errno ) ) );

    const size_t units = info.mem_unit;
    MemoryInfo memoryInfo;
    memoryInfo.totalram = info.totalram * units;
    memoryInfo.freeram = info.freeram * units;
    memoryInfo.sharedram = info.sharedram * units;
    memoryInfo.bufferram = info.bufferram * units;

    return memoryInfo;
}

size_t getMemoryPageSize() {
    return sysconf( _SC_PAGESIZE );
}

void dropVirtualMemoryCaches() {
    System::getCommandOutput( "sysctl vm.drop_caches=1" );
}

std::string readFile( const std::string& filePath ) {
    std::ifstream fileReaded;
    std::stringstream fileStream;

    fileReaded.open( filePath );
    if( !fileReaded.is_open() ) {
        return fileStream.str();
    }
    fileStream << fileReaded.rdbuf();
    fileReaded.close();

    return fileStream.str();
}

CommandResult getCommandOutputWithErrorReport( const char* command, bool catchStderr, bool stderrToDevNull ) {
    std::array< char, 128 > buffer;
    std::stringstream ss;

    static bool showAdvancedLogs = areAdvancedLogsSet();

    if( showAdvancedLogs ) {
        SystemUtils::logInfo() << "Command: " << command << "\n";
    }

    int result = 0;
    pid_t childPid = -1;
    std::string error;

    std::string commandWithRedirect( command ? command : "" );
    const char* redirect = "";

    if( catchStderr ) {
        redirect = " 2>&1";
    }
    else if( stderrToDevNull ) {  // use it if you have no stderr catching in a command passed
        redirect = " 2>/dev/null";
    }

    commandWithRedirect += redirect;
    command = commandWithRedirect.c_str();

    FILE* pipe = popen( command, "r" );

    if( !pipe ) {
        SystemUtils::logError() << "popen() failed, command: '" << command << "', error: '" << strerror( errno )
                                << "'\n";
        throw PopenFailed( std::string{ "popen() failed, error: " } + strerror( errno ) );
    }
    childPid = getpid();

    std::string logMessage;

    if( showAdvancedLogs ) {
        logMessage = "Output: ";
    }

    while( fgets( buffer.data(), buffer.size(), pipe ) != nullptr ) {
        ss << buffer.data();
    }

    result = pclose( pipe );

    CommandResult commandResult;
    commandResult.output = ss.str();
    commandResult.exitCode = -1;  // Default value if not exited normally

    if( WIFEXITED( result ) ) {
        commandResult.exitCode = WEXITSTATUS( result );
    }

    commandResult.childPid = childPid;

    if( WIFSIGNALED( result ) ) {
        int signalNumber = WTERMSIG( result );
        commandResult.error = "Terminated by signal: " + cc::num( signalNumber );
    }

    if( WIFSTOPPED( result ) ) {
        int stopSignal = WSTOPSIG( result );
        commandResult.error = "Stopped by signal " + cc::num( stopSignal );
    }

    if( showAdvancedLogs ) {
        logMessage += ss.str();
        SystemUtils::logInfo() << logMessage << "\n";
    }

    return commandResult;
}

CommandResult unpackExitCode( int wrappedExitCode ) {
    CommandResult commandResult;
    commandResult.exitCode = -1;  // Default value if not exited normally

    if( WIFEXITED( wrappedExitCode ) ) {
        commandResult.exitCode = WEXITSTATUS( wrappedExitCode );
    }

    if( WIFSIGNALED( wrappedExitCode ) ) {
        int signalNumber = WTERMSIG( wrappedExitCode );
        commandResult.error = "Terminated by signal: " + cc::num10( signalNumber );
    }

    if( WIFSTOPPED( wrappedExitCode ) ) {
        int stopSignal = WSTOPSIG( wrappedExitCode );
        commandResult.error = "Stopped by signal " + cc::num10( stopSignal );
    }
    return commandResult;
}

void errorReport( const CommandResult& commandResult ) {
    if( !commandResult.error.empty() ) {
        SystemUtils::logError() << cc::error( "Command error: " + commandResult.output +
                                              ", Child process id: " + cc::num( commandResult.childPid ) +
                                              ", child process error: " + commandResult.error +
                                              ", exited with code: " + cc::num( commandResult.exitCode ) );
    }
}

}  // namespace System
}  // namespace HubPlatform
