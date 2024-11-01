#pragma once

#include "jsonrpc/error_codes.h"
#include "bus/messagesclasses.h"

namespace HubPlatform {
namespace HardwareManager {

class Messenger {
public:
    virtual ~Messenger() = default;
    virtual void sendExecutePluginScript( const std::string& script,
                                          const Json& scriptParams,
                                          const std::string& operationId = "",
                                          bool isUserRunScript = false,
                                          int timeout = 60 ) = 0;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
