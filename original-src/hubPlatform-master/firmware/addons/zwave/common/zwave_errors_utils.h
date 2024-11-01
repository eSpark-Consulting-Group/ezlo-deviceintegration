#pragma once

#include "json_utils/json_fwd.h"
#include <znet/znet_error_message.hpp>

namespace HubPlatform {
namespace Zwave {

Json toJson( const znet::err::znet_error_message_t& errorInfo );

void addErrorDetails( Json& result, const znet::ErrorOpt& errorDetails );

std::string getHighlightedMessage( const znet::err::znet_error_message_t& errorInfo );

std::string getMessage( const znet::err::znet_error_message_t& errorInfo );

}  // namespace Zwave
}  // namespace HubPlatform
