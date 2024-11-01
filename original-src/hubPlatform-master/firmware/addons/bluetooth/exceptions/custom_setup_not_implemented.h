#pragma once

#include <exception>

namespace HubPlatform {

class CustomSetupNotImplemented : public std::exception {
public:
    const char* what() const noexcept override;
};

}  // namespace HubPlatform
