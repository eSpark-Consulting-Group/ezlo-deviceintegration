#pragma once

#include "jsonrpc/message.h"

#include <deque>

class server_peer_message;

namespace HubPlatform {

namespace Uid {
class Service;
class icc_client_message;
}  // namespace Uid

namespace InterControllerCommunication {

class IccManager final {
public:
    explicit IccManager( Uid::Service& uiService );

    bool processDirectCmd( rpc::Message& rpcMessage, const std::string& controllerUuid );
    bool processCloudCmd( rpc::Message& rpcMessage );
    void handleCloudControllerResponse( rpc::Message& rpcMessage );
    void handleRequest( const JsonObject& request, const std::string& uuid, const std::string& sender );

private:
    bool sendAsyncResponse( const std::string& messageId, const Json& message, const std::string& uuid );
    void cleanOldMessageIds();

private:
    Uid::Service& m_uiService;

    using SenderInfo = std::pair< std::string, std::string >;
    using SenderByMessageIds = std::unordered_map< std::string, SenderInfo >;
    SenderByMessageIds m_senderByMessageIds;

    using MessageIdsWithTimestamps = std::deque< std::pair< std::string, std::chrono::steady_clock::time_point > >;
    MessageIdsWithTimestamps m_messageIdsWithTimestamps;
};

}  // namespace InterControllerCommunication
}  // namespace HubPlatform
