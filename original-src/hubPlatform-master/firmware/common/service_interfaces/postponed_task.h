#pragma once

#include <chrono>
#include <functional>
#include <memory>

namespace HubPlatform {
namespace Services {

class PostponedTask;
using PostponedTaskPtr = std::shared_ptr< PostponedTask >;
using PostponedTaskFunction = std::function< void( PostponedTask& ) >;

class PostponedTask {
public:
    using Interval = std::chrono::steady_clock::duration;
    enum class StoppingMode : std::uint8_t {
        NoWaiting,
        WaitingIfPossible,            // No waiting if stop was called from callback (To avoid dead lock)
        OnTerminationSignalReceived,  // No waiting and no mutex locking in this mode.
                                      // Use it only from signal handlers.
        Auto = WaitingIfPossible
    };

    // clang-format off
    enum class PeriodicMode : std::uint8_t {
        FixedPeriod,  // Example:   _     __    _     _
                      //          _| |___|  |__| |___|
                      // a=b=c     .  a  .  b  .  c  .

        FixedIdleDuration,  // Example:  _       __       ___       _
                            //         _| |_____|  |_____|   |_____|
                            // a=b=c      .  a  .  .  b  .   .  c  .

        Default = FixedPeriod
    };
    // clang-format on

    virtual ~PostponedTask() = default;

    virtual void start( const Interval& first, const Interval& periodic ) = 0;
    virtual void start( const Interval& interval, bool isPeriodic ) = 0;
    virtual void stop( StoppingMode mode = StoppingMode::Auto ) = 0;
    virtual bool isActive() const = 0;
    virtual bool isPeriodic() const = 0;
    virtual Interval getInterval() const = 0;
    virtual void setCallback( PostponedTaskFunction cb ) = 0;
    virtual void setPeriodicMode( PeriodicMode periodicMode ) = 0;
    virtual Interval getTimeToExecution() = 0;
};

}  // namespace Services
}  // namespace HubPlatform
