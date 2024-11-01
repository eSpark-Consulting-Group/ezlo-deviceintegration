#include "uid/handlers/hub_video_forward_handler.h"
#include "service_interfaces/service.h"
#include "uid/ui_service.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace Uid {

HubVideoForwardHandler::HubVideoForwardHandler( Uid::Service& hostService )
: m_service( hostService ) {}

void HubVideoForwardHandler::handleVideoForward( rpc::Message& rpcMessage ) {
    try {
        const JsonObject& joParams = rpcMessage.getParams();
        auto& joAnswer = rpcMessage.getAnswer();
        auto peer = rpcMessage.getNetworkObject();

        auto callback = [peer, joAnswer]( const JsonObject& response ) mutable {
            setErrorIfHas( joAnswer, response );
            joAnswer[ "result" ] = response;
            json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );

            if( peer ) {
                peer->sendMessage( joAnswer );
            }
        };

        auto apiVersion = json_get_str( rpcMessage.getMessage(), "api" );
        if( apiVersion.empty() )
            apiVersion = "1.0";
        apiVersion = selectApiVersion( apiVersion, joParams );

        // check if webrtc engine with such version is present:
        if( checkWebRtcEngineFeatureStatus( apiVersion, joAnswer ) ) {
            auto targetGroup = selectTargetGroup( apiVersion );
            LOGGER_INFO_RAW( cc::info( "Send hub.videoforward to: " ) + cc::note( targetGroup ) );

            m_service.sendAsyncRequest( targetGroup,
                                        joParams,
                                        MessageSubClasses::HUB_VIDEOFORWARD,
                                        std::move( callback ),
                                        std::chrono::minutes( 2 ) );
            joAnswer = nullptr;
        }
        else {
            LOGGER_ERROR_RAW( cc::error( "WebRtc feature is not enabled" ) );
        }
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

/** Select hub.videoforward command version: if empty or 1.0 is passed, and hub setting 'webrtc.version' is present -
 *      override command's api version with value pointed by 'webrtc.version' hub setting. If 'auto', select version by
 * payload. */
std::string HubVideoForwardHandler::selectApiVersion( const std::string& apiVersion, const Json& commandParams ) const {
    auto coredPtr = CoreDRemoteInterface::makeInstance( m_service );
    if( auto webrtcVersion = coredPtr->findSetting( "webrtc.version" ) ) {
        const std::string webrtcSettingValue = webrtcVersion->getTokenValue();
        return selectApiVersionBySettingOrPayload( webrtcSettingValue, apiVersion, commandParams );
    }
    return apiVersion;
}

std::string HubVideoForwardHandler::selectApiVersionBySettingOrPayload( const std::string& webrtcSettingValue,
                                                                        const std::string& apiVersion,
                                                                        const Json& commandParams ) {
    std::string result = apiVersion;
    if( webrtcSettingValue == "2" ) {
        LOGGER_TRACE( cc::trace( "WebRTC: Override API version to 2.0" ) );
        result = selectApiVersion( apiVersion );
    }
    else if( webrtcSettingValue == "auto" ) {
        auto version = selectApiVersionByPayload( commandParams );
        if( version.empty() )
            version = apiVersion;
        if( version != apiVersion ) {
            LOGGER_TRACE( cc::trace( "WebRTC version automatically selected by payload: " ) + version );
        }
        result = version;
    }
    if( result.empty() )
        result = "1.0";
    return result;
}

std::string HubVideoForwardHandler::selectApiVersion( const std::string& apiVersion ) {
    if( apiVersion.empty() || apiVersion == "1.0" ) {
        return "2.0";
    }
    return apiVersion;
}

/** Selects webrtc engine by command payload. Temporary. for firmware versions with both webrtc engines onboard.
 * Docs for hub.videoforward: https://log.ezlo.com/new/hub/linux_controller_specific/#hubvideoforward */
std::string HubVideoForwardHandler::selectApiVersionByPayload( const JsonObject& commandParams ) {
    auto dataParamIt = commandParams.find( "data" );
    auto targetParamIt = commandParams.find( "target" );

    if( dataParamIt != commandParams.end() && targetParamIt != commandParams.end() ) {
        return "1.0";
    }

    auto v2Action = commandParams.find( "action" );
    Json sdp = JSON_EMPTY_OBJ;
    auto hasSdp = json_get( commandParams, { "params", "sdp" }, sdp );

    if( v2Action != commandParams.end() && hasSdp ) {
        return "2.0";
    }

    auto apiVersion = json_get_str( commandParams, "api" );
    return apiVersion;
}

std::string HubVideoForwardHandler::selectTargetGroup( const std::string& apiVersion ) {
    auto targetGroup = GroupsNames::WEBRTC;
    if( apiVersion == "2.0" ) {
        targetGroup = GroupsNames::WEBRTC_V2;
    }
    return targetGroup;
}

bool HubVideoForwardHandler::checkWebRtcEngineFeatureStatus( const std::string& apiVersion, Json& answer ) const {
    CoreDRemoteInterface::CoreDRemoteInterface cored( m_service );
    auto webrtcVersion1Status = cored.checkFeatureStatus( "webrtc" );
    if( ( apiVersion.empty() || apiVersion == "1.0" ) && !webrtcVersion1Status ) {
        setError( answer, Errors::FIRMWARE_FEATURE_DISABLED, "webrtc engine 1.0 is not enabled" );
        return false;
    }
    auto webrtcVersion2Status = cored.checkFeatureStatus( "mqttwebrtc.v2" );
    if( apiVersion == "2.0" && !webrtcVersion2Status ) {
        setError( answer, Errors::FIRMWARE_FEATURE_DISABLED, "webrtc engine 2.0 is not enabled" );
        return false;
    }

    return true;
}

}  // namespace Uid
}  // namespace HubPlatform
