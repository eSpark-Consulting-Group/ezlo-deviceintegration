#pragma once

#include <string>
#include <functional>

namespace HubPlatform {
namespace Luad {

class IPluginStatusEventsSubscription {
public:
    virtual ~IPluginStatusEventsSubscription() = default;

    using PluginStatusChangedCallback = std::function< void( const std::string& ) >;

    virtual void subscribeOnPluginRestarted( PluginStatusChangedCallback callback ) = 0;
    virtual void subscribeOnPluginTeardown( PluginStatusChangedCallback callback ) = 0;
    virtual void subscribeOnPluginUninstall( PluginStatusChangedCallback callback ) = 0;
};

}  // namespace Luad
}  // namespace HubPlatform
