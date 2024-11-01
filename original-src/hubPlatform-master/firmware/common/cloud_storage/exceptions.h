#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

class CloudResponseStatusError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class WrongFieldError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform