#pragma once

#include <ctime>
#include <memory>
#include <chrono>
#include <mutex>
#include <functional>

namespace SceneEngine {
// it's already implemented in C++ 20 !!!
// remove in the future
// main idea use : it becomes able for using the tests !!!
namespace UT {
class EnvironmentTimeTest;
}
class EnvironmentTime final {
public:
    using Clock = std::function< std::time_t() >;

public:
    EnvironmentTime();
    EnvironmentTime( const EnvironmentTime& cpy ) = delete;
    EnvironmentTime& operator=( const EnvironmentTime& cpy ) = delete;

    /* brief : just wrapper for std::time(nullptr);
     * could be used for converting it by :
     * std::localtime, std::gmtime */
    std::time_t getTime() const;
    std::chrono::seconds getTimeFromMidnight( const std::time_t& tm ) const;
    std::chrono::seconds getTimeToMidnight( const std::time_t& tm ) const;
    long int getTimeZoneOffset() const;

    std::time_t getTomorrow( const std::time_t& value ) const;
    std::tm getTomorrow( const std::tm& value ) const;

    std::time_t calcDaysOffset( const std::time_t& value, int days ) const;
    int getDayOfWeek( const std::time_t& value ) const;

    int getYear( const std::time_t& value ) const;
    int getMonth( const std::time_t& value ) const;
    int getDayOfMonth( const std::time_t& value ) const;
    int getDaysInMonth( const std::time_t& value ) const;

    void setClock( Clock clock );

private:
    friend UT::EnvironmentTimeTest;
    // TODO: implement the wrapper for the time
    // and pass it for processing.
    // Because getting time point should be atomic.
    // No other interfaces must not update it by accessing system
    class EnvironmentTimePimpl {
    public:
        EnvironmentTimePimpl();
        virtual ~EnvironmentTimePimpl() = default;
        virtual std::time_t getTime() const;
        virtual std::chrono::seconds timeFromMidnight( const std::time_t& tm ) const;
        virtual std::chrono::seconds timeToMidnight( const std::time_t& tm ) const;
        virtual long int getTimeZoneOffset() const;
        virtual std::time_t getTomorrow( const std::time_t& value ) const;
        virtual std::tm getTomorrow( const std::tm& value ) const;
        virtual std::time_t calcDaysOffset( const std::time_t& value, int days ) const;
        virtual int getDayOfWeek( const std::time_t& value ) const;
        virtual int getYear( const std::time_t& value ) const;
        virtual int getMonth( const std::time_t& value ) const;
        virtual int getDayOfMonth( const std::time_t& value ) const;
        virtual int getDaysInMonth( const std::time_t& value ) const;
        virtual void setClock( Clock clock );

    private:
        Clock m_clock;
        mutable std::mutex m_mutex;
    };
    std::unique_ptr< EnvironmentTimePimpl > m_impl;

private:
    EnvironmentTime( std::unique_ptr< EnvironmentTimePimpl > customImpl );
};

class Environment {
public:
    Environment();
    virtual ~Environment();
    Environment( const Environment& cpy ) = delete;
    Environment& operator=( const Environment& cpy ) = delete;

    EnvironmentTime& getTimeEnv();
    const EnvironmentTime& getTimeEnv() const;

private:
    std::unique_ptr< EnvironmentTime > m_envTime;

private:
    friend UT::EnvironmentTimeTest;
    Environment( std::unique_ptr< EnvironmentTime > custom );
};
}  // namespace SceneEngine
