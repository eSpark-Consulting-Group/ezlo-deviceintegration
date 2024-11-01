#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace VhubManager {

class VhubManagerError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit VhubManagerError( const std::string& msg )
    : runtime_error( msg ){};
};

class FirmwareStartError final : public VhubManagerError {
public:
    explicit FirmwareStartError( const std::string& message )
    : VhubManagerError( message ) {}
};

class FirmwareStopError final : public VhubManagerError {
public:
    explicit FirmwareStopError( const std::string& message )
    : VhubManagerError( message ) {}
};

class ProvisionDataError final : public VhubManagerError {
public:
    explicit ProvisionDataError( const std::string& message )
    : VhubManagerError( message ) {}
};

class VhubManagerMessageError final : public VhubManagerError {
public:
    explicit VhubManagerMessageError( const std::string& message )
    : VhubManagerError( message ) {}
};

}  // namespace VhubManager
}  // namespace HubPlatform
