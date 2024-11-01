#include "hardware_manager/battery/battery_charger.h"

namespace HubPlatform {
namespace HardwareManager {

namespace {
int ChargerEnablePin = 352;
int ChargerStatusPin = 363;
}  // namespace

BatteryCharger::BatteryCharger()
: m_chargerEnablePin( ChargerEnablePin )
, m_chargerStatusPin( ChargerStatusPin ) {
    if( !m_chargerEnablePin.setDirection( common::GpioDirection::OUT ) ) {
        throw BatteryChargerException( "Failed to setup charger ENABLE pin" );
    }
    if( !m_chargerStatusPin.setDirection( common::GpioDirection::IN ) ) {
        throw BatteryChargerException( "Failed to setup charger STATUS pin" );
    }
}

bool BatteryCharger::isCharging() const {
    if( m_chargerEnablePin.getState() == common::GpioState::OFF ) {      // SHDN pin, active low
        return m_chargerStatusPin.getState() == common::GpioState::OFF;  // CHG pin, low is during charging
    }

    return false;
}

bool BatteryCharger::enableCharger() {
    return m_chargerEnablePin.setState( common::GpioState::OFF );  // SHDN pin, active low
}

bool BatteryCharger::disableCharger() {
    return m_chargerEnablePin.setState( common::GpioState::ON );  // SHDN pin, active low
}

}  // namespace HardwareManager
}  // namespace HubPlatform
