#pragma once

#include "jsonrpc/message.h"

#include "tiny/tiny_ws.h"
#include "tiny/tiny_dispatch.h"

#include <string>
#include <functional>

namespace HubPlatform {
namespace Tests {

class TestServerPeer : public tiny::ws::server::peer {
public:
    using OnMessageFunction = std::function< void( rpc::Message& ) >;
    using MutexType = tiny::multithreading::recursive_mutex_type;
    using LockType = std::lock_guard< MutexType >;

public:
    TestServerPeer( tiny::ws::server::server& srv,
                    const tiny::ws::server::hdl_t& hdl,
                    OnMessageFunction onMessage,
                    std::string name );

    TestServerPeer( const TestServerPeer& ) = delete;

    TestServerPeer( TestServerPeer&& ) = delete;

    virtual ~TestServerPeer();

    TestServerPeer operator=( const TestServerPeer& ) = delete;

    TestServerPeer operator=( TestServerPeer&& ) = delete;

    MutexType& peer_mtx() const;

    static TestServerPeer* make_ptr( const TestServerPeer* p ) {
        return const_cast< TestServerPeer* >( p );
    }

    tiny::dispatch::queue_id_t get_dispatch_queue_id() const;

    std::string getRemoteIp() const override;

    std::string getOrigin() const override;

    //
    const std::string& getSerial() const;  // if not empty, then logged in

    void onPeerRegister() override;

    void onPeerUnregister() override;

    typedef std::function< bool( TestServerPeer* pPeer ) > fn_for_each_peer_t;

    //
    std::string getShortTypeDescrition( bool isColored = false ) const override;

    std::string getShortPeerDescription( bool isColored = false ) const override;
    //
    bool isLoggedIn() const;

    void onHubRegistered( const std::string& serial );
    //
    bool sendMessage( const std::string& msg, tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text ) override;

    bool postMessage( const std::string& msg, tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    bool sendMessage( const JsonObject& jo ) {
        return sendMessage( jo.dump() );
    }

    bool postMessage( const JsonObject& joMsg ) {
        return postMessage( joMsg.dump() );
    }
    //
    bool sendMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    bool postMessageNoLog( const std::string& msg, tiny::ws::server::opcv eOpCode = tiny::ws::server::opcv::text );

    bool sendMessageNoLog( const JsonObject& joMsg ) {
        return sendMessageNoLog( joMsg.dump() );
    }

    bool postMessageNoLog( const JsonObject& joMsg ) {
        return postMessageNoLog( joMsg.dump() );
    }

    //
    void onMessage( const std::string& msg, tiny::ws::server::opcv eOpCode ) override;

    nlohmann::json toJSON( bool bSkipEmptyStats ) const override {
        return tiny::ws::server::peer::toJSON( bSkipEmptyStats );
    }

private:
    mutable tiny::dispatch::queue_id_t m_dispatchQueueId;
    mutable std::string m_cachedRemoteIp, m_cachedOrigin;

    std::string m_hubSerial;  // if not empty, then logged in

    OnMessageFunction m_onMessage;

    std::string m_name;

private:
    static bool g_bSingleThreadedPeerMessages;  // this should be used for debug only
    static bool g_bSingleThreadedPeerAnswers;   // this should be used for debug only
    static bool g_bEnableMicroNmaPeerStatsLogging;

};  /// class micro_nma_peer

}  // namespace Tests
}  // namespace HubPlatform