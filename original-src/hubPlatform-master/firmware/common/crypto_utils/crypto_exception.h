#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace CryptoUtils {

class CryptoException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

}  // namespace CryptoUtils
}  // namespace HubPlatform
