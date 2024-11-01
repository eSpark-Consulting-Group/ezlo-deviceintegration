#pragma once

#include <chrono>
#include <functional>

namespace HubPlatform {
namespace BackupUpdateManager {

class IVeraBackupManager {
public:
    using OnFinishedCallback = std::function< void( std::string ) >;
    virtual ~IVeraBackupManager() = default;
    virtual void setUp( OnFinishedCallback onFinished ) = 0;
    virtual void start( std::string backupDirectory ) = 0;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
