#pragma once

#include <exception>

namespace HubPlatform {

class BadProperty : public std::exception {
public:
    const char* what() const noexcept override;
};

}  // namespace HubPlatform
