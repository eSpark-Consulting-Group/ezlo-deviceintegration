#pragma once

#include "test_utils/component_tests/ws_server/test_peer_message.h"
#include "base_utils/parser.h"

namespace HubPlatform {
namespace Tests {

class TestServerProto : public Parser< void( TestPeerMessage& ) > {
public:
    using CallMethod = std::function< void( const std::string& method, rpc::Message& rpcMessage ) >;

    TestServerProto();

    bool processCmd( TestPeerMessage& rpcMessage );

    static bool processCmd( rpc::Message& rpcMessage, CallMethod callMethod );

public:
    void unknown( TestPeerMessage& rpcMessage );
    void ping( TestPeerMessage& rpcMessage );
    void registerHub( TestPeerMessage& rpcMessage );
    void ignore( TestPeerMessage& rpcMessage );

};  /// class server_proto

}  // namespace Tests
}  // namespace HubPlatform
