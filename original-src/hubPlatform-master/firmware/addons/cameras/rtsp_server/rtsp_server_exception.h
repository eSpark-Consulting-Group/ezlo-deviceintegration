#pragma once

#include <stdexcept>

namespace HubPlatform {
namespace Cameras {

class RtspServerException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace Cameras
}  // namespace HubPlatform
