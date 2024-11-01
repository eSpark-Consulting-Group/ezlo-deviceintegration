#include "hardware_manager/config/hw_manager_config.h"
#include "hardware_manager/led/led_sercomm_gpio.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace HardwareManager {

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
constexpr uint HWManagerConfig::LedPowerNumber;
constexpr uint HWManagerConfig::LedEthernetNumber;
constexpr uint HWManagerConfig::LedZwaveNumber;
constexpr uint HWManagerConfig::LedServiceNumber;
#endif

HWManagerConfig::HWManagerConfig()
#ifdef __HUB_TYPE_A20__
: m_ledColor( std::make_unique< LedColorPwmA20 >( "emotionLed" ) )
, m_powerLed( std::make_unique< NoLed >( "powerLed" ) )
, m_internetLed( std::make_unique< NoLed >( "internetLed" ) )
, m_zwaveLed( std::make_unique< NoLed >( "zwaveLed" ) )
, m_serviceLed( std::make_unique< NoLed >( "serviceLed" ) )
, m_wifiLed( std::make_unique< LedSysfs >( "wifiLed", "/sys/class/leds/wifi_blue_ph09" ) )
, m_readyLed( std::make_unique< LedSysfs >( "readyLed", "/sys/class/leds/ready_yellow_ph10" ) )
, m_errorLed( std::make_unique< LedSysfs >( "errorLed", "/sys/class/leds/error_red_ph06" ) )
, m_messageLed( std::make_unique< LedSysfs >( "messageLed", "/sys/class/leds/zlink_blue_ph08" ) )
, m_excludeLed( std::make_unique< LedSysfs >( "excludeLed", "/sys/class/leds/zexclude_red_ph07" ) )
, m_includeLed( std::make_unique< LedSysfs >( "includeLed", "/sys/class/leds/zinclude_green_ph19" ) )
, m_zigbeeLed( std::make_unique< NoLed >( "zigbeeLed" ) )
, m_bluetoothLed( std::make_unique< NoLed >( "bluetoothLed" ) )
, m_lpfrLed( std::make_unique< NoLed >( "lpfrLed" ) )
, m_3gLed( std::make_unique< NoLed >( "3gLed" ) )
, m_voiceLed( std::make_unique< NoLed >( "voiceLed" ) )
, m_resetButtonNumber( 0 )

#elif( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) )
: m_ledColor( std::make_unique< LedColorPwm >( "emotionLed", "/dev/i2c-0", 0x35, 0x34, 0x36 ) )
, m_powerLed( std::make_unique< NoLed >( "powerLed" ) )
, m_internetLed( std::make_unique< NoLed >( "internetLed" ) )
, m_zwaveLed( std::make_unique< NoLed >( "zwaveLed" ) )
, m_serviceLed( std::make_unique< NoLed >( "serviceLed" ) )
, m_wifiLed( std::make_unique< NoLed >( "wifiLed" ) )
, m_readyLed( std::make_unique< NoLed >( "readyLed" ) )
, m_errorLed( std::make_unique< NoLed >( "errorLed" ) )
, m_messageLed( std::make_unique< NoLed >( "messageLed" ) )
, m_excludeLed( std::make_unique< NoLed >( "excludeLed" ) )
, m_includeLed( std::make_unique< NoLed >( "includeLed" ) )
, m_zigbeeLed( std::make_unique< NoLed >( "zigbeeLed" ) )
, m_bluetoothLed( std::make_unique< NoLed >( "bluetoothLed" ) )
, m_lpfrLed( std::make_unique< NoLed >( "lpfrLed" ) )
, m_3gLed( std::make_unique< NoLed >( "3gLed" ) )
, m_voiceLed( std::make_unique< NoLed >( "voiceLed" ) )
, m_resetButtonNumber( 356 )

#elif( defined( __HUB_TYPE_BEEWAVE__ ) )
: m_ledColor( std::make_unique< LedColorPwm >( "emotionLed", "/dev/i2c-0", 0x36, 0x35, 0x34 ) )
, m_powerLed( std::make_unique< NoLed >( "powerLed" ) )
, m_internetLed( std::make_unique< NoLed >( "internetLed" ) )
, m_zwaveLed( std::make_unique< NoLed >( "zwaveLed" ) )
, m_serviceLed( std::make_unique< NoLed >( "serviceLed" ) )
, m_wifiLed( std::make_unique< NoLed >( "wifiLed" ) )
, m_readyLed( std::make_unique< NoLed >( "readyLed" ) )
, m_errorLed( std::make_unique< NoLed >( "errorLed" ) )
, m_messageLed( std::make_unique< NoLed >( "messageLed" ) )
, m_excludeLed( std::make_unique< NoLed >( "excludeLed" ) )
, m_includeLed( std::make_unique< NoLed >( "includeLed" ) )
, m_zigbeeLed( std::make_unique< NoLed >( "zigbeeLed" ) )
, m_bluetoothLed( std::make_unique< NoLed >( "bluetoothLed" ) )
, m_lpfrLed( std::make_unique< NoLed >( "lpfrLed" ) )
, m_3gLed( std::make_unique< NoLed >( "3gLed" ) )
, m_voiceLed( std::make_unique< NoLed >( "voiceLed" ) )
, m_resetButtonNumber( 14 )

#elif __HUB_TYPE_G150__
: m_ledColor( std::make_unique< NoLed >( "emotionLed" ) )
, m_powerLed( std::make_unique< LedSysfs >( "powerLed", LedPowerNumber ) )
, m_internetLed( std::make_unique< LedSysfs >( "internetLed", LedEthernetNumber ) )
, m_zwaveLed( std::make_unique< LedSysfs >( "zwaveLed", LedZwaveNumber ) )
, m_serviceLed( std::make_unique< NoLed >( "serviceLed" ) )
, m_wifiLed( std::make_unique< NoLed >( "wifiLed" ) )
, m_readyLed( std::make_unique< LedSysfs >( "readyLed", LedServiceNumber ) )
, m_errorLed( std::make_unique< NoLed >( "errorLed" ) )
, m_messageLed( std::make_unique< NoLed >( "messageLed" ) )
, m_excludeLed( std::make_unique< NoLed >( "excludeLed" ) )
, m_includeLed( std::make_unique< NoLed >( "includeLed" ) )
, m_zigbeeLed( std::make_unique< NoLed >( "zigbeeLed" ) )
, m_bluetoothLed( std::make_unique< NoLed >( "bluetoothLed" ) )
, m_lpfrLed( std::make_unique< NoLed >( "lpfrLed" ) )
, m_3gLed( std::make_unique< NoLed >( "3gLed" ) )
, m_voiceLed( std::make_unique< NoLed >( "voiceLed" ) )
, m_resetButtonNumber( 0 )

#elif __HUB_TYPE_G450__
: m_ledColor( std::make_unique< NoLed >( "emotionLed" ) )
, m_powerLed( std::make_unique< LedSercommGPIO >( "powerLed", 1 )
, m_internetLed( std::make_unique< LedSercommGPIO >( "internetLed", 2 ) )
, m_zwaveLed( std::make_unique< LedSercommGPIO >( "zwaveLed", 4 ) )
, m_serviceLed( std::make_unique< LedSercommGPIO >( "serviceLed", 5 ) )
, m_wifiLed( std::make_unique< LedSercommGPIO >( "wifiLed", 3 ) )
, m_readyLed( std::make_unique< NoLed >( "readyLed" ) )
, m_errorLed( std::make_unique< NoLed >( "errorLed" ) )
, m_messageLed( std::make_unique< NoLed >( "messageLed" ) )
, m_excludeLed( std::make_unique< NoLed >( "excludeLed" ) )
, m_includeLed( std::make_unique< NoLed >( "includeLed" ) )
, m_zigbeeLed( std::make_unique< LedSercommGPIO >( "zigbeeLed", 6 ) )
, m_bluetoothLed( std::make_unique< LedSercommGPIO >( "bluetoothLed", 7 ) )
, m_lpfrLed( std::make_unique< NoLed >( "lpfrLed" ) )
, m_3gLed( std::make_unique< NoLed >( "3gLed" ) )
, m_voiceLed( std::make_unique< NoLed >( "voiceLed" )
, m_resetButtonNumber( 0 )

#elif __HUB_TYPE_G550__
: m_ledColor( std::make_unique< NoLed >( "emotionLed" ) )
, m_powerLed( std::make_unique< LedSercommGPIO >( "powerLed", 1 )
, m_internetLed( std::make_unique< LedSercommGPIO >( "internetLed", 2 ) )
, m_zwaveLed( std::make_unique< LedSercommGPIO >( "zwaveLed", 3 ) )
, m_serviceLed( std::make_unique< LedSercommGPIO >( "serviceLed", 7 ) )
, m_wifiLed( std::make_unique< LedSercommGPIO >( "wifiLed", 2 ) )
, m_readyLed( std::make_unique< NoLed >( "readyLed" ) )
, m_errorLed( std::make_unique< NoLed >( "errorLed" ) )
, m_messageLed( std::make_unique< NoLed >( "messageLed" ) )
, m_excludeLed( std::make_unique< NoLed >( "excludeLed" ) )
, m_includeLed( std::make_unique< NoLed >( "includeLed" ) )
, m_zigbeeLed( std::make_unique< LedSercommGPIO >( "zigbeeLed", 4 ) )
, m_bluetoothLed( std::make_unique< LedSercommGPIO >( "bluetoothLed", 5 ) )
, m_lpfrLed( std::make_unique< LedSercommGPIO >( "lpfrLed", 6 ) )
, m_3gLed( std::make_unique< LedSercommGPIO >( "3gLed", 8 ) )
, m_voiceLed( std::make_unique< LedSercommGPIO >( "voiceLed", 9 ) )
, m_resetButtonNumber( 0 )

#else
: m_ledColor( std::make_unique< NoLed >( "emotionLed" ) )
, m_powerLed( std::make_unique< NoLed >( "powerLed" ) )
, m_internetLed( std::make_unique< NoLed >( "internetLed" ) )
, m_zwaveLed( std::make_unique< NoLed >( "zwaveLed" ) )
, m_serviceLed( std::make_unique< NoLed >( "serviceLed" ) )
, m_wifiLed( std::make_unique< NoLed >( "wifiLed" ) )
, m_readyLed( std::make_unique< NoLed >( "readyLed" ) )
, m_errorLed( std::make_unique< NoLed >( "errorLed" ) )
, m_messageLed( std::make_unique< NoLed >( "messageLed" ) )
, m_excludeLed( std::make_unique< NoLed >( "excludeLed" ) )
, m_includeLed( std::make_unique< NoLed >( "includeLed" ) )
, m_zigbeeLed( std::make_unique< NoLed >( "zigbeeLed" ) )
, m_bluetoothLed( std::make_unique< NoLed >( "bluetoothLed" ) )
, m_lpfrLed( std::make_unique< NoLed >( "lpfrLed" ) )
, m_3gLed( std::make_unique< NoLed >( "3gLed" ) )
, m_voiceLed( std::make_unique< NoLed >( "voiceLed" ) )
, m_resetButtonNumber( 0 )

#endif
{
}

LedColorPwm& HWManagerConfig::getEmotionLed() {
    return *m_ledColor;
}

Led& HWManagerConfig::getWifiLed() {
    return *m_wifiLed;
}

Led& HWManagerConfig::getReadyLed() {
    return *m_readyLed;
}

Led& HWManagerConfig::getErrorRed() {
    return *m_errorLed;
}

Led& HWManagerConfig::getMessageLed() {
    return *m_messageLed;
}

Led& HWManagerConfig::getExcludeLed() {
    return *m_excludeLed;
}

Led& HWManagerConfig::getIncludeLed() {
    return *m_includeLed;
}

Led& HWManagerConfig::getInternetLed() {
    return *m_internetLed;
}

Led& HWManagerConfig::getPowerLed() {
    return *m_powerLed;
}

Led& HWManagerConfig::getZwaveLed() {
    return *m_zwaveLed;
}

Led& HWManagerConfig::getServiceLed() {
    return *m_serviceLed;
}

Led& HWManagerConfig::getZigbeeLed() {
    return *m_zigbeeLed;
}

Led& HWManagerConfig::getBluetoothLed() {
    return *m_bluetoothLed;
}

Led& HWManagerConfig::getLpfrLed() {
    return *m_lpfrLed;
}

Led& HWManagerConfig::get3gLed() {
    return *m_3gLed;
}

Led& HWManagerConfig::getVoiceLed() {
    return *m_voiceLed;
}

int HWManagerConfig::getResetButtonNumber() {
    return m_resetButtonNumber;
}

}  // namespace HardwareManager
}  // namespace HubPlatform
