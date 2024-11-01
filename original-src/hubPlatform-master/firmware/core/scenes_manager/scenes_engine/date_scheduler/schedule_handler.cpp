#include "scenes_manager/scenes_engine/date_scheduler/schedule_handler.h"
#include "scenes_manager/scenes_engine/events/transactional_events_updater.h"
#include "scenes_manager/scenes_engine/helpers/clock_interface.h"
#include "scenes_manager/scenes_engine/scene_manager.h"

namespace SceneEngine {

ScheduleHandler::ScheduleHandler( SceneManager& sceneManager, IClock& clock )
: m_sceneManager( sceneManager )
, m_epsilon( std::chrono::milliseconds( 1000 ) )
, m_clock( clock )
, m_isEnabled( false )
, m_isTimeChanged( false ) {}

ScheduleHandler::~ScheduleHandler() {
    tearDown();
}

void ScheduleHandler::tearDown() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_isEnabled = false;
    lock.unlock();
    m_queueCondVar.notify_all();
    if( m_thread.joinable() ) {
        m_thread.join();
    }
}

void ScheduleHandler::start() {
    m_isEnabled = true;
    m_thread = std::thread( [this]() {
        init();
        while( true ) {
            std::unique_lock< std::mutex > lock( m_mutex );

            if( !m_isEnabled )
                break;

            m_queueCondVar.wait_for( lock, m_epsilon, [this] { return m_isTimeChanged || !m_isEnabled; } );
            m_isTimeChanged = false;
            threadHandler();
        }
    } );
}

void ScheduleHandler::push( DateEventBasePtr event ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    IClock& clocks = m_clock.get();
    m_dateScheduler.setTime( clocks.localTimeSinceEpoch(), clocks.timeZoneOffset() );
    m_dateScheduler.push( std::move( event ) );
    lock.unlock();
    m_queueCondVar.notify_all();
}

void ScheduleHandler::push( DateRangeEventBasePtr event ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_dateRangeScheduler.push( std::move( event ) );
    lock.unlock();
    m_queueCondVar.notify_all();
}

void ScheduleHandler::erase( const Scene::SceneID& sceneId ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_dateScheduler.eraseBySceneId( sceneId );
    m_dateRangeScheduler.eraseBySceneId( sceneId );
}

void ScheduleHandler::setClock( IClock& clock ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_clock = clock;
    m_dateScheduler.setTime( m_clock.get().localTimeSinceEpoch(), m_clock.get().timeZoneOffset() );
}

void ScheduleHandler::onSystemTimeChanged() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_isTimeChanged = true;
    lock.unlock();
    m_queueCondVar.notify_all();
}

ScheduleHandler::SchedulePoints ScheduleHandler::getSchedule() const {
    std::unique_lock< std::mutex > lock( m_mutex );
    SchedulePoints schedulePoints;
    auto localNow = m_clock.get().localTime();
    auto offset = m_clock.get().timeZoneOffset();

    const auto& dateSchedule = m_dateScheduler.getSchedule();
    for( const DateScheduler::EventEntry& entry : dateSchedule ) {
        const Scene& scene = entry.event->getScene();

        SchedulePoint point;
        // TODO: EFWR-4203: Q: Why last event time is used, not next time?
        point.m_eventTime.setLocalTime(
            entry.m_lastEvent.duration(), entry.m_lastEvent.offset(), entry.m_lastEvent.isLocalTime() );

        point.method = entry.event->getMethodName();
        point.sceneId = scene.getId();
        point.name = scene.getName();
        schedulePoints.emplace( std::move( point ) );
    }

    const auto& dateRangeEvents = m_dateRangeScheduler.getEvents();
    for( auto& event : dateRangeEvents ) {
        const Scene& scene = event->getScene();

        SchedulePoint point;

        point.m_eventTime.setLocalTime( GregorianCalendar::ClockDuration::max(), std::chrono::seconds( 0 ) );
        if( auto optionalClock = event->getNextDate( localNow, offset ) ) {
            point.m_eventTime = *optionalClock;
        }

        point.method = event->getMethodName();
        point.sceneId = scene.getId();
        point.name = scene.getName();
        schedulePoints.emplace( std::move( point ) );
    }
    return schedulePoints;
}

bool ScheduleHandler::evaluateCurrentStateForEvent( DateRangeEventBase& event ) const {
    const bool verbose = true;
    event.update( m_clock.get().localTime(), m_clock.get().timeZoneOffset(), verbose );
    return event.isInRange();
}

void ScheduleHandler::init() {
    m_dateScheduler.setTime( m_clock.get().localTimeSinceEpoch(), m_clock.get().timeZoneOffset() );
}

void ScheduleHandler::threadHandler() {
    const auto localNow = m_clock.get().localTime();
    const auto offset = m_clock.get().timeZoneOffset();
    m_dateScheduler.setTime( localNow.time_since_epoch(), offset );
    auto activatedDateEvents = m_dateScheduler.collectActivatedEvents();
    auto contextId = m_sceneManager.createSceneContext( JSON_EMPTY_OBJ );
    TransactionalEventsUpdater updater = m_sceneManager.makeEventsUpdater( contextId );

    for( const auto& dateEvent : activatedDateEvents )
        updater.requestTrigger( dateEvent );
    const bool updateVerbosely = false;

    m_dateRangeScheduler.update( localNow, offset, updater, updateVerbosely );
    updater.flush();

    for( const auto& dateEvent : activatedDateEvents )
        updater.requestResetSilent( dateEvent );
    updater.flush();
}

bool ScheduleHandler::SchedulePoint::isLess( const SchedulePoint& right ) const {
    return m_eventTime.toUtc() < right.m_eventTime.toUtc();
}

bool ScheduleHandler::SchedulePoint::operator<( const SchedulePoint& right ) const {
    return isLess( right );
}

}  // namespace SceneEngine
