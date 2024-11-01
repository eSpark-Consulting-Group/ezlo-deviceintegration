#pragma once

#include "vidoo4hub/tests/utils/test_utils.h"

#include "vidoo4hub/core_manager/core_manager_interface.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"
#include "cored/exceptions.h"

namespace HubPlatform {
namespace UT {

class FakeCoreManager : public Vidoo4Hub::ICoreManager {
public:
    FakeCoreManager( CoreDRemoteInterface::CoreDInterface& core )
    : m_core( core ) {}

    CoreDRemoteInterface::CoreDInterface& core() override {
        return m_core;
    }
    void sync() override {}

public:
    CoreDRemoteInterface::CoreDInterface& m_core;
};

}  // namespace UT
}  // namespace HubPlatform
