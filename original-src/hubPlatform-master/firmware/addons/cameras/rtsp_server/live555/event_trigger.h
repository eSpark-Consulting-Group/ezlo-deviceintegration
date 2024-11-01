#pragma once

#include "UsageEnvironment/UsageEnvironment.hh"

class TaskScheduler;
class UsageEnvironment;
class RTSPServer;

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

class EventTrigger final {
public:
    using TaskFunc = void( void* clientData );
    EventTrigger();
    EventTrigger( const UsageEnvironment* env, TaskFunc taskFunc );
    EventTrigger( EventTrigger&& );
    EventTrigger( const EventTrigger& ) = delete;
    ~EventTrigger();

    EventTrigger& operator=( EventTrigger&& );

    const EventTriggerId& getId() const;

    void release();

private:
    const UsageEnvironment* m_env;
    EventTriggerId m_triggerId;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
