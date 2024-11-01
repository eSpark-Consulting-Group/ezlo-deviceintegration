#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Zwave {

class ColorError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class AuthModeError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class ValueError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class NodeNotFoundError : public std::runtime_error {
public:
    explicit NodeNotFoundError( uint8_t nodeId )
    : runtime_error{ "Node not found with id: " + std::to_string( nodeId ) } {}

    NodeNotFoundError( uint8_t nodeId, uint8_t channelId )
    : runtime_error( "Node not found. Node id: " + std::to_string( nodeId ) +
                     ", channel id: " + std::to_string( channelId ) ) {}

    NodeNotFoundError( uint8_t nodeId, uint8_t channelId, uint8_t classId )
    : runtime_error( "Node not found. Node id: " + std::to_string( nodeId ) +
                     ", channel id: " + std::to_string( channelId ) + ", classId: " + std::to_string( classId ) ) {}
};

class InvalidDoorLockMode : public std::runtime_error {
public:
    explicit InvalidDoorLockMode( const std::string& doorLockMode )
    : runtime_error{ "Invalid door lock mode: " + doorLockMode } {}
};

class InvalidUserIdStatus : public std::runtime_error {
public:
    explicit InvalidUserIdStatus( const std::string& userIdStatus )
    : runtime_error{ "Invalid user id status: " + userIdStatus } {}
};

class InvalidSensorType : public std::runtime_error {
public:
    explicit InvalidSensorType( const std::string& sensorType )
    : runtime_error{ "Invalid sensor type: " + sensorType } {}
};

class InvalidWeekDay : public std::runtime_error {
public:
    explicit InvalidWeekDay( const std::string& weekDay )
    : runtime_error{ "Invalid week day: " + weekDay } {}
};

class ProvisioningListEntryNotFound : public std::runtime_error {
public:
    explicit ProvisioningListEntryNotFound( uint8_t nodeId )
    : runtime_error{ "Provisioning info not found for node with id: " + std::to_string( nodeId ) } {}
};

}  // namespace Zwave
}  // namespace HubPlatform
