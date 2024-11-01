#pragma once

#include "hardware_manager/effect/led_color_effect.h"

class FilesUploadedEffect final : public LedColorEffect {
public:
    FilesUploadedEffect( LedColor& led );
    void effect() override;
};
