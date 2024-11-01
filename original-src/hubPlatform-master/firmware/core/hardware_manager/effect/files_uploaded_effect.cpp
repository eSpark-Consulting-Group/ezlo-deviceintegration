#include "hardware_manager/effect/files_uploaded_effect.h"

FilesUploadedEffect::FilesUploadedEffect( LedColor& led )
: LedColorEffect( led ) {}

void FilesUploadedEffect::effect() {
    blinkingRgbColor( Rgb::Pink, 5, 0, 50 );
    m_led.turnOff();
}
