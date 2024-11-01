#pragma once

#include "vidoo4hub/core_manager/core_manager_interface.h"
#include "cored/remote_interface/cored_interface.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class CoreManager : public ICoreManager {
public:
    CoreManager( CoreDRemoteInterface::CoreDInterface& core );
    CoreDRemoteInterface::CoreDInterface& core() override;
    void sync() override;

private:
    CoreDRemoteInterface::CoreDInterface& m_core;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
