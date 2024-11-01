#pragma once

#include <memory>
#include <functional>

namespace HubPlatform {
namespace EventsLoop {

enum class EventType { NONE = 1 << 0, IN = 1 << 1, OUT = 1 << 2, RDHUP = 1 << 3, ERR = 1 << 4, HUP = 1 << 5 };

class IEventsLoop {
public:
    using EventSourceHandle = int64_t;
    using EventHandler = std::function< void( EventType eventType, EventSourceHandle eventSourceHandle ) >;

    virtual ~IEventsLoop();

    /* TODO: add 'updateEventSource' method */
    /* @brief register an @c eventHandler to be called when @c eventType happens on a fd */
    virtual EventSourceHandle addEventSource( int fileDescriptor,
                                              EventType eventType,
                                              EventHandler&& eventHandler ) = 0;
    /* @brief unregister en event source managed by an @c eventSourceHandle
     * @note -1 as an eventSourceHandle isn't an error, it just does nothing */
    virtual void removeEventSource( EventSourceHandle eventSourceHandle ) = 0;

    virtual void pauseEventSource( EventSourceHandle eventSourceHandle ) = 0;
    virtual void resumeEventSource( EventSourceHandle eventSourceHandle ) = 0;
};

EventType operator|( EventType a, EventType b );
EventType operator|=( EventType a, EventType b );
EventType operator&( EventType a, EventType b );
EventType operator&=( EventType a, EventType b );

std::unique_ptr< IEventsLoop > makeEpollEventsLoop();

}  // namespace EventsLoop
}  // namespace HubPlatform
