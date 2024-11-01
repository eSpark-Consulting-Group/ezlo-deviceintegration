#pragma once

#include "jsonrpc/message.h"

#include "tiny/tiny_ws.h"

namespace HubPlatform {

namespace InterControllerCommunication {
class IccManager;
}

namespace Uid {

class UiProto;
class ControllerConnection;

class ControllerClient : public tiny::ws::client::client {
private:
    using Base = tiny::ws::client::client;

public:
    ControllerClient( ControllerConnection& controllerConnection );

    const std::string& getControllerId() const;
    std::string getControllerUuid() const;
    bool isHubRegistered() const;

    void onRestartConnectionTimer() override;
    void establishConnection();
    bool processCmd( rpc::Message& rpcMessage );

    void onDestruct();
    void onOpen( tiny::ws::client::hdl_t hdl ) override;
    void onClose( tiny::ws::client::hdl_t hdl,
                  const std::string& reason,
                  int local_close_code,
                  const std::string& local_close_code_as_str ) override;
    void onFail( tiny::ws::client::hdl_t hdl ) override;
    void onMessage( tiny::ws::client::hdl_t hdl, tiny::ws::client::opcv eOpCode, const std::string& msg ) override;

    bool sendMessage( const std::string& msg, tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text ) override;
    bool postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text );

private:
    void setHubRegistered( bool isHubRegistered );

    void sendLoginRequest();
    void handleLoginResponse( rpc::Message& rpcMessage );

    void onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) override;
    void logMessage( bool isRead, const std::string& msg, tiny::ws::client::opcv eOpCode );

    UiProto& getUiProto();
    InterControllerCommunication::IccManager& getIccManager();

private:
    ControllerConnection& m_controllerConnection;

    std::mutex m_connectionMutex;
    mutable std::mutex m_registrationMutex;
    std::condition_variable m_registrationConditionVariable;

    std::string m_loginRequestId;
    bool m_isHubRegistered;
    std::atomic_bool m_connectionProblem;

    std::shared_ptr< bool > m_destructed;

};  // class ControllerClient

// EFWR-4852 NEW: TRAITS
// Use tiny::retain_release_ptr because ControllerClient inherits tiny::ref_retain_release
using ControllerClientPtr =
    tiny::retain_release_ptr< ControllerClient, rpc::LegacySynchronizationTraits< ControllerClient > >;

class icc_client_message : public rpc::TypedClientMessage< ControllerClient > {
public:
    icc_client_message( ControllerClient* pClient );
    ~icc_client_message() override;
};  // class nma_client_message

}  // namespace Uid
}  // namespace HubPlatform
