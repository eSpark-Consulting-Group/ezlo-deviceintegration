#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace System {

class SystemError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class PopenFailed : public SystemError {
public:
    explicit PopenFailed( const std::string& message );
};

class UntarFailed : public SystemError {
public:
    explicit UntarFailed( const std::string& message );
};

struct USBDescription {
    std::string idVendor;
    std::string idProduct;
};

struct CommandResult {
    std::string output;
    int exitCode{ -1 };
    pid_t childPid{ 0 };
    std::string error;
};

std::pair< std::string, int > getCommandOutput( std::string command,
                                                bool catchStderr = false,
                                                bool stderrToDevNull = true );
CommandResult getCommandOutputWithErrorReport( const char* command,
                                               bool catchStderr = false,
                                               bool stderrToDevNull = true );
void errorReport( const CommandResult& commandResult );
/** Unpacks exit code of the child process, returned by getCommandOutput/pclose/std::system. */
CommandResult unpackExitCode( int exitCode );

std::string getAwkOutput( const std::string& awkCommand );
std::string getAwkOutput( const char* awkCommand );

USBDescription getUSBDescription( const std::string& usbDevicePath );
std::string formatTimeoutCommand( const std::string& command, int timeoutSeconds );

void untarArchive( const std::string& archivePath, const std::string& directoryToExtractPath = "" );
void untarUnCompressedArchive( const std::string& archivePath, const std::string& directoryToExtractPath = "" );
bool tarArchive( const std::string& path, const std::string& archivePath );
std::string readFile( const std::string& filePath );

// Memory info in bytes
struct MemoryInfo {
    uint64_t totalram = 0;
    uint64_t freeram = 0;
    uint64_t sharedram = 0;
    uint64_t bufferram = 0;
    size_t pagesize = 0;
};

MemoryInfo getMemoryInfo();
size_t getMemoryPageSize();

void dropVirtualMemoryCaches();

}  // namespace System
}  // namespace HubPlatform
