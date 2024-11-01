#include "uid/handlers/hub_notifications_handler.h"
#include "uid/ui_service.h"

namespace HubPlatform {
namespace Uid {

HubNotificationsHandler::HubNotificationsHandler( Uid::Service& hostService )
: m_notifications( hostService ) {}

void HubNotificationsHandler::handleSMTPSettingsSet( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_notifications.smtpSettingsSet( rpcMessage.getParams() );
        setErrorIfHas( joAnswer, response );
        joAnswer[ "result" ] = std::move( response );
        json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::WRONG_FIELD, e.what() );
    }
}

void HubNotificationsHandler::handleSMTPSettingsGet( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();
        joAnswer[ "result" ] = m_notifications.smtpSettingsGet();
    }
    catch( const std::exception& e ) {
        setError( rpcMessage.getAnswer(), Errors::INTERNAL_OPERATION_ERROR, e.what() );
    }
}

void HubNotificationsHandler::handleSMTPSend( rpc::Message& rpcMessage ) {
    try {
        auto& joAnswer = rpcMessage.getAnswer();

        auto response = m_notifications.sendEmail( rpcMessage.getParams() );
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
