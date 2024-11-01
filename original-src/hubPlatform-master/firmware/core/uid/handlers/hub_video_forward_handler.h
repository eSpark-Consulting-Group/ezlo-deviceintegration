#pragma once

#include "jsonrpc/message.h"

namespace HubPlatform {
namespace Uid {

class Service;

class HubVideoForwardHandler final {
public:
    explicit HubVideoForwardHandler( Uid::Service& hostService );

    void handleVideoForward( rpc::Message& rpcMessage );

public:
    bool checkWebRtcEngineFeatureStatus( const std::string& apiVersion, Json& answer ) const;
    std::string selectApiVersion( const std::string& apiVersion, const Json& commandParams ) const;
    static std::string selectApiVersionBySettingOrPayload( const std::string& webrtcSettingValue,
                                                           const std::string& apiVersion,
                                                           const Json& commandParams );

    static std::string selectApiVersion( const std::string& apiVersion );
    static std::string selectApiVersionByPayload( const JsonObject& commandParams );
    static std::string selectTargetGroup( const std::string& apiVersion );

private:
    Uid::Service& m_service;
};

}  // namespace Uid
}  // namespace HubPlatform
