#pragma once

#include "uid/server/server_peer.h"
#include "jsonrpc/message.h"

class server_peer_message : public rpc::TypedServerMessage< server_peer > {
public:
    server_peer_message( server_peer* pPeer );
    ~server_peer_message() override;
    std::string userId() const;  // if not empty, then logged in
    bool isLoggedIn() const;
};  /// class server_peer_message
