#pragma once

#include "extensions_manager/em_environment.h"
#include "extensions_manager/tests/fakes/em_fake_addon_utils.h"
#include "extensions_manager/tests/fakes/em_fake_plugin_utils.h"
#include "extensions_manager/tests/fakes/em_fake_messenger.h"
#include "extensions_manager/tests/fakes/em_fake_cored_interface.h"
#include "extensions_manager/tests/fakes/em_fake_scenes_manager_interface.h"
#include "database_client/fake_db/fake_db_client.h"
#include "service_interfaces/tests/fakes/fake_service.h"

namespace HubPlatform {
namespace ExtensionsManager {
namespace UT {

class FakeEnvironment final : public Environment {
public:
    AddonUtils& getAddonUtils() override {
        return m_fakeAddonUtils;
    }
    PluginUtils& getPluginUtils() override {
        return m_fakePluginUtils;
    }
    FakeMessenger& getMessenger() override {
        return m_fakeMessenger;
    }
    IDbClient& getDbClient() override {
        return m_fakeIDbClient;
    }
    const CoreDRemoteInterface::CoreDInterface& getCoreDInterface() const override {
        return m_fakeCoreDInterface;
    }
    CoreDRemoteInterface::CoreDInterface& getCoreDInterface() override {
        return m_fakeCoreDInterface;
    }

    const ScenesManager::BaseRemoteInterface& getScenesManagerInterface() const override {
        return m_fakeScenesManagerInterface;
    }
    ScenesManager::BaseRemoteInterface& getScenesManagerInterface() override {
        return m_fakeScenesManagerInterface;
    }

    bool isPluginUnavailable( const std::string& pluginId ) const {
        return false;
    }

private:
    FakeAddonUtils m_fakeAddonUtils;
    FakePluginUtils m_fakePluginUtils;
    FakeMessenger m_fakeMessenger;
    Services::UT::FakeService m_fakeService;
    HubPlatform::UT::FakeDbClient m_fakeIDbClient;
    CoreDRemoteInterface::UT::FakeCoreDInterface m_fakeCoreDInterface;
    ScenesManager::UT::FakeScenesManagerInterface m_fakeScenesManagerInterface;
};

}  // namespace UT
}  // namespace ExtensionsManager
}  // namespace HubPlatform
