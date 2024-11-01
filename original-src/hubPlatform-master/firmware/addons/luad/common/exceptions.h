#pragma once

#include <stdexcept>

namespace HubPlatform {

class OverriddenLuaFunctionError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

}  // namespace HubPlatform
