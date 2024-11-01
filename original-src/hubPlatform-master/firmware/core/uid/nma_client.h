#pragma once

#include "jsonrpc/message.h"
#include "device_model/h/permission.h"
#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "scenes_manager/remote_interface/scenes_remote_interface.h"
#include "service_interfaces/task_loop_timer.h"

#include <tiny/tiny_ws.h>
#include <tiny/tiny_url.h>
#include <tiny/tiny_atomic_shared_ptr.h>
#include <tiny/tiny_multithreading.h>
#include <tiny/tiny_dispatch.h>

#include <functional>
#include <chrono>
#include <mutex>
#include <thread>

namespace HubPlatform {
namespace Uid {

class UiProto;
class NmaProto;
class Service;

class NmaClient : public tiny::ws::client::client {
private:
    using Base = tiny::ws::client::client;
    using LogOutputFunction =
        std::function< void( NmaClient& ws, bool isRead, const std::string& msg, tiny::ws::client::opcv eOpCode ) >;
    struct PostponedMessage {
        std::string message;
        tiny::ws::client::opcv eOpCode;
    };

public:
    static bool g_bEnableNmaClientStatsLogging;
    static bool g_bSingleThreadedPeerMessages;  // this should be used for debug only
    static bool g_bSingleThreadedPeerAnswers;   // this should be used for debug only

public:
    NmaClient( Service& uiService,
               const tiny::ws::security_args& sa,
               LogOutputFunction logOutFunction,
               const std::string& serverUri );
    ~NmaClient() override;

    bool open( const std::string& uri ) override;
    void close() override;
    void closeConnection() override;
    void restoreConnection() override;
    void delay_close( uint64_t nMilliseconds );
    void setForceReAskCloudCoordinator( bool force );
    void setConnected( bool state ) override;
    void onLogMessage( tiny::ws::e_ws_log_message_type_t eWSLMT, const std::string& msg ) override;
    void onDisconnected() override;
    void onOpen( tiny::ws::client::hdl_t hdl ) override;
    void onClose( tiny::ws::client::hdl_t hdl,
                  const std::string& reason,
                  int local_close_code,
                  const std::string& local_close_code_as_str ) override;
    void onFail( tiny::ws::client::hdl_t hdl ) override;
    void onMessage( tiny::ws::client::hdl_t hdl, tiny::ws::client::opcv eOpCode, const std::string& msg ) override;
    //
    bool sendMessage( const std::string& msg, tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text ) override;
    bool postMessage( const std::string& msg, tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text );
    bool sendMessage( const JsonObject& joMsg ) {
        return sendMessage( joMsg.dump() );
    }
    bool postMessage( const JsonObject& joMsg ) {
        return postMessage( joMsg.dump() );
    }

    void onRestartConnectionTimer() override;

    //
    bool sendMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode = tiny::ws::client::opcv::text );
    bool postMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode = tiny::ws::client::opcv::text );
    bool sendMessageNoLog( const JsonObject& joMsg ) {
        return sendMessageNoLog( joMsg.dump() );
    }
    bool postMessageNoLog( const JsonObject& joMsg ) {
        return postMessageNoLog( joMsg.dump() );
    }

    UiProto& getUiProto();
    NmaProto& getNmaProto();
    void postponeMessage( const std::string& msg, tiny::ws::client::opcv eOpCode = tiny::ws::client::opcv::text );
    void postPostponedMessages();

    void establishConnection( const std::string& uri );

    bool isHubRegistered() const;
    void setHubRegistered( bool isHubRegistered );
    void onHubRegisterRepeat();

    void processBroadcast( const Services::Message& message );

    bool isConnecting() const;

protected:
    bool isNeedReAskCloudCoordinator() const;
    void setupPingInterval();
    void emitCloudConnectedEvent( const std::string& interfaceName );
    void emitCloudDisconnectedEvent( const std::string& interfaceName, const std::string& action );

private:
    Service& m_uiService;
    HardwareManager::RemoteInterface m_hardwareManager;
    ScenesManager::RemoteInterface m_scenesInterface;
    std::unique_ptr< UiProto > m_uiProto;
    std::unique_ptr< NmaProto > m_nmaProto;
    LogOutputFunction m_logOutFunction;
    Services::TaskLoopTimer m_closingTimer;
    std::atomic_bool m_forceReAskCloudCoordinator{ false };
    std::atomic_bool m_isHubRegistered{ false };
    std::atomic_bool m_connecting{ false };
    Services::PostponedTaskPtr m_registerRepeatTask = nullptr;

    std::atomic< std::chrono::system_clock::time_point::duration > m_ttDisconnectionCheckPoint;

    std::mutex m_postponedMutex;
    std::string m_activeNetworkInterface;
};  /// class NmaClient

// EFWR-4852 NEW: TRAITS
// Use tiny::retain_release_ptr because nma_client inherits tiny::ref_retain_release
using NmaClientPtr = tiny::retain_release_ptr< NmaClient, rpc::LegacySynchronizationTraits< NmaClient > >;

class nma_client_message : public rpc::TypedClientMessage< NmaClient > {
public:
    nma_client_message( NmaClient* pClient );
    ~nma_client_message() override;
};  /// class nma_client_message

}  // namespace Uid
}  // namespace HubPlatform
