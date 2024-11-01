#pragma once

#include "hardware_manager/battery/battery_manager.h"
#include "hardware_manager/battery/battery_charger.h"

#include "service_interfaces/service.h"

#include <deque>

namespace HubPlatform {
namespace HardwareManager {
namespace EmpiricalBatteryManager {

class BatteryManager final : public HardwareManager::BatteryManager {
public:
    BatteryManager( Services::Service& hostService );

    void setUp( BatteryInfoChangedCallback callback ) override;
    void tearDown() override;
    void reset() override;

    // NOTE: should be called only from taskLoop passed to ctor (hostService)
    BatteryInfo getInfo() const override;

    bool isHardwareBased() const override;

private:
    void collectInfo();
    int evaluateSoC( float batteryADCValue, bool isCharging ) const;
    float filterBatteryADCValues( float batteryADCValue );
    void resetBatteryADCValuesFilter();

private:
    Services::Service& m_hostService;
    BatteryInfoChangedCallback m_batteryInfoChangedCallback;
    Services::PostponedTaskPtr m_batteryInfoCheckTask;

    BatteryCharger m_batteryCharger;
    BatteryInfo m_lastInfo;
    std::deque< float > m_ADCValuesQueue;
    bool m_isCharging;
};

}  // namespace EmpiricalBatteryManager
}  // namespace HardwareManager
}  // namespace HubPlatform
