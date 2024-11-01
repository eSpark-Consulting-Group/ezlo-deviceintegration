#pragma once

#include "hardware_test/hardware_tests/hardware_test_base.h"

#if defined( __HUB_TYPE_H2_SECURE__ )
#include "hardware_manager/battery/bq27441g1/bq27441g1.h"
#endif

#include <tuple>
#include <memory>

namespace HubPlatform {
namespace HardwareTest {

class BatteryTest final : public HardwareTestBase {
public:
    class MeterBase {
    public:
        enum class MeterType { BatteryGauge, ModemADC };
        virtual MeterType getMeterType() = 0;
        virtual bool isBatteryAttached() = 0;
        virtual int getVoltage() = 0;
        virtual int getCurrent() = 0;
    };

    BatteryTest() = delete;
    explicit BatteryTest( const volatile sig_atomic_t& exitSignalRef );

public:
    TestResult run() override;

private:
    void initTest();
    std::tuple< bool, int, int > waitForVoltageCurrent( int minExpectedVoltage,
                                                        int maxExpectedVoltage,
                                                        int minExpectedCurrent,
                                                        int maxExpectedCurrent );

private:
    std::unique_ptr< MeterBase > m_meter;

    int m_MinChargeVoltage;
    int m_MaxChargeVoltage;
    int m_MinChargeCurrent;
    int m_MaxChargeCurrent;

    int m_MinIdleVoltage;
    int m_MaxIdleVoltage;
    int m_MinIdleCurrent;
    int m_MaxIdleCurrent;

    int m_MinDischargeVoltage;
    int m_MaxDischargeVoltage;
    int m_MinDischargeCurrent;
    int m_MaxDischargeCurrent;
};

}  // namespace HardwareTest
}  // namespace HubPlatform
