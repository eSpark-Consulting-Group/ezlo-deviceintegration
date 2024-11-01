#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class HubNotConnectedEffect final : public LedColorEffect {
public:
    HubNotConnectedEffect( LedColor& led );
    void effect() override;
};
