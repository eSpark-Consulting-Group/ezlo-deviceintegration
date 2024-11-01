#pragma once

#include "addon_interfaces/addon.h"

#include "service_interfaces/task_loop_timer.h"

namespace HubPlatform {
namespace TwoGig {

class DummyAddon final : public Addons::Addon {
    using Base = Addons::Addon;

public:
    static const char* const Name;
    static const char* const Version;

public:
    DummyAddon();
    ~DummyAddon() override;

    CommonUtils::AddonVersion getVersion() const override;

protected:
    void setUp() override;
    void tearDown() override;

    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;

    std::unique_ptr< Services::TaskLoopTimer > m_discoveringTimeoutTask;
};

}  // namespace TwoGig
}  // namespace HubPlatform
