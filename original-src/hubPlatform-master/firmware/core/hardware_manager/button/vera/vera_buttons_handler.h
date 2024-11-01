#pragma once
#include <functional>
#include <map>
#include <string>

#include "hardware_manager/button/button_state.h"

namespace HubPlatform {
namespace HardwareManager {

class VeraButtonsHandler {
public:
    class Exception : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    using OnButtonStateChanged = std::function< void( ButtonState::StateValue state, unsigned int secondsPressed ) >;

    static VeraButtonsHandler& instance();
    VeraButtonsHandler( const VeraButtonsHandler& ) = delete;
    ~VeraButtonsHandler();

    void listen();

    void processButtons();
    void addButtonStateHandler( const std::string& buttonName, OnButtonStateChanged buttonStateHandler );
    void removeButtonStateHandler( const std::string& buttonName );

private:
    VeraButtonsHandler() = default;
    void parseNetlinkEvent( const std::string& data );

private:
    int m_socketDesc = -1;

    std::map< std::string /*button name*/, OnButtonStateChanged > m_buttonHandlers;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
