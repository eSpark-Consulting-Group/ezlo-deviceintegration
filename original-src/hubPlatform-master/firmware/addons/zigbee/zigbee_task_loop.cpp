#include "zigbee/zigbee_task_loop.h"

#include "common_utils/cpp14.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

constexpr std::chrono::milliseconds ZigbeeLoopInterval( 20 );

std::unique_ptr< ZigbeeTaskLoop > ZigbeeTaskLoop::ms_taskLoop{ nullptr };
std::thread ZigbeeTaskLoop::ms_zigbeeThread{};

ZigbeeTaskLoop& ZigbeeTaskLoop::init( std::function< void() > initAction ) {
    ms_taskLoop.reset( new ZigbeeTaskLoop( initAction ) );
    ms_zigbeeThread = std::thread( &TaskLoop::run, ms_taskLoop.get() );

    auto loopTask = ms_taskLoop->createPostponedTask( [initAction]( Services::PostponedTask& ) { initAction(); } );
    loopTask->setPeriodicMode( Services::PostponedTask::PeriodicMode::FixedIdleDuration );
    loopTask->start( Interval( 0 ), ZigbeeLoopInterval );

    return getInstance();
}

ZigbeeTaskLoop& ZigbeeTaskLoop::getInstance() {
    assert( ms_taskLoop );
    return *ms_taskLoop;
}

void ZigbeeTaskLoop::stopLoop() {
    assert( ms_taskLoop );
    ms_taskLoop->addTask( []() { ms_taskLoop->stop(); } );
    if( ms_zigbeeThread.joinable() ) {
        ms_zigbeeThread.join();
    }
}

ZigbeeTaskLoop::ZigbeeTaskLoop( std::function< void() > initAction )
: Services::TaskLoop() {}

}  // namespace Zigbee
}  // namespace HubPlatform
