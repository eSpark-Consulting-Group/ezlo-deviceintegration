#include "hardware_manager/button/generic/generic_reset_button.h"

namespace HubPlatform {
namespace HardwareManager {

GenericResetButton::GenericResetButton( ResetButtonBase::OnCombinationExecuted handler, int gpioNumber )
: ResetButtonBase( std::move( handler ) )
, m_buttonState( gpioNumber ) {}

}  // namespace HardwareManager
}  // namespace HubPlatform
