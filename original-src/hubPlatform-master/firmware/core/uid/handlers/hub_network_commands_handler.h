#pragma once

#include "networkd/remote_interface/network_remote_interface.h"

#include "jsonrpc/message.h"

#include <string>
#include <atomic>

namespace HubPlatform {
namespace Uid {

class HubNetworkCommandsHandler final {
public:
    HubNetworkCommandsHandler( Services::Service& hostService );

    void invoke( const std::string& method, rpc::Message& rpcMessage );

private:
    void handleGetRequest( rpc::Message& rpcMessage );
    void handleSettingsSetRequest( rpc::Message& rpcMessage );
    void handleResetRequest( rpc::Message& rpcMessage );
    void handleWifiScanStartRequest( rpc::Message& rpcMessage );
    void handleWifiScanStopRequest( rpc::Message& rpcMessage );
    void handleWifiTryConnectRequest( rpc::Message& rpcMessage );
    void handleModemTryConnectRequest( rpc::Message& rpcMessage );
    void handleModemPinCodeSetRequest( rpc::Message& rpcMessage );
    void handleModemPinCodeResetRequest( rpc::Message& rpcMessage );
    void handleModemSimLockSetRequest( rpc::Message& rpcMessage );
    void handleModemClearFPLMNRequest( rpc::Message& rpcMessage );
    void handleModemReRegisterRequest( rpc::Message& rpcMessage );
    void handleStatisticsSettingsGetRequest( rpc::Message& rpcMessage );
    void handleStatisticsSettingsSetRequest( rpc::Message& rpcMessage );
    void handleStatisticsGetRequest( rpc::Message& rpcMessage );
    void handleEnabledSetRequest( rpc::Message& rpcMessage );
    void handleCommunicationTest( rpc::Message& rpcMessage );

private:
    bool getAndValidateInterfaceField( const Json& message, Json& answer, std::string& result );
    bool isActiveNetworkInterface( const std::string& interfaceId );

private:
    Network::RemoteInterface m_networkdInterface;
    std::atomic< bool > m_isCommunicationManualTestStarted{ false };
};

}  // namespace Uid
}  // namespace HubPlatform
