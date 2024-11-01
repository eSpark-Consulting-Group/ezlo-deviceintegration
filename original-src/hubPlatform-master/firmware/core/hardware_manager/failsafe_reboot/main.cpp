#include "hardware_manager/resources/hm_resources.h"

#include "system_utils/environment.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>
#include <memory>
#include <functional>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

using namespace HubPlatform::HardwareManager;

const auto logPath = HubPlatform::Environment::getLogsFolder() + "/failsafe_reboot.log";

class SystemError : public std::runtime_error {
public:
    SystemError( const std::string& description )
    : std::runtime_error( description + ": " + strerror( errno ) ) {}
};

class SpeakerButton final {
public:
    bool isPressed() {
        auto speakerButtonFd = open( Resources::SpeakerButton::speakerButtonPath, O_RDONLY );
        if( speakerButtonFd < 0 ) {
            throw SystemError( "failed to open a speaker button 'value' file" );
        }

        std::unique_ptr< int, std::function< void( int* ) > > speakerButton( &speakerButtonFd,
                                                                             []( int* fd ) { close( *fd ); } );

        char buff[ 2 ] = { '\0', '\0' };
        read( speakerButtonFd, buff, 1 );

        if( buff[ 0 ] == '0' ) {
            return true;
        }

        return false;
    }
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
    std::cout << "rebooting...\n\n";
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

    system( "reboot -f" );
    std::this_thread::sleep_for( std::chrono::seconds( 15 ) );
    system( "echo \"b\" > /proc/sysrq-trigger" );
}

void waitForRebootEvent() {
    enum class State { INIT, BUTTON_PRESSED, CANCELED };

    std::chrono::time_point< std::chrono::steady_clock > m_buttonPressedTimePoint;
    SpeakerButton speakerButton;
    auto state = State::INIT;
    bool rebootReady = false;

    std::cout << "start waiting for user actions...\n";

    while( true ) {
        if( speakerButton.isPressed() ) {
            if( state == State::INIT ) {
                std::cout << "button is pressed\n";

                state = State::BUTTON_PRESSED;
                m_buttonPressedTimePoint = std::chrono::steady_clock::now();
            }
            if( state == State::BUTTON_PRESSED ) {
                const auto buttonHoldTime = std::chrono::steady_clock::now() - m_buttonPressedTimePoint;
                if( buttonHoldTime >= Resources::SpeakerButton::Reboot::minThreshold ) {
                    if( !rebootReady ) {
                        std::cout << "ready to reboot" << std::endl;
                        system( "/opt/firmware/bin/ha-ledcmd --start hubReboot" );
                        rebootReady = true;
                    }
                }
                if( buttonHoldTime >= Resources::SpeakerButton::Reboot::maxThreshold ) {
                    if( rebootReady ) {
                        std::cout << "reboot canceled" << std::endl;
                        system( "/opt/firmware/bin/ha-ledcmd --stop hubReboot" );
                        rebootReady = false;
                        state = State::CANCELED;
                    }
                }
            }
        }
        else {
            if( state == State::BUTTON_PRESSED ) {
                const auto buttonHoldTime = std::chrono::steady_clock::now() - m_buttonPressedTimePoint;

                std::cout << "button is released\n";

                if( buttonHoldTime >= Resources::SpeakerButton::Reboot::minThreshold &&
                    buttonHoldTime <= Resources::SpeakerButton::Reboot::maxThreshold ) {
                    std::cout << "user initiated reboot\n";
                    break;
                }
                if( rebootReady ) {
                    std::cout << "reboot canceled" << std::endl;
                    system( "/opt/firmware/bin/ha-ledcmd --stop hubReboot" );
                    rebootReady = false;
                }
            }
            state = State::INIT;
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
}

/* the sole purpose of this binary/service is to provide a failsafe reboot
 * (i.e. regardless of a state of a hubPlatfrom firmware)
 * keeping this binary as simple as possible without depending on huge hubPlatfrom libraries leads
 * to code duplication, as logic for handling a speaker button is(will be) a part of hm_manager, but makes
 * this binary/service more reliable
 * this binary/service is restarted automatically by openwrt (procd) in a case of any fail, if whole userspace
 * gets stuck, the whole system gets rebooted by a watchdog */
int main( int argc, char* argv[] ) {
    try {
        initLogSystem();

        waitForRebootEvent();
        reboot();
    }
    catch( const std::exception& ex ) {
        std::cout << ex.what() << std::endl;
    }
    catch( ... ) {
        std::cout << "unknown exception\n";
    }
}
