#include "system_utils/environment.h"

#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <exception>
#include <array>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timerfd.h>

const std::string logPath = HubPlatform::Environment::getLogsFolder() + "/poweroff.log";

class SystemError : public std::runtime_error {
public:
    SystemError( const std::string& description )
    : std::runtime_error( description + ": " + strerror( errno ) ) {}
};

class EventsHandler final {
public:
    enum class WakeUpEvent { None, BUTTON, POWER_SUPPLY };

    EventsHandler() {
        memset( &m_events[ 0 ], 0, sizeof( m_events ) );

        m_events[ 0 ].fd = open( m_wakeUpButton.c_str(), O_RDONLY );
        m_events[ 0 ].events = POLLPRI | POLLERR;

        m_events[ 1 ].fd = timerfd_create( CLOCK_MONOTONIC, 0 );
        m_events[ 1 ].events = POLLIN;

        armTimer( m_batteryChargingStatusPollPeriod );
        system( ( "echo both > " + m_wakeUpButtonEdge ).c_str() );
    }

    WakeUpEvent handle() {
        m_wakeUpEvent = WakeUpEvent::None;

        poll( &m_events[ 0 ], m_events.size(), -1 );
        if( m_events[ 1 ].revents == POLLIN ) {
            m_events[ 1 ].revents = 0;
            timerAction();
        }
        else if( m_events[ 0 ].revents == POLLPRI | POLLERR ) {
            m_events[ 0 ].revents = 0;
            buttonAction();
        }

        return m_wakeUpEvent;
    }

private:
    void buttonAction() {
        static bool first = true;
        static bool pressed = false;
        char buff[ 2 ] = { '\0', '\0' };

        // consume a gpio fd event
        lseek( m_events[ 0 ].fd, 0, SEEK_SET );
        read( m_events[ 0 ].fd, buff, 1 );

        // skip a first read from a button fd
        if( first ) {
            first = false;
            return;
        }

        if( !pressed ) {
            m_buttonPressedTimePoint = std::chrono::steady_clock::now();

            std::cout << "button has been pressed\n";
            pressed = true;
        }
        else {
            if( std::chrono::steady_clock::now() - m_buttonPressedTimePoint >= m_wakeUpButtonHoldTime ) {
                m_wakeUpEvent = WakeUpEvent::BUTTON;
            }

            std::cout << "button has been released\n";
            pressed = false;
        }
    }

    void timerAction() {
        // consume a timer fd event
        char timerBuffer[ 8 ];
        read( m_events[ 1 ].fd, timerBuffer, sizeof( timerBuffer ) );

        if( isPowerSupplyProvided() ) {
            m_wakeUpEvent = WakeUpEvent::POWER_SUPPLY;
        }

        std::cout << "timer expired\n";
    }

    void armTimer( int period ) {
        itimerspec timePoint;

        memset( &timePoint, 0, sizeof( timePoint ) );
        timePoint.it_value.tv_sec = period;
        timePoint.it_interval.tv_sec = period;

        timerfd_settime( m_events[ 1 ].fd, 0, &timePoint, nullptr );
    }

    bool isPowerSupplyProvided() {
        auto chargerStatusPinFd = open( m_chargerStatusPin.c_str(), O_RDONLY );

        auto chargerEnablePinFd = open( m_chargerEnablePin.c_str(), O_WRONLY );
        write( chargerEnablePinFd, "0", 1 );

        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

        char buff[ 2 ] = { '\0', '\0' };
        read( chargerStatusPinFd, buff, 1 );

        write( chargerEnablePinFd, "1", 1 );

        // if a battery charger reports that charging is in progress
        /* TODO: when battery is fully charged this approach doesn't work...
         *       check battery fuel gauge for providing such functionality for a hardware implementation of power off */
        if( buff[ 0 ] == '0' ) {
            std::cout << "power supply is provided\n";
            return true;
        }

        return false;
    }

private:
    const std::string m_wakeUpButton = "/sys/class/gpio/gpio356/value";
    const std::string m_wakeUpButtonEdge = "/sys/class/gpio/gpio356/edge";
    const std::string m_chargerEnablePin = "/sys/class/gpio/gpio352/value";
    const std::string m_chargerStatusPin = "/sys/class/gpio/gpio363/value";

    std::chrono::time_point< std::chrono::steady_clock > m_buttonPressedTimePoint;
    const std::chrono::seconds m_wakeUpButtonHoldTime = std::chrono::seconds( 3 );  // in seconds
    const int m_batteryChargingStatusPollPeriod = 5;                                // in seconds

    WakeUpEvent m_wakeUpEvent = WakeUpEvent::None;
    std::array< pollfd, 2 > m_events;
};

void initLogSystem() {
    if( !freopen( logPath.c_str(), "w", stdout ) ) {
        throw SystemError( "failed to redirect stdout" );
    }
    if( !freopen( logPath.c_str(), "w", stderr ) ) {
        throw SystemError( "failed to redirect stderr" );
    }
}

void reboot() {
    std::cout << "reboot...\n\n";
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

    system( "reboot -f" );
    std::this_thread::sleep_for( std::chrono::seconds( 15 ) );
    system( "echo \"b\" > /proc/sysrq-trigger" );
}

void waitForWakeUpEvent() {
    EventsHandler eventsHandler;

    std::cout << "start waiting for wakeup events....\n";

    while( true ) {
        auto event = eventsHandler.handle();

        if( event == EventsHandler::WakeUpEvent::BUTTON ) {
            std::cout << "wakeup event: user has pressed and hold a recovery/reset button\n";
            break;
        }
        else if( event == EventsHandler::WakeUpEvent::POWER_SUPPLY ) {
            std::cout << "wakeup event: user has plugged a power supply\n";
            break;
        }
    }
}

/* it's a software/fake poweroff implementation:
 *  -turn off ha-firmware
 *  -turn off external peripherals
 *  -turn off 1-3 cpu cores
 *  -turn on a 'powersafe' cpufreq governor (which forces the last cpu0 to operate on a lowest frequency)
 *  -wait for wakeup events:
 *    .a recovery/reset button has been pressed on hold more than 3 seconds
 *    .a power supply has been, provided (plugged)
 *
 *    (even given the fact that we leveraged actual waiting to a hardware and thus left cpu almost unloaded
 *    it won't help us a lot as cpu should execute instructions anyway even if there's nothing to do :),
 *    we just hope that in that case cpu/kernel may perform some power related optimization, but anyway this
 *    implementation doesn't solve delivery issues and not to mention a battery degradation issue,
 *    we just make everything possible as a part of an implementation of a poweroff functionality by this short time...)
 *  -reboot the hub (the easiest way to restore the system)
 * it's going to be changed to a standby poweroff, which is also a fake poweroff - we luck of hardware which
 * may cut the power off for the cpu thus preventing us from implementing a real poweroff,
 * but this standby poweroff can provide much less power consumption compared to this software poweroff */
int main( int argc, char* argv[] ) {
    try {
        if( daemon( 0, 0 ) != 0 ) {
            throw SystemError( "failed to run a process in background" );
        }

        initLogSystem();

        system( ( "/opt/firmware/bin/poweroff.sh 2& > " + logPath ).c_str() );
    }
    catch( const std::exception& ex ) {
        std::cout << ex.what() << std::endl;
        reboot();
    }
    catch( ... ) {
        std::cout << "unknown exception\n";
        reboot();
    }

    waitForWakeUpEvent();

    // turn on battery charger
    system( "echo 0 > /sys/class/gpio/gpio352/value" );
    reboot();
}
