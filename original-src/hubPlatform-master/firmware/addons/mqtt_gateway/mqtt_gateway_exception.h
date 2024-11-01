#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace MQTTGateway {

class MQTTGatewayError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit MQTTGatewayError( const std::string& msg )
    : runtime_error( msg ){};
};

class MQTTGatewayMessageError final : public MQTTGatewayError {
public:
    explicit MQTTGatewayMessageError( const std::string& message )
    : MQTTGatewayError( message ) {}
};

}  // namespace MQTTGateway
}  // namespace HubPlatform
