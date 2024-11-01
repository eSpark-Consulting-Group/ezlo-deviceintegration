#pragma once

#include "events_loop/events_loop.h"

#include <map>
#include <mutex>
#include <thread>
#include <atomic>

namespace HubPlatform {

namespace UT {
class EpollEventsLoopTestSuite;
}

namespace EventsLoop {

/* @brief implements an events loop as a thread waiting for events on fds using epoll functionality */
class EpollEventsLoop final : public IEventsLoop {
    friend class UT::EpollEventsLoopTestSuite;

public:
    EpollEventsLoop();
    ~EpollEventsLoop() override;

    EpollEventsLoop( const EpollEventsLoop& rhs ) = delete;
    EpollEventsLoop& operator=( const EpollEventsLoop& rhs ) = delete;
    EpollEventsLoop( EpollEventsLoop&& rhs );

    EventSourceHandle addEventSource( int fileDescriptor, EventType eventType, EventHandler&& eventHandler ) override;
    void removeEventSource( EventSourceHandle eventSourceHandle ) override;

    void pauseEventSource( EventSourceHandle eventSourceHandle ) override;
    void resumeEventSource( EventSourceHandle eventSourceHandle ) override;

private:
    struct EventSource final {
        EventSource();

        EventSourceHandle eventSourceHandle;
        EventHandler eventHandler;
        EventType eventTypes;
        int fileDescriptor;
        bool armed;
        bool paused;
    };

private:
    void handleEvents();

private:
    int m_epollFileDescriptor;
    std::thread m_eventsLoopThread;

    std::map< EventSourceHandle, EventSource > m_eventSources;
    std::mutex m_eventSourcesMtx;
    std::atomic_bool m_isThreadLaunched;

    EventSourceHandle m_eventSourceCounter;
};

}  // namespace EventsLoop
}  // namespace HubPlatform
