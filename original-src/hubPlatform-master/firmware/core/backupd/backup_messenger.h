#pragma once

#include "jsonrpc/error_codes.h"
#include "bus/messagesclasses.h"

namespace HubPlatform {
namespace BackupUpdateManager {

class Messenger {
public:
    virtual ~Messenger() = default;
    virtual void sendProgressStatus( MessageSubClasses processType, std::string status, int percentage ) = 0;
    virtual void sendProgressError( MessageSubClasses processType,
                                    Errors::Error error,
                                    const std::string& reason = "" ) = 0;
    virtual void sendBackupRequest( const std::string& serviceName,
                                    const std::string& intermediateBackupDirectory,
                                    const Json& config,
                                    std::function< void( const JsonObject& message ) > responseCallback ) = 0;
    virtual void sendVeraBackupRequest( const std::string& serviceName,
                                        const std::string& intermediateBackupDirectory,
                                        const std::string& action,
                                        std::function< void( const JsonObject& message ) > responseCallback ) = 0;
};

}  // namespace BackupUpdateManager
}  // namespace HubPlatform
