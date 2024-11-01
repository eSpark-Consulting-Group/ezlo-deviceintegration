#pragma once
#include "hardware_manager/button/button_state.h"

#include <chrono>
#include <functional>

extern const std::chrono::milliseconds ButtonCheckInterval;

class ResetButtonBase {
public:
    enum class PressingPicture : int {
        SKIP = 0,
        FIND_HUB,             // press 1 time for 0.05 - 0.5 seconds within 1.5 second
        SAVE_LOGS,            // press 1 time for 0.05 - 0.5 seconds within 1.5 second
        WIFI_RESET,           // press 2 times for 0.05 - 0.5 seconds within 1.5 second
        POWEROFF,             // press 3 times for 0.05 - 0.5 seconds within 1.5 second
        REBOOT_READY,         // hold more than 2 seconds
        REBOOT,               // release after holding for 2 - 5 seconds
        SOFT_RESET_READY,     // hold more than 5 seconds
        SOFT_RESET,           // release after holding for 5 - 10 seconds
        FACTORY_RESET_READY,  // hold more than 10 seconds
        FACTORY_RESET,        // release after holding for 10 - 20 seconds
        CANCEL                // release after holding for more than 20 seconds
    };

    using OnCombinationExecuted = std::function< void( PressingPicture ) >;
    ResetButtonBase( OnCombinationExecuted handler );
    virtual ~ResetButtonBase() = default;

    virtual void processButton();

protected:
    virtual ButtonState& getButtonState() = 0;

protected:
    OnCombinationExecuted m_handler;

    std::chrono::milliseconds m_shortPressesPeriod;
    int m_shortPressCounter;
};
