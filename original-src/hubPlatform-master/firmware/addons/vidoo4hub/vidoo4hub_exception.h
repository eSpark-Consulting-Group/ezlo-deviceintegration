#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Vidoo4Hub {

class Vidoo4HubError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit Vidoo4HubError( const std::string& msg )
    : runtime_error( msg ){};
};

class Vidoo4HubMessageError final : public Vidoo4HubError {
public:
    explicit Vidoo4HubMessageError( const std::string& message )
    : Vidoo4HubError( message ) {}
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
