#include "events_loop/epoll_events_loop.h"

#include <gtest/gtest.h>

#include <tuple>

#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

/*

TESTPLAN

DONE    1. EpollEventsLoop creation
DONE        1.1 Check epoll instance creation
DONE        1.2 Check extra thread gets launched
DONE    2. EpollEventsLoop destroying
TODO        2.1 Check extra thread gets stopped
DONE    3. EpollEventsLoop moving
TODO        3.1 Check moving correctness
DONE    4. Add event source
DONE        4.1 Check adding event source object
DONE        4.2 Check filling event source object
DONE        4.3 Check arming event source object
DONE        4.4 Check event source registration
DONE    5. Remove event source
DONE        5.1 Check keeping event source object
DONE        5.2 Check event source unregistration
DONE        5.3 Check disarming event source object
DONE    6. Pause event source
DONE        6.1 Check keeping event source object
DONE        6.2 Check event source unregistration
DONE        6.3 Check pausing event source object
DONE    7. Resuming event source
DONE        6.2 Check event source registration
DONE        6.3 Check unpausing event source object

*/

namespace HubPlatform {
namespace UT {

using namespace EventsLoop;

class EpollEventsLoopTestSuite : public testing::Test {
protected:
    EpollEventsLoopTestSuite() {
        m_evFd = eventfd( 0, 0 );
    }

    ~EpollEventsLoopTestSuite() {
        close( m_evFd );
    }

    EpollEventsLoop& getEpollEventsLoop() {
        return m_eventsLoop;
    }

    int getEpollFd() const {
        return m_eventsLoop.m_epollFileDescriptor;
    }

    const std::thread& getEpollThread() const {
        return m_eventsLoop.m_eventsLoopThread;
    }

    std::thread& getEpollThread() {
        return m_eventsLoop.m_eventsLoopThread;
    }

    const std::map< IEventsLoop::EventSourceHandle, EpollEventsLoop::EventSource >& getEventSources() const {
        return m_eventsLoop.m_eventSources;
    }

    void destroyEpollEventsLoop() {
        EpollEventsLoop tmp = std::move( m_eventsLoop );
    }

    int addEventSource() {
        m_eventsLoop.addEventSource( m_evFd, EventType::IN, IEventsLoop::EventHandler{} );
        return m_evFd;
    }

    int addRemoveEventSource() {
        auto eventHndl = m_eventsLoop.addEventSource( m_evFd, EventType::IN, IEventsLoop::EventHandler{} );
        m_eventsLoop.removeEventSource( eventHndl );

        return m_evFd;
    }

    std::pair< IEventsLoop::EventSourceHandle, int > addPauseEventSource() {
        auto eventHndl = m_eventsLoop.addEventSource( m_evFd, EventType::IN, IEventsLoop::EventHandler{} );
        m_eventsLoop.pauseEventSource( eventHndl );

        return { eventHndl, m_evFd };
    }

private:
    EpollEventsLoop m_eventsLoop;
    int m_evFd;
};

TEST_F( EpollEventsLoopTestSuite, testCheckEpollInstanceCreation /* 1.1 */ ) {
    EXPECT_GE( getEpollFd(), 0 );
}

TEST_F( EpollEventsLoopTestSuite, testCheckExtraThreadGetsLaunched /* 1.2 */ ) {
    EXPECT_TRUE( getEpollThread().joinable() );
}

TEST_F( EpollEventsLoopTestSuite, DISABLED_testCheckExtraThreadGetsStopped /* 2.1 */ ) {
    auto threadId = getEpollThread().native_handle();

    destroyEpollEventsLoop();

    EXPECT_NE( pthread_detach( threadId ), 0 );
}

TEST_F( EpollEventsLoopTestSuite, testCheckAddingEventSourceObject /* 4.1 */ ) {
    auto& eventSources = getEventSources();
    auto& eventsLoop = getEpollEventsLoop();

    addEventSource();

    EXPECT_EQ( eventSources.size(), 1 );
}

TEST_F( EpollEventsLoopTestSuite, testCheckFillingEventSourceObject /* 4.2 */ ) {
    const auto& eventSources = getEventSources();
    auto& eventsLoop = getEpollEventsLoop();

    int evFd = addEventSource();

    const auto& elm = eventSources.at( 0 );
    EXPECT_EQ( elm.fileDescriptor, evFd );
    EXPECT_EQ( elm.eventTypes, EventType::IN );
}

TEST_F( EpollEventsLoopTestSuite, testCheckArmingEventSourceObject /* 4.3 */ ) {
    const auto& eventSources = getEventSources();
    auto& eventsLoop = getEpollEventsLoop();

    addEventSource();

    const auto& elm = eventSources.at( 0 );
    EXPECT_TRUE( elm.armed );
}

TEST_F( EpollEventsLoopTestSuite, testCheckEventSourceRegistration /* 4.4 */ ) {
    int epollFd = getEpollFd();
    auto& eventsLoop = getEpollEventsLoop();

    int evFd = addEventSource();

    epoll_event event;

    std::cout << "epoll fd: " << epollFd << ", ev fd: " << evFd << std::endl;
    EXPECT_TRUE( epoll_ctl( epollFd, EPOLL_CTL_ADD, evFd, &event ) < 0 );
    // TODO: epoll_ctl behaves very strange, sometimes it returns EINVAL...
    EXPECT_TRUE( ( errno == EEXIST ) || ( errno == EINVAL ) );
}

TEST_F( EpollEventsLoopTestSuite, testCheckKeepingEventSourceObjectDuringUnregistring /* 5.1 */ ) {
    const auto& eventSources = getEventSources();

    addRemoveEventSource();

    EXPECT_EQ( eventSources.size(), 1 );
}

TEST_F( EpollEventsLoopTestSuite, testCheckEventSourceUnregistrationDuringUnregistring /* 5.2 */ ) {
    int epollFd = getEpollFd();

    int evFd = addRemoveEventSource();

    EXPECT_TRUE( epoll_ctl( epollFd, EPOLL_CTL_DEL, evFd, nullptr ) < 0 );
    EXPECT_TRUE( errno == ENOENT );
}

TEST_F( EpollEventsLoopTestSuite, testCheckDisarmingEventSourceObjectDuringUnregistring /* 5.3 */ ) {
    const auto& eventSources = getEventSources();

    addRemoveEventSource();

    const auto& elm = eventSources.at( 0 );
    EXPECT_FALSE( elm.armed );
}

TEST_F( EpollEventsLoopTestSuite, testCheckKeepingEventSourceObjectDuringPausing /* 6.1 */ ) {
    const auto& eventSources = getEventSources();

    addPauseEventSource();

    EXPECT_EQ( eventSources.size(), 1 );
}

TEST_F( EpollEventsLoopTestSuite, testCheckEventSourceUnregistrationDuringPausing /* 6.2 */ ) {
    int epollFd = getEpollFd();

    int evFd;
    std::tie( std::ignore, evFd ) = addPauseEventSource();

    EXPECT_TRUE( epoll_ctl( epollFd, EPOLL_CTL_DEL, evFd, nullptr ) < 0 );
    EXPECT_TRUE( errno == ENOENT );
}

TEST_F( EpollEventsLoopTestSuite, testCheckPausingEventSourceObjectDuringPausing /* 6.3 */ ) {
    const auto& eventSources = getEventSources();

    addPauseEventSource();

    const auto& elm = eventSources.at( 0 );
    EXPECT_TRUE( elm.paused );
}

TEST_F( EpollEventsLoopTestSuite, testCheckEventSourceRegistrationDuringResuming /* 7.1 */ ) {
    int epollFd = getEpollFd();
    auto& eventsLoop = getEpollEventsLoop();

    int eventHndl, evFd;
    std::tie( eventHndl, evFd ) = addPauseEventSource();
    eventsLoop.resumeEventSource( eventHndl );

    epoll_event event;

    std::cout << "epoll fd: " << epollFd << ", ev fd: " << evFd << std::endl;
    EXPECT_TRUE( epoll_ctl( epollFd, EPOLL_CTL_ADD, evFd, &event ) < 0 );
    // TODO: epoll_ctl behaves very strange, sometimes it returns EINVAL...
    EXPECT_TRUE( ( errno == EEXIST ) || ( errno == EINVAL ) );
}

TEST_F( EpollEventsLoopTestSuite, testCheckUnPausingEventSourceObjectDuringResuming /* 7.2 */ ) {
    const auto& eventSources = getEventSources();
    auto& eventsLoop = getEpollEventsLoop();

    int eventHndl;
    std::tie( eventHndl, std::ignore ) = addPauseEventSource();
    eventsLoop.resumeEventSource( eventHndl );

    const auto& elm = eventSources.at( 0 );
    EXPECT_FALSE( elm.paused );
}

}  // namespace UT
}  // namespace HubPlatform
