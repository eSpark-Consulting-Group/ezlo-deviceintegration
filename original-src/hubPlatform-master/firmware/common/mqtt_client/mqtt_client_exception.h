#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Common {

class MQTTClientError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit MQTTClientError( const std::string& msg )
    : runtime_error( msg ){};
};

}  // namespace Common
}  // namespace HubPlatform
