#include "luad/modules/timer/timer_helper.h"

#include "luad/modules/timer/timer_manager.h"
#include "luad/modules/timer/timer_resources.h"

#include "luad/addon/lua_event_emitter.h"

#include <cassert>

namespace HubPlatform {
namespace Luad {

TimerHelper::TimerHelper( TimerManager& timerManager,
                          GroupName groupName,
                          TimerId&& timerId,
                          Services::PostponedTaskWPtr task,
                          std::string&& script,
                          Json&& scriptParams,
                          Badge< TimerHelper > )
: m_timerManager( timerManager )
, m_groupName( std::move( groupName ) )
, m_timerId( std::move( timerId ) )
, m_task( task )
, m_script( std::move( script ) )
, m_scriptParams( std::move( scriptParams ) ) {
    assert( !m_task.expired() );
}

TimerHelperPtr TimerHelper::create( TimerManager& timerManager,
                                    GroupName groupName,
                                    TimerId timerId,
                                    Services::PostponedTaskWPtr task,
                                    std::string script,
                                    Json scriptParams ) {
    auto result = std::make_shared< TimerHelper >( timerManager,
                                                   groupName,
                                                   std::move( timerId ),
                                                   task,
                                                   std::move( script ),
                                                   std::move( scriptParams ),
                                                   PrivateTag{} );
    timerManager.registerTimer( {}, groupName, result->m_timerId, result );
    return result;
}

TimerHelper::~TimerHelper() noexcept {
    m_timerManager.unregisterTimer( {}, m_groupName, m_timerId );
}

void TimerHelper::operator()() {
    auto scriptParams = m_scriptParams;
    try {
        scriptParams[ Resources::TimerId ] = m_timerId;
    }
    catch( std::exception& e ) {
        std::cout << "Caught Exception: " << e.what() << std::endl;
    }
    m_timerManager.getEventEmitter().localEmit( m_script, scriptParams );
}

const Json& TimerHelper::getScriptParams() const {
    return m_scriptParams;
}

void TimerHelper::setScriptParams( Json params ) {
    m_scriptParams = std::move( params );
}

Services::PostponedTaskWPtr TimerHelper::getTask() {
    return m_task;
}

}  // namespace Luad
}  // namespace HubPlatform
