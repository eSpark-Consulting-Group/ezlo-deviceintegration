#pragma once

#include "cored/remote_interface/cored_interface.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class ICoreManager {
public:
    virtual CoreDRemoteInterface::CoreDInterface& core() = 0;
    virtual void sync() = 0;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
