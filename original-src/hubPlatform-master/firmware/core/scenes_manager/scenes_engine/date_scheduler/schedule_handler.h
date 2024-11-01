#pragma once

#include "scenes_manager/scenes_engine/date_scheduler/date_scheduler.h"
#include "scenes_manager/scenes_engine/date_scheduler/date_range_scheduler.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace SceneEngine {

class IClock;
class SceneManager;

class ScheduleHandler {
public:
    struct SchedulePoint {
        Scene::SceneID sceneId;
        std::string name;
        std::string method;
        LocalTimePoint m_eventTime;

    public:
        bool operator<( const SchedulePoint& a ) const;
        bool isLess( const SchedulePoint& right ) const;
    };

    using SchedulePoints = std::multiset< SchedulePoint >;

public:
    ScheduleHandler( SceneManager& sceneManager, IClock& clock );
    ~ScheduleHandler();

    void start();
    void tearDown();

    void push( DateEventBasePtr event );
    void push( DateRangeEventBasePtr event );
    void erase( const Scene::SceneID& sceneId );

    void setClock( IClock& clock );
    void onSystemTimeChanged();

    SchedulePoints getSchedule() const;
    bool evaluateCurrentStateForEvent( DateRangeEventBase& event ) const;

private:
    void init();
    void threadHandler();

private:
    DateScheduler m_dateScheduler;
    DateRangeScheduler m_dateRangeScheduler;
    SceneManager& m_sceneManager;
    std::reference_wrapper< IClock > m_clock;

    std::thread m_thread;
    mutable std::mutex m_mutex;
    std::condition_variable m_queueCondVar;
    bool m_isEnabled;
    bool m_isTimeChanged;
    const std::chrono::milliseconds m_epsilon;
};

}  // namespace SceneEngine
