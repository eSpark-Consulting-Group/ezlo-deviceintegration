#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace HubPlatform {
namespace CloudDevices {

class SubscriptionError : public std::runtime_error {
public:
    explicit SubscriptionError( const std::string& msg )
    : runtime_error{ msg } {}
};

class BadGatewayError : public SubscriptionError {
public:
    using SubscriptionError::SubscriptionError;
};

class GatewayNotFoundError : public SubscriptionError {
public:
    using SubscriptionError::SubscriptionError;
};

class SubscriptionBadParamError : public std::invalid_argument {
public:
    explicit SubscriptionBadParamError( const std::string& msg, std::string fieldName )
    : std::invalid_argument{ msg }
    , m_field( std::move( fieldName ) ) {}
    const std::string& getFieldName() const {
        return m_field;
    }

private:
    std::string m_field;
};

class SubscriptionCapabilityError : public SubscriptionError {
public:
    using SubscriptionError::SubscriptionError;
};

class SubscriptionVariableError : public SubscriptionError {
public:
    using SubscriptionError::SubscriptionError;
};

class SubscriptionItemNameError : public SubscriptionError {
public:
    using SubscriptionError::SubscriptionError;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
