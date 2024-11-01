#pragma once

#include "json_utils/json_fwd.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Notifications {

class BaseRemoteInterface {
public:
    virtual ~BaseRemoteInterface() = default;

    virtual JsonObject smtpSettingsSet( const JsonObject& params ) = 0;
    virtual JsonObject smtpSettingsGet() = 0;
    virtual JsonObject sendEmail( const JsonObject& params ) = 0;
};

}  // namespace Notifications
}  // namespace HubPlatform
