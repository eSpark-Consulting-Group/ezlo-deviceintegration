#pragma once

#include "scenes_manager/scenes_engine/helpers/clock_interface.h"

namespace SceneEngine {

class TestClock : public IClock {
public:
    explicit TestClock( std::chrono::seconds startTime, std::chrono::hours timeZoneOffset )
    : m_timeZoneOffset( std::chrono::duration_cast< std::chrono::seconds >( timeZoneOffset ) ) {
        auto now = std::chrono::system_clock::now();
        m_clockOffset = now.time_since_epoch() - startTime;
    }
    explicit TestClock( std::chrono::seconds startTime, std::chrono::minutes timeZoneOffsetMinutes )
    : m_timeZoneOffset( std::chrono::duration_cast< std::chrono::seconds >( timeZoneOffsetMinutes ) ) {
        auto now = std::chrono::system_clock::now();
        m_clockOffset = now.time_since_epoch() - startTime;
    }

    std::chrono::system_clock::time_point greenwichMeanTimePoint() override {
        return std::chrono::system_clock::now() - m_clockOffset;
    }

    std::chrono::system_clock::duration greenwichMeanTimeSinceEpoch() override {
        return greenwichMeanTimePoint().time_since_epoch();
    }

    std::chrono::system_clock::time_point localTime() override {
        return greenwichMeanTimePoint() + timeZoneOffset();
    }

    std::chrono::system_clock::duration localTimeSinceEpoch() override {
        return localTime().time_since_epoch();
    }

    std::chrono::seconds timeZoneOffset() override {
        return m_timeZoneOffset;
    }

    std::chrono::seconds timeZoneOffset( std::chrono::system_clock::time_point timePointUTC ) const override {
        auto timestamp = std::chrono::system_clock::to_time_t( timePointUTC );
        std::tm timeInfo = *std::localtime( &timestamp );

        timeInfo.tm_isdst = -1;
        std::mktime( &timeInfo );
        return std::chrono::seconds( timeInfo.tm_gmtoff );
    }

private:
    std::chrono::system_clock::duration m_clockOffset{ 0 };
    std::chrono::seconds m_timeZoneOffset;
};

}  // namespace SceneEngine
