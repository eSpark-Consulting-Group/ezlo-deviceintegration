#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class DeviceAddingEffect final : public LedColorEffect {
public:
    explicit DeviceAddingEffect( LedColor& led );
    void effect() override;
};
