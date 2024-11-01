#include "common_utils/cpp14.h"
#include "bus/messagesclasses.h"
#include "service_interfaces/groups_names.h"
#include "bus/spread_bus.h"
#include "hardware_manager/config/hw_manager_config.h"
#include "hardware_manager/effect/update_ezlofw_effect.h"
#include "hardware_manager/effect/hub_reboot_effect.h"
#include "hardware_manager/effect/firmware_down_preset.h"
#include "hardware_manager/hub_state/model/hub_state.h"
#include <set>

using namespace std;
using namespace HubPlatform;
using namespace HubPlatform::HardwareManager;

namespace {

const char* const FIRMWARE_UPDATE_PROCESS = "firmwareUpdate";
const char* const HUB_REBOOT_PROCESS = "hubReboot";
const char* const CUSTOM_PROCESS = "custom";
const std::set< std::string > colors = { "red", "green", "blue", "yellow", "orange", "cyan", "magenta", "white" };
const std::set< std::string > types = { "on", "blink", "breathe" };

void help() {
    const char* const progName = "ha-ledcmd";
    const char* const sp4 = "    ";
    cerr << "Usage:" << endl;
    cerr << "Each Led indication process can start using `--start` parameter, and then should be stopped by --stop "
            "command"
         << endl;
    cerr << sp4 << progName << " --start [process]" << endl;
    cerr << sp4 << progName << " --start custom [color] [led type]" << endl;
    cerr << sp4 << progName << " --stop [process]" << endl;
    cerr << "Options:" << endl;
    cerr << sp4 << "Processes: " << FIRMWARE_UPDATE_PROCESS << ", " << HUB_REBOOT_PROCESS << ", " << CUSTOM_PROCESS
         << endl;

    auto printSetParams = [sp4]( const std::string name, std::set< std::string > parameterSet ) {
        cerr << sp4 << name << ": ";
        for( auto parameter : parameterSet )
            cerr << parameter << " ";
        cerr << endl;
    };

    cerr << "Custom process supported parameters:" << std::endl;
    printSetParams( "colors", colors );
    printSetParams( "types", types );

    cerr << "Example:" << endl;
    cerr << sp4 << progName << " --start custom red blink" << endl;
    cerr << sp4 << progName << " --stop custom" << endl;
}

class Environment {
public:
    Environment()
    : m_bus( nullptr, []( BaseBus* spread ) {} ) {}

    LedColorPwm& getEmotionLed() {
        return getHardwareConfig().getEmotionLed();
    }

    bool onProcessRunning( HubState::Process process ) {
        auto busPtr = getBus();
        if( !busPtr ) {
            return false;
        }
        busPtr->send( GroupsNames::HARDWARE_MANAGER,
                      MessageClasses::JSON_RPC,
                      HUB_PROCESS_RUNNING,
                      JSON( "type" << HubState::toString( process ) ) );
        return true;
    }

    bool onProcessStopped( HubState::Process process,
                           HubState::ProcessResult result = HubState::ProcessResult::Unspecified ) {
        auto busPtr = getBus();
        if( !busPtr ) {
            return false;
        }
        busPtr->send( GroupsNames::HARDWARE_MANAGER,
                      MessageClasses::JSON_RPC,
                      HUB_PROCESS_STOPPED,
                      JSON( "type" << HubState::toString( process ) << "result" << HubState::toString( result ) ) );
        return true;
    }

    bool onCustomProcess( const std::string& color, const std::string& type ) {
        auto busPtr = getBus();
        if( !busPtr ) {
            return false;
        }

        busPtr->send( GroupsNames::HARDWARE_MANAGER,
                      MessageClasses::JSON_RPC,
                      HUB_CUSTOM_LED,
                      JSON( "color" << (int) Effect::colorFromString( color ) << "blinkingType"
                                    << (int) Effect::blinkingTypeFromString( type ) ) );

        return true;
    }

private:
    HWManagerConfig& getHardwareConfig() {
        if( !m_config ) {
            m_config = std::make_unique< HWManagerConfig >();
        }
        return *m_config;
    }

    BaseBus* getBus() {
        if( system( "pidof ha-hardware_manager > /dev/null 2>&1" ) != 0 ) {
            return nullptr;
        }

        if( !m_bus ) {
            try {
                m_bus = createBus( "led_command", []( const bus_rx_data_t* /*rx_data*/ ) {}, true, 1 );
                m_bus->start();
            }
            catch( const std::runtime_error& error ) {
            }
        }
        return m_bus.get();
    }

private:
    std::unique_ptr< HWManagerConfig > m_config;
    BaseBusPtr m_bus;
};

}  // namespace

int main( int argc, char* argv[] ) {
    if( argc < 2 ) {
        cerr << "Wrong params count: " << argc - 1 << " required at least 1" << endl;
        return 1;
    }

    using namespace HubPlatform::HardwareManager;
    Environment env;

    if( string( argv[ 1 ] ) == "--help" ) {
        help();
        return 1;
    }
    else if( string( argv[ 1 ] ) == "--start" ) {
        if( argc < 3 ) {
            cerr << "Wrong params count: " << argc - 1 << " required at least 2 for start process" << endl;
            return 1;
        }
        std::string preset{ argv[ 2 ] };
        if( preset == FIRMWARE_UPDATE_PROCESS ) {
            if( !env.onProcessRunning( HubState::Process::FirmwareUpdate ) ) {
                updateEzloFWEffectPreset( env.getEmotionLed() );
            }
        }
        else if( preset == HUB_REBOOT_PROCESS ) {
            if( !env.onProcessRunning( HubState::Process::Reboot ) ) {
                hubRebootEffectPreset( env.getEmotionLed() );
            }
        }
        else if( preset == CUSTOM_PROCESS ) {
            if( argc < 5 ) {
                cerr << "Wrong params count: " << argc << "." << endl;
                return 1;
            }

            std::string color{ argv[ 3 ] };
            std::string type{ argv[ 4 ] };

            if( !colors.count( color ) ) {
                cout << "Color name: " << color << " incorrect or not supported" << endl;
                return 1;
            }

            if( !types.count( type ) ) {
                cout << "Led type: " << type << " incorrect or not supported" << endl;
                return 1;
            }

            env.onCustomProcess( color, type );
        }
        else {
            cout << "Unknown preset: " << preset << endl;
            return 1;
        }
        return 0;
    }
    else if( string( argv[ 1 ] ) == "--stop" ) {
        if( argc < 3 ) {
            cerr << "Wrong params count: " << argc - 1 << " required at least 2 for stop process" << endl;
            return 1;
        }

        std::string preset{ argv[ 2 ] };
        if( preset == FIRMWARE_UPDATE_PROCESS ) {
            if( !env.onProcessStopped( HubState::Process::FirmwareUpdate ) ) {
                firmwareDownPreset( env.getEmotionLed() );
            }
        }
        else if( preset == HUB_REBOOT_PROCESS ) {
            if( !env.onProcessStopped( HubState::Process::Reboot ) ) {
                firmwareDownPreset( env.getEmotionLed() );
            }
        }
        else if( preset == CUSTOM_PROCESS ) {
            env.onProcessStopped( HubState::Process::Custom );
        }
        else {
            cerr << "Unknown preset: " << preset << endl;
            help();
            return 1;
        }
        return 0;
    }

    return 0;
}
