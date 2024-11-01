#include "uid/server/server_peer_messager.h"

server_peer_message::server_peer_message( server_peer* pPeer )
: rpc::TypedServerMessage< server_peer >( pPeer ) {}
server_peer_message::~server_peer_message() {}

std::string server_peer_message::userId() const {  // if not empty, then logged in
    auto px = getPeerObject();
    if( !px )
        return "";
    return px->userId();
}
bool server_peer_message::isLoggedIn() const {
    bool bIsLoggedIn = !userId().empty();
    return bIsLoggedIn;
}
