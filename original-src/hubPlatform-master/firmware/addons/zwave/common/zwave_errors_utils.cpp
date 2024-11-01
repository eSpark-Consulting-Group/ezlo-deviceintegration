
#include "zwave/common/zwave_errors_utils.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Zwave {

Json toJson( const znet::err::znet_error_message_t& errorInfo ) {
    Json result;
    result[ "code" ] = errorInfo.m_code;
    result[ "data" ] = errorInfo.m_data;
    if( !errorInfo.m_details.empty() ) {
        Json details;
        for( const auto& detail : errorInfo.m_details ) {
            details[ detail.first ] = detail.second;
        }
        result[ "details" ] = details;
    }

    return result;
}

void addErrorDetails( Json& result, const znet::ErrorOpt& errorDetails ) {
    if( errorDetails ) {
        Json errorInfo;
        errorInfo[ "code" ] = errorDetails->m_code;
        errorInfo[ "data" ] = errorDetails->m_data;
        if( !errorDetails->m_details.empty() ) {
            Json details;
            for( const auto& detail : errorDetails->m_details ) {
                details[ detail.first ] = detail.second;
            }
            errorInfo[ "details" ] = details;
        }

        result[ "error" ] = errorInfo;
    }
}

std::string getHighlightedMessage( const znet::err::znet_error_message_t& errorInfo ) {
    std::string message =
        cc::note( errorInfo.m_data ) + cc::debug( "(" ) + cc::num10( errorInfo.m_code ) + cc::debug( ")." );
    if( !errorInfo.m_details.empty() ) {
        message += cc::debug( " Details:" );
        for( const auto& element : errorInfo.m_details ) {
            message +=
                cc::debug( " " ) + cc::note( element.first ) + cc::debug( " = " ) + cc::note( element.second ) + ",";
        }
        message.back() = '.';
    }

    return message;
}

std::string getMessage( const znet::err::znet_error_message_t& errorInfo ) {
    std::string errorMessage = errorInfo.m_data + "(" + std::to_string( errorInfo.m_code ) + ").";
    if( !errorInfo.m_details.empty() ) {
        errorMessage += " Details:";
        for( const auto& element : errorInfo.m_details ) {
            errorMessage += " " + element.first + " = " + element.second + ",";
        }
        errorMessage.back() = '.';
    }

    return errorMessage;
}

}  // namespace Zwave
}  // namespace HubPlatform
