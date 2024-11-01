#pragma once

#include "hardware_manager/battery/battery_manager.h"
#include "hardware_manager/battery/bq27441g1/bq27441g1.h"

#include "service_interfaces/task_loop.h"
#include "database_client/idbclient.h"

#include <deque>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

class BatteryManager final : public HardwareManager::BatteryManager {
public:
    BatteryManager( Services::TaskLoop& taskLoop, IDbClient& dbClient );

    void setUp( BatteryInfoChangedCallback callback ) override;
    void tearDown() override;
    void reset() override;

    // NOTE: should be called only from taskLoop passed to ctor
    BatteryInfo getInfo() const override;

    bool isHardwareBased() const override;

private:
    BatteryInfo readInfo();

    bool service();

    void updateConfiguration();
    void storeLearned();

private:
    Services::TaskLoop& m_taskLoop;
    IDbClient& m_dbClient;
    BatteryInfoChangedCallback m_batteryInfoChangedCallback;
    Services::PostponedTaskPtr m_batteryInfoCheckTask;

    Bq27441g1 m_battery;

    std::deque< int16_t > m_lastMeasuredCurrents;
    int32_t m_lastMeasuredCurrentsSum;
    BatteryStatus m_lastStatus;
    BatteryInfo m_lastInfo;
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
