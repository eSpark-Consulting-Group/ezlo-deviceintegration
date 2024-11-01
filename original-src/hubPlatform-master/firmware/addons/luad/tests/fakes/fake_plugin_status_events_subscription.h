#pragma once

#include "binding_interfaces/common/plugin_status_events_subscription.h"

#include <vector>

namespace HubPlatform {
namespace Luad {
namespace UT {

class FakePluginStatusEventsSubscription : public IPluginStatusEventsSubscription {
public:
    void subscribeOnPluginRestarted( PluginStatusChangedCallback callback ) override {
        m_pluginRestartedCallbacks.emplace_back( std::move( callback ) );
    }

    void subscribeOnPluginTeardown( PluginStatusChangedCallback callback ) override {
        m_pluginTeardownCallbacks.emplace_back( std::move( callback ) );
    }

    void subscribeOnPluginUninstall( PluginStatusChangedCallback callback ) override {
        m_pluginUninstallCallbacks.emplace_back( std::move( callback ) );
    }

    using PluginStatusCallbacksCollection = std::vector< PluginStatusChangedCallback >;

    PluginStatusCallbacksCollection m_pluginRestartedCallbacks;
    PluginStatusCallbacksCollection m_pluginTeardownCallbacks;
    PluginStatusCallbacksCollection m_pluginUninstallCallbacks;
};

}  // namespace UT
}  // namespace Luad
}  // namespace HubPlatform
