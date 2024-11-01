#pragma once

#include "service_interfaces/task_loop.h"
#include "jsonrpc/message.h"

#include "tiny/tiny_ws.h"

namespace HubPlatform {
namespace Tests {

class TestClient : public tiny::ws::client::client {
public:
    using OnMessageFunction = std::function< void( rpc::Message& ) >;

private:
    using Base = tiny::ws::client::client;

public:
    TestClient( Services::TaskLoop& taskLoop, OnMessageFunction onMessage, const tiny::ws::security_args& sa );
    ~TestClient() override;

    const std::string& getControllerUuid() const;
    const std::string& getToken() const;
    bool isHubRegistered() const;

    bool establishConnection( const std::string& serverUrl,
                              const std::string& controllerUuid,
                              const std::string& token );
    bool processCmd( rpc::Message& rpcMessage );

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
    bool handleLoginResponse( rpc::Message& rpcMessage );

    void onRestartConnectionTimer() override;
    void onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) override;
    void logMessage( bool isRead, const std::string& msg, tiny::ws::client::opcv eOpCode );

private:
    Services::TaskLoop& m_taskLoop;
    OnMessageFunction m_onMessage;

    std::string m_controllerUuid;
    std::string m_token;

    std::mutex m_connectionMutex;
    std::mutex m_registrationMutex;
    std::condition_variable m_registrationConditionVariable;

    std::atomic_bool m_isHubRegistered;
    std::atomic_bool m_connectionProblem;

};  // class ControllerClient

// Use tiny::retain_release_ptr because TestClient inherits tiny::ref_retain_release
using TestClientPtr = tiny::retain_release_ptr< TestClient, rpc::LegacySynchronizationTraits< TestClient > >;

class test_client_message : public rpc::TypedClientMessage< TestClient > {
public:
    test_client_message( TestClient* pClient );
    ~test_client_message() override;
};  // class nma_client_message

}  // namespace Tests
}  // namespace HubPlatform
