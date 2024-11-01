#include "uid/handlers/hub_cloud_subscriptions_handler.h"
#include "uid/ui_service.h"

namespace HubPlatform {
namespace Uid {

HubCloudSubscriptionsHandler::HubCloudSubscriptionsHandler( Uid::Service& hostService )
: m_cloudDevices( hostService ) {}

void HubCloudSubscriptionsHandler::handleCloudSubscriptionCreate( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_cloudDevices.createCloudSubscription( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void HubCloudSubscriptionsHandler::handleCloudSubscriptionUpdate( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_cloudDevices.updateCloudSubscription( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void HubCloudSubscriptionsHandler::handleCloudSubscriptionDelete( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_cloudDevices.deleteCloudSubscription( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void HubCloudSubscriptionsHandler::handleCloudSubscriptionsList( rpc::Message& rpcMessage ) {
    try {
        rpcMessage.getAnswer()[ "result" ] = m_cloudDevices.getCloudSubscriptionsList();
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void HubCloudSubscriptionsHandler::handleCloudSubscriptionDispatch( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_cloudDevices.dispatchCloudSubscription( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

}  // namespace Uid
}  // namespace HubPlatform
