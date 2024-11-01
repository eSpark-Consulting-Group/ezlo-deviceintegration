#pragma once

#include "jsonrpc/message.h"
#include <tiny/tiny_ws.h>
#include <tiny/tiny_dispatch.h>
#include <string>
#include <functional>

typedef tiny::ws::server::peer server_peer_base;
class server_peer_message;

class server_peer : public server_peer_base {
public:
    typedef std::function< void( server_peer_message& ) > on_message_t;

protected:
    mutable tiny::dispatch::queue_id_t dispatch_queue_id_;
    mutable std::string cached_remote_ip_, cached_origin_;
    std::string strUserId_;  // if not empty, then logged in
    std::string strTypeOfUI_;
    on_message_t on_message;
    typedef tiny::multithreading::recursive_mutex_type mutex_type;
    typedef std::lock_guard< mutex_type > lock_type;
    // mutable mutex_type peer_mutex_;
public:
    static bool g_bSingleThreadedPeerMessages;  // this should be used for debug only
    static bool g_bSingleThreadedPeerAnswers;   // this should be used for debug only
    static bool g_bEnableMicroNmaPeerStatsLogging;
    server_peer( tiny::ws::server::server& srv, const tiny::ws::server::hdl_t& hdl, on_message_t on_message );
    server_peer( const server_peer& ) = delete;
    server_peer( server_peer&& ) = delete;
    virtual ~server_peer();
    server_peer operator=( const server_peer& ) = delete;
    server_peer operator=( server_peer&& ) = delete;

    mutex_type& peer_mtx() const;
    static server_peer* make_ptr( const server_peer* p ) {
        return const_cast< server_peer* >( p );
    }
    tiny::dispatch::queue_id_t get_dispatch_queue_id() const;
    std::string getRemoteIp() const override;
    std::string getOrigin() const override;
    //
    const std::string& userId() const;  // if not empty, then logged in
    const std::string& uiType() const;
    void onPeerRegister() override;
    void onPeerUnregister() override;
    static bool stat_peer_register( server_peer* pPeer );
    static bool stat_peer_unregister( server_peer* pPeer );
    static bool stat_peer_is_registered( server_peer* pPeer );
    typedef std::function< bool( server_peer* pPeer ) > fn_for_each_peer_t;
    static size_t stat_locked_for_each_peer( fn_for_each_peer_t fn );
    //
    void log_extended_peer_statistics( tiny::ws::server::server* pSrv = nullptr );
    struct stats_info {
        void clear();
        size_t cntPeers_ = 0;
        size_t cntUnknown_ = 0;
        size_t cntUIs_ = 0;
        size_t sum() const {
            return ( cntUnknown_ + cntUIs_ );
        }
        typedef tiny::string_set_t peer_descriptions_t;
        typedef std::map< std::string, peer_descriptions_t > map_peer_descriptions_t;
        map_peer_descriptions_t map_peer_descriptions_;
        std::string get_log_string() const;
    };  /// struct stats_info
    static void stat_peer_stats( stats_info& si, bool isColored );
    //
    std::string getShortTypeDescrition( bool isColored = false ) const override;
    std::string getShortPeerDescription( bool isColored = false ) const override;
    //
    bool isLoggedIn() const;
    virtual void onLoginConfirmed( const std::string& userId, bool isUser );
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
        return server_peer_base::toJSON( bSkipEmptyStats );
    }
};  /// class micro_nma_peer
