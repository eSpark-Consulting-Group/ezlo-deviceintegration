#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept>

namespace HubPlatform {
namespace BackupUpdateManager {

class ScriptManagerError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit ScriptManagerError( const std::string& msg )
    : runtime_error( msg ){};
};

class ScriptManagerBase {
public:
    explicit ScriptManagerBase( std::chrono::seconds timeoutSec = std::chrono::seconds( 180 ) );
    virtual ~ScriptManagerBase() = default;

    void setScriptRunTimeout( std::chrono::seconds timeoutSec );
    virtual void startDaemon( const std::string daemonName, const std::vector< std::string > args ) = 0;
    virtual void startDaemons() = 0;
    virtual void stopDaemons() = 0;

    virtual int restoreBackupCommand( const std::string& binaryName, const std::string& targetDirectory ) = 0;

    virtual bool migrateData() = 0;
    virtual void resetData() = 0;

protected:
    std::chrono::seconds m_timeoutSec;
};

class ScriptManager : public ScriptManagerBase {
public:
    ScriptManager();
    explicit ScriptManager( std::chrono::seconds timeoutSec );

    void startDaemon( const std::string daemonName, const std::vector< std::string > args ) override;
    void startDaemons() override;
    void stopDaemons() override;

    int restoreBackupCommand( const std::string& binaryName, const std::string& targetDirectory ) override;

    bool migrateData() override;
    void resetData() override;

private:
    std::string getTimeoutCommand();
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
