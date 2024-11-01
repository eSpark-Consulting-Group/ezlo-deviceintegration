#pragma once

#include "jsonrpc/error_codes.h"
#include "bus/messagesclasses.h"

namespace HubPlatform {
namespace CoreD {

class Messenger {
public:
    virtual ~Messenger() = default;
    virtual JsonObject sendBackupRequest( const std::string& ServiceName,
                                          const JsonObject& request,
                                          MessageSubClasses processType ) = 0;

    virtual void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) = 0;
};

}  // namespace CoreD
}  // namespace HubPlatform
