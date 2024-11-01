#pragma once

#include "hardware_manager/led/led.h"
#include <string>

class LedSysfs final : public Led {
public:
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    LedSysfs( std::string name, uint ledNumber );
#else
    LedSysfs( std::string name, std::string sysfsHandler );
#endif
    bool turnOn() override;
    bool turnOff() override;
    bool turn( bool enable ) override;
    bool turnBlinking( int msOff, int msOn ) override;

private:
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    uint m_ledNumber;
#else
    std::string m_sysfsHandler;
#endif
};
