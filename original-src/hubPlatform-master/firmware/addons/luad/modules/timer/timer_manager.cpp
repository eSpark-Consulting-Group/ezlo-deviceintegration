#include "luad/modules/timer/timer_manager.h"

#include "luad/modules/timer/timer_helper.h"
#include "luad/addon/lua_event_emitter.h"
#include "luad/modules/timer/timer_type.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "service_interfaces/task_loop.h"

#include "database_client/dbid.h"

#include <utility>

namespace HubPlatform {
namespace Luad {

TimerManager::TimerManager( LuaAddon::EventEmitter& eventEmitter,
                            Services::TaskLoop& taskLoop,
                            IPluginStatusEventsSubscription& pluginStatusEventsSubscription )
: m_eventEmitter( eventEmitter )
, m_taskLoop( taskLoop ) {
    pluginStatusEventsSubscription.subscribeOnPluginRestarted(
        [this]( const std::string& pluginId ) { stopTimersGroup( pluginId ); } );
    pluginStatusEventsSubscription.subscribeOnPluginTeardown(
        [this]( const std::string& pluginId ) { stopTimersGroup( pluginId ); } );
    pluginStatusEventsSubscription.subscribeOnPluginUninstall(
        [this]( const std::string& pluginId ) { stopTimersGroup( pluginId ); } );
}

TimerManager::~TimerManager() {
    std::vector< std::pair< GroupName, TimerId > > timerIds;

    for( const auto& group : m_timers ) {
        for( const auto& timer : group.second ) {
            timerIds.push_back( std::make_pair( group.first, timer.first ) );
        }
    }

    for( const auto& timerId : timerIds ) {
        stopTimer( timerId.first, timerId.second );
    }
}

TimerId TimerManager::setTimer( GroupName groupName,
                                TimerDuration delay,
                                std::string script,
                                Json scriptParams,
                                TimerType timerType ) {
    const TimerId timerId = generateUniqueId();
    setTimerWithId( std::move( groupName ), timerId, delay, std::move( script ), std::move( scriptParams ), timerType );

    return timerId;
}

void TimerManager::setTimerWithId( GroupName groupName,
                                   TimerId timerId,
                                   TimerDuration delay,
                                   std::string script,
                                   Json scriptParams,
                                   TimerType timerType ) {
    if( groupName.empty() ) {
        LOGGER_WARN_RAW( cc::warn( "Timer can't be set up, timer group name is empty" ) );
        return;
    }

    {
        std::lock_guard< std::mutex > locker{ m_mutex };
        if( getTimerHelper( groupName, timerId ) ) {
            LOGGER_ERROR_RAW( cc::error( "Timer " ) + cc::note( groupName ) + cc::error( "/" ) + cc::note( timerId ) +
                              cc::error( " exists already" ) );
            return;
        }
    }

    auto task = m_taskLoop.createPostponedTask( nullptr );
    task->setCallback( std::bind( &TimerHelper::operator(),
                                  TimerHelper::create( *this,
                                                       std::move( groupName ),
                                                       std::move( timerId ),
                                                       task,
                                                       std::move( script ),
                                                       std::move( scriptParams ) ) ) );

    const bool isPeriodic = TimerType::Periodic == timerType;
    task->start( delay, isPeriodic );
}

void TimerManager::stopTimer( const GroupName& groupName, const TimerId& timerId ) {
    std::unique_lock< std::mutex > locker{ m_mutex };

    auto timerHelper = getTimerHelper( groupName, timerId );
    if( !timerHelper ) {
        LOGGER_WARN( cc::warn( "Timer " ) + cc::note( groupName ) + cc::warn( "/" ) + cc::note( timerId ) +
                     cc::warn( " does not exist" ) );
        return;
    }

    if( auto timer = timerHelper->getTask().lock() ) {
        locker.unlock();
        timer->stop();
    }
}

void TimerManager::restartTimer( const GroupName& groupName, const TimerId& timerId, TimerDuration delay ) {
    std::unique_lock< std::mutex > locker{ m_mutex };

    auto timerHelper = getTimerHelper( groupName, timerId );
    if( !timerHelper ) {
        LOGGER_WARN( cc::warn( "Timer " ) + cc::note( groupName ) + cc::warn( "/" ) + cc::note( timerId ) +
                     cc::warn( " does not exist" ) );
        return;
    }

    if( auto timer = timerHelper->getTask().lock() ) {
        locker.unlock();
        timer->start( delay, timer->isPeriodic() );
    }
}

bool TimerManager::hasTimer( const GroupName& groupName, const TimerId& timerId ) const {
    std::lock_guard< std::mutex > locker{ m_mutex };
    return static_cast< bool >( getTimerHelper( groupName, timerId ) );
}

LuaAddon::EventEmitter& TimerManager::getEventEmitter() {
    return m_eventEmitter;
}

void TimerManager::registerTimer( Badge< TimerHelper >,
                                  const GroupName& groupName,
                                  const TimerId& id,
                                  TimerHelperWPtr timerHelper ) {
    LOGGER_TRACE( cc::trace( id ) );

    std::lock_guard< std::mutex > locker{ m_mutex };
    m_timers[ groupName ].emplace( id, timerHelper );
}

void TimerManager::unregisterTimer( Badge< TimerHelper >, const GroupName& groupName, const TimerId& id ) {
    LOGGER_TRACE( cc::trace( id ) );

    std::lock_guard< std::mutex > locker{ m_mutex };
    m_timers[ groupName ].erase( id );

    if( m_timers[ groupName ].empty() ) {
        m_timers.erase( groupName );
    }
}

void TimerManager::stopTimersGroup( const GroupName& groupName ) {
    try {
        std::unique_lock< std::mutex > locker{ m_mutex };
        std::vector< TimerId > timerIds;

        auto groupIt = m_timers.find( groupName );
        if( groupIt == m_timers.end() ) {
            return;
        }

        for( const auto& timer : groupIt->second ) {
            timerIds.push_back( timer.first );
        }

        locker.unlock();

        for( const auto& timerId : timerIds ) {
            stopTimer( groupName, timerId );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to free timer resources for group " ) + cc::note( groupName ) +
                      cc::error( ", reason: " ) + cc::note( ex.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Failed to free timer resources for group " ) + cc::note( groupName ) );
    }
}

TimerId TimerManager::generateUniqueId() const {
    return db::DbId::genOID();
}

TimerHelperPtr TimerManager::getTimerHelper( const GroupName& groupName, const TimerId& timerId ) const {
    auto groupIt = m_timers.find( groupName );
    if( groupIt != m_timers.end() ) {
        const auto& group = groupIt->second;

        auto it = group.find( timerId );
        if( it != group.end() ) {
            return it->second.lock();
        }
    }

    return nullptr;
}

const Json* TimerManager::getTimerParameters( const GroupName& groupName, const TimerId& timerId ) const {
    std::lock_guard< std::mutex > locker{ m_mutex };

    auto timerHelper = getTimerHelper( groupName, timerId );
    if( timerHelper ) {
        return &timerHelper->getScriptParams();
    }

    return nullptr;
}

void TimerManager::setTimerParameters( const GroupName& groupName, const TimerId& timerId, Json parameters ) {
    std::lock_guard< std::mutex > locker{ m_mutex };

    auto timerHelper = getTimerHelper( groupName, timerId );
    if( !timerHelper ) {
        LOGGER_WARN( cc::warn( "Timer " ) + cc::note( groupName ) + cc::warn( "/" ) + cc::note( timerId ) +
                     cc::warn( " does not exist" ) );
        return;
    }

    timerHelper->setScriptParams( std::move( parameters ) );
}

}  // namespace Luad
}  // namespace HubPlatform
