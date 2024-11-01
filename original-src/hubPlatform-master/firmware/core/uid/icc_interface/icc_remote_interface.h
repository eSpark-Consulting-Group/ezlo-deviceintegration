#pragma once

#include "json_utils/json_fwd.h"

#include <string>
#include <chrono>

namespace HubPlatform {

namespace Services {
class Service;
}

namespace InterControllerCommunication {

class RemoteInterface final {
public:
    explicit RemoteInterface( Services::Service& hostService );

    JsonObject sendRequest( const std::string& controllerId, const JsonObject& request );

    using ReceiveCallback = std::function< void( const JsonObject& message ) >;
    using TimeoutCallback = std::function< void() >;

    void sendAsyncRequest( const std::string& controllerId,
                           const JsonObject& request,
                           ReceiveCallback receiveCallback,
                           TimeoutCallback timeoutCallback );

    void sendAsyncRequest( const std::string& controllerId,
                           const JsonObject& request,
                           ReceiveCallback receiveCallback );

    std::string getControllerIdByUUID( const std::string& controllerUuid ) const;
    std::string getControllerUUIDById( const std::string& controllerId ) const;

    void registerControllerConnection( const std::string& controllerId );
    void unregisterControllerConnection( const std::string& controllerId );

private:
    Services::Service& m_hostService;
    std::chrono::seconds m_iccRequestTimeout;
};

}  // namespace InterControllerCommunication
}  // namespace HubPlatform
