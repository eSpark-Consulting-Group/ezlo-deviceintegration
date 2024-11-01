#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class DeviceRemovingEffect final : public LedColorEffect {
public:
    explicit DeviceRemovingEffect( LedColor& led );
    void effect() override;
};
