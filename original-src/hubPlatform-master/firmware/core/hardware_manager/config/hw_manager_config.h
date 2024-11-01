#pragma once
#include "hardware_manager/led/led_color.h"
#include "hardware_manager/led/led_color_pwm_a20.h"
#include "hardware_manager/led/led_color_pwm.h"
#include "hardware_manager/led/led_sysfs.h"
#include <string>

namespace HubPlatform {
namespace HardwareManager {

class NoLed : public LedColorPwm {
public:
    NoLed( const std::string& name )
    : LedColorPwm( name ) {}
    bool turnOn() override {
        return true;
    }
    bool turnOff() override {
        return true;
    }
    bool turn( bool ) override {
        return true;
    }
    bool turnBlinking( int msOff, int msOn ) override {
        return true;
    }

    void setColor( Rgb color ) override {
        m_color = color;
    }
};

class HWManagerConfig final {
public:
    HWManagerConfig();
    LedColorPwm& getEmotionLed();
    Led& getWifiLed();
    Led& getReadyLed();
    Led& getErrorRed();
    Led& getMessageLed();
    Led& getExcludeLed();
    Led& getIncludeLed();
    Led& getInternetLed();
    Led& getPowerLed();
    Led& getZwaveLed();
    Led& getServiceLed();
    Led& getZigbeeLed();
    Led& getBluetoothLed();
    Led& getLpfrLed();
    Led& get3gLed();
    Led& getVoiceLed();

    int getResetButtonNumber();

public:
#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
    static constexpr uint LedPowerNumber = 1;
    static constexpr uint LedEthernetNumber = 2;
    static constexpr uint LedZwaveNumber = 3;
    static constexpr uint LedServiceNumber = 4;
#endif

private:
    std::unique_ptr< LedColorPwm > m_ledColor;
    std::unique_ptr< Led > m_powerLed;
    std::unique_ptr< Led > m_internetLed;
    std::unique_ptr< Led > m_zwaveLed;
    std::unique_ptr< Led > m_serviceLed;
    std::unique_ptr< Led > m_wifiLed;
    std::unique_ptr< Led > m_readyLed;
    std::unique_ptr< Led > m_errorLed;
    std::unique_ptr< Led > m_messageLed;
    std::unique_ptr< Led > m_excludeLed;
    std::unique_ptr< Led > m_includeLed;
    std::unique_ptr< Led > m_zigbeeLed;
    std::unique_ptr< Led > m_bluetoothLed;
    std::unique_ptr< Led > m_lpfrLed;
    std::unique_ptr< Led > m_3gLed;
    std::unique_ptr< Led > m_voiceLed;
    int m_resetButtonNumber;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
