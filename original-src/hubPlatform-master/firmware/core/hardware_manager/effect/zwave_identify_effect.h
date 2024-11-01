#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class ZwaveIdentifyEffect final : public LedColorEffect {
public:
    ZwaveIdentifyEffect( LedColor& led, Led& zwaveLed );
    void effect() override;

private:
    Led& m_zwaveLed;
};
