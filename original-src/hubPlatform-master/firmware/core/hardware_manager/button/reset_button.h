#pragma once

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
#include "hardware_manager/button/vera/vera_reset_button.h"
#else
#include "hardware_manager/button/generic/generic_reset_button.h"
#endif

namespace HubPlatform {
namespace HardwareManager {
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
using ResetButton = VeraResetButton;
#else
using ResetButton = GenericResetButton;
#endif
}  // namespace HardwareManager
}  // namespace HubPlatform
