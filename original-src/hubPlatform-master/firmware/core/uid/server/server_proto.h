#pragma once

#include "uid/server/server_peer_messager.h"
#include "base_utils/parser.h"

class server_proto : public Parser< void( server_peer_message& ) > {
public:
    server_proto();
    bool processCmd( server_peer_message& rpcMessage );
    bool allowEveryone( const std::string& strMethodName );
    static void unknown( server_peer_message& rpcMessage );
    static void loginUser( server_peer_message& rpcMessage );
    static void loginController( server_peer_message& rpcMessage );
};  /// class server_proto
