#pragma once

#include "binding_interfaces/common/hub_api_restrictions.h"
#include "plugin_events_subscriber/plugin_events_subscriber.h"

#include "service_interfaces/service.h"

#include <string>
#include <memory>

namespace HubPlatform {

class PluginEventsSubscribeFunctionality final {
public:
    PluginEventsSubscribeFunctionality( Services::Service& hostService, const std::string& eventsEmitterName );

    using SubscriberId = std::string;

    SubscriberId subscribe( const PluginEventsSubscriber& subscriber );
    void unsubscribe( const PluginEventsSubscriber& subscriber );

private:
    void sendMethodCall( Json data );
    Json sendRequest( MessageSubClasses requestType, Json data );

private:
    Services::Service& m_host;
    std::string m_eventsEmitterName;
};

class FunctionalityCreator final {
public:
    FunctionalityCreator( Services::Service& hostService, HubAPIRestrictions hubAPIRestrictions );

    // eventsEmitterName - qualified name of an events emitter
    std::unique_ptr< PluginEventsSubscribeFunctionality > getPluginEventsSubscribeFunctionality(
        const std::string& eventsEmitterName );

private:
    Services::Service& m_host;
    HubAPIRestrictions m_hubAPIRestrictions;
};

}  // namespace HubPlatform
