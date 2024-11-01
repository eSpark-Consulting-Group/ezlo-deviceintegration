#include "hardware_manager/button/vera/vera_reset_button.h"

namespace HubPlatform {
namespace HardwareManager {

VeraResetButton::VeraResetButton( ResetButtonBase::OnCombinationExecuted handler )
: ResetButtonBase( std::move( handler ) )
, m_buttonState( "reset" ) {}

}  // namespace HardwareManager
}  // namespace HubPlatform
