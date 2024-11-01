#pragma once

#include "base_utils/gpio.h"
#include "base_utils/uart/uart.h"

#include <functional>
#include <chrono>
#include <memory>

namespace HubPlatform {
namespace HardwareTest {

class TestLog;

class LteModule {
public:
    enum class PowerState { ON, OFF };
    enum class SimStatus : int { NotInserted = 0, Inserted = 1, Unknown = 2 };
    LteModule( TestLog& logger );

    PowerState getPowerState();
    void powerOn();
    void powerOff();
    void powerReset();
    void reboot();
    void waitForSerial();
    void waitForPowerOn();
    void waitForPowerOff();
    void wakeUp( std::chrono::milliseconds warmUpTime );

    std::string getIMEI();
    SimStatus getSimStatus();
    std::string simStatusStr( SimStatus status );
    std::string getSimICCID();
    int getRSSI();
    void reattach();
    void disattach();
    double getADCValue();

private:
    void setupSerial();
    std::string executeCmd( const std::string& cmd,
                            const std::chrono::milliseconds& delayBeforeAnswer = std::chrono::milliseconds( 200 ) );
    std::function< bool() > moduleIsOn;
    std::function< bool() > moduleIsOff;

private:
    TestLog& m_logger;
    common::Gpio m_powerPin;
    common::Gpio m_resetPin;
    common::Gpio m_statusPin;
    std::unique_ptr< Uart > m_serial;
    std::array< char, 1024 > m_response;
    const char* m_serialPath = "/dev/ttyUSB3";
};

}  // namespace HardwareTest
}  // namespace HubPlatform
