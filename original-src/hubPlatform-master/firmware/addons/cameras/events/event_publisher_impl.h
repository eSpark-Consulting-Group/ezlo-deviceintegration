#pragma once

#include "cameras/events/event_publisher_interface.h"

#include <mutex>

namespace HubPlatform {
namespace Cameras {

template < typename EventType >
class EventPublisher : public IEventPublisher< EventType > {
public:
    using EventHandler = typename IEventPublisher< EventType >::EventHandler;
    using ListenerId = typename IEventPublisher< EventType >::ListenerId;

    void sendEvent( EventType ) override;
    ListenerId addListener( EventHandler func ) override;
    void removeListener( const ListenerId& id ) override;

private:
    ListenerId m_counter = 0;
    std::map< ListenerId, EventHandler > m_listeners;
    std::mutex m_mutex;
};

template < typename EventType >
void EventPublisher< EventType >::sendEvent( EventType event ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    for( const auto& listener : m_listeners ) {
        listener.second( std::move( event ) );
    }
}

template < typename EventType >
typename EventPublisher< EventType >::ListenerId EventPublisher< EventType >::addListener( EventHandler func ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    ListenerId id = m_counter++;
    m_listeners.emplace( id, std::move( func ) );
    return id;
}

template < typename EventType >
void EventPublisher< EventType >::removeListener( const ListenerId& id ) {
    std::lock_guard< std::mutex > guard( m_mutex );
    m_listeners.erase( id );
}

}  // namespace Cameras
}  // namespace HubPlatform
