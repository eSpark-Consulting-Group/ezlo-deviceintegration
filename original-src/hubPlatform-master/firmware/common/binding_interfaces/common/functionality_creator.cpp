#include "binding_interfaces/common/functionality_creator.h"

#include "addon_interfaces/addon.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {

#define SUBSCRIBE_ATTEMPTS_COUNT 5

PluginEventsSubscribeFunctionality::PluginEventsSubscribeFunctionality( Services::Service& hostService,
                                                                        const std::string& eventsEmitterName )
: m_host( hostService )
, m_eventsEmitterName( eventsEmitterName ) {}

PluginEventsSubscribeFunctionality::SubscriberId PluginEventsSubscribeFunctionality::subscribe(
    const PluginEventsSubscriber& subscriber ) {
    Json result;
    for( int i = 0; i < SUBSCRIBE_ATTEMPTS_COUNT; ++i ) {
        result = sendRequest( SUBSCRIBE_FOR_PLUGIN_EVENTS, subscriber.toJson() );
        if( result.count( "subscriberId" ) ) {
            return result.at( "subscriberId" );
        }
    }

    return "";
}

void PluginEventsSubscribeFunctionality::unsubscribe( const PluginEventsSubscriber& subscriber ) {
    auto method = prepareMethod( Services::Service::UnsubscribeMethod );
    method[ "params" ] = subscriber.toJson();
    sendMethodCall( std::move( method ) );
}

Json PluginEventsSubscribeFunctionality::sendRequest( MessageSubClasses requestType, Json data ) {
    return m_host.sendRequest( m_eventsEmitterName, requestType, data );
}

void PluginEventsSubscribeFunctionality::sendMethodCall( Json data ) {
    Services::Message message{ m_eventsEmitterName, JSON_RPC, CALL_METHOD, std::move( data ) };
    m_host.send( message );
}

FunctionalityCreator::FunctionalityCreator( Services::Service& hostService, HubAPIRestrictions hubAPIRestrictions )
: m_host( hostService )
, m_hubAPIRestrictions( hubAPIRestrictions ) {}

std::unique_ptr< PluginEventsSubscribeFunctionality > FunctionalityCreator::getPluginEventsSubscribeFunctionality(
    const std::string& eventsEmitterName ) {
    if( m_hubAPIRestrictions == HubAPIRestrictions::NO_ASYNC_RESTRICTION ) {
        return nullptr;
    }

    return std::make_unique< PluginEventsSubscribeFunctionality >( m_host, eventsEmitterName );
}

}  // namespace HubPlatform
