#pragma once

#include "test_utils/component_tests/ws_server/test_server_peer.h"
#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Tests {

class TestPeerMessage : public rpc::TypedServerMessage< TestServerPeer > {
public:
    TestPeerMessage( TestServerPeer* peer );

    ~TestPeerMessage() override;

    std::string getSerial() const;
    bool isLoggedIn() const;

};  /// class server_peer_message

}  // namespace Tests
}  // namespace HubPlatform