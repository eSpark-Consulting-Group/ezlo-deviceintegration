#pragma once
#include "hardware_manager/button/reset_button_base.h"
#include "hardware_manager/button/vera/vera_button_state.h"

namespace HubPlatform {
namespace HardwareManager {

class VeraResetButton : public ResetButtonBase {
public:
    VeraResetButton( OnCombinationExecuted handler );

protected:
    ButtonState& getButtonState() override {
        return m_buttonState;
    }

protected:
    VeraButtonState m_buttonState;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
