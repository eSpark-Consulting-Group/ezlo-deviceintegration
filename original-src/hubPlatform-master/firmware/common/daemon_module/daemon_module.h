#pragma once

#include <string>

namespace tiny {
namespace command_line {
class parser;
}
}  // namespace tiny

namespace HubPlatform {

class DaemonModule {
public:
    DaemonModule();

    virtual ~DaemonModule();

    int run( int argc, char* argv[] );

private:
    static void handleSignal( int signal );

    virtual void defineDaemonParameters( tiny::command_line::parser& parser );

    virtual void runImpl() = 0;

    virtual void setBinaryName( const std::string& name ) = 0;

    virtual int restoreBackup( const std::string& tempBackupFolder ) = 0;

    virtual void onTerminate() = 0;

private:
    static DaemonModule* m_instance;
};

}  // namespace HubPlatform
