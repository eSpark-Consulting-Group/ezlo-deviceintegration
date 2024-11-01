#pragma once
#include "hardware_manager/led/led.h"
#include "hardware_manager/effect/effect.h"

class LedEffect : public Effect {
public:
    LedEffect( Led& led );
    virtual ~LedEffect();

protected:
    Led& m_led;
};
