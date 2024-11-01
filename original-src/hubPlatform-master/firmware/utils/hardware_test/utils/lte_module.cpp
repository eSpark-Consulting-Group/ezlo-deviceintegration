#include "hardware_test/utils/lte_module.h"

#include "hardware_test/utils/pin_guard.h"
#include "hardware_test/utils/test_log.h"
#include "base_utils/utils.h"

#include <thread>
#include <cstring>
#include <sstream>
#include <map>
#include <regex>

#include <sys/stat.h>

namespace HubPlatform {
namespace HardwareTest {

#define LTE_PWRKEY 201
#define LTE_RESET 100
#define LTE_STATUS 203

LteModule::LteModule( TestLog& logger )
: m_logger( logger )
, m_powerPin( LTE_PWRKEY )
, m_resetPin( LTE_RESET )
, m_statusPin( LTE_STATUS ) {
    if( !m_powerPin.setDirection( common::GpioDirection::OUT ) )
        throw std::runtime_error( "Can't set direction for LTE power pin\n" );
    if( !m_resetPin.setDirection( common::GpioDirection::OUT ) )
        throw std::runtime_error( "Can't set direction for LTE reset pin\n" );
    if( !m_statusPin.setDirection( common::GpioDirection::IN ) )
        throw std::runtime_error( "Can't set direction for LTE status pin\n" );

    m_powerPin.setState( common::GpioState::OFF );
    m_resetPin.setState( common::GpioState::OFF );

    moduleIsOn = [this]() { return getPowerState() == LteModule::PowerState::ON; };
    moduleIsOff = [this]() { return getPowerState() == LteModule::PowerState::OFF; };
}

void LteModule::setupSerial() {
    m_serial.reset( new Uart( m_serialPath, false, false, true ) );
}

std::string LteModule::executeCmd( const std::string& cmd, const std::chrono::milliseconds& delayBeforeAnswer ) {
    std::stringstream err_msg;
    if( !m_serial )
        setupSerial();

    m_serial->flush();
    if( m_serial->write( cmd.data(), cmd.size() ) < 0 ) {
        err_msg << "LTE command \"" << cmd << "\" failed: unable to write to serial\n";
        throw std::runtime_error( err_msg.str() );
    }
    m_serial->flush();

    std::this_thread::sleep_for( delayBeforeAnswer );
    memset( m_response.data(), 0, m_response.size() );
    int read_bytes = m_serial->read( m_response.data(), m_response.size() );
    if( read_bytes < 0 ) {
        err_msg << "LTE command \"" << cmd << "\" failed: unable to read response from serial\n";
        throw std::runtime_error( err_msg.str() );
    }
    auto response = std::string( m_response.data(), std::min< size_t >( read_bytes, m_response.size() ) );
    return response;
}

LteModule::PowerState LteModule::getPowerState() {
    auto pin_state = m_statusPin.getState();
    if( pin_state == common::GpioState::OFF )
        return LteModule::PowerState::ON;
    if( pin_state == common::GpioState::ON )
        return LteModule::PowerState::OFF;
    throw std::runtime_error( "Undefined state of LTE status pin" );
}

void LteModule::powerOn() {
    m_logger() << "Powering on module...";
    if( getPowerState() == LteModule::PowerState::ON )
        return;

    PinGuard power_pin( m_powerPin );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    waitForPowerOn();
}

void LteModule::powerOff() {
    m_logger() << "Powering off module...";
    m_serial.reset();

    if( getPowerState() == LteModule::PowerState::OFF )
        return;

    PinGuard powerPin( m_powerPin );
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
}

void LteModule::powerReset() {
    m_logger() << "Power reset";
    m_serial.reset();

    m_resetPin.setState( common::GpioState::ON );
    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    m_resetPin.setState( common::GpioState::OFF );

    waitForPowerOff();
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    waitForPowerOn();
}

void LteModule::reboot() {
    m_serial.reset();

    m_logger() << "Rebooting module ( powering off ) ...";
    if( moduleIsOn() ) {
        PinGuard powerPin( m_powerPin );
        std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );
    }

    waitForPowerOff();
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    m_logger() << "Rebooting module ( powering on  ) ...";
    PinGuard powerPin( m_powerPin );
    std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );

    waitForPowerOn();
}

void LteModule::waitForSerial() {
    m_logger() << "Waiting for serial...";
    struct stat st;
    auto condition = [&st, this]() { return stat( m_serialPath, &st ) != -1; };

    if( false == waitForCondition( condition, std::chrono::seconds( 20 ), std::chrono::milliseconds( 250 ) ) )
        throw std::runtime_error( "LTE wait for serial timeout" );
    m_logger() << "Serial device is found";
}

void LteModule::waitForPowerOn() {
    if( false == waitForCondition( moduleIsOn, std::chrono::seconds( 20 ), std::chrono::milliseconds( 250 ) ) )
        throw std::runtime_error( "LTE module power on timeout" );
}

void LteModule::waitForPowerOff() {
    if( false == waitForCondition( moduleIsOff, std::chrono::seconds( 25 ), std::chrono::milliseconds( 250 ) ) )
        throw std::runtime_error( "LTE module power off timeout" );
}

void LteModule::wakeUp( std::chrono::milliseconds warmUpTime ) {
    bool warmUpRequired = false;
    if( getPowerState() == LteModule::PowerState::OFF ) {
        powerOn();
        warmUpRequired = true;
    }
    waitForSerial();

    if( warmUpRequired )
        std::this_thread::sleep_for( warmUpTime );
}

std::string LteModule::getIMEI() {
    static const std::regex re( "\\D(\\d{15})\\D" );
    std::smatch match;

    auto response = executeCmd( "AT+GSN\r\n" );
    if( !std::regex_search( response, match, re ) )
        throw std::runtime_error( "LTE IMEI not found" );
    m_logger() << "Module IMEI: " << match[ 1 ];
    return match[ 1 ];
}

std::string LteModule::simStatusStr( SimStatus status ) {
    static const std::map< SimStatus, std::string > statusMap{ { SimStatus::NotInserted, "not inserted" },
                                                               { SimStatus::Inserted, "inserted" },
                                                               { SimStatus::Unknown, "unknown" } };
    return statusMap.at( status );
}

LteModule::SimStatus LteModule::getSimStatus() {
    static const std::regex re( "QSIMSTAT: [0-1],([0-2])" );
    std::smatch match;

    auto response = executeCmd( "AT+QSIMSTAT?\r\n" );
    if( !std::regex_search( response, match, re ) )
        throw std::runtime_error( "Unknown LTE SIM status response" );
    return static_cast< SimStatus >( std::stoi( match[ 1 ] ) );
}

std::string LteModule::getSimICCID() {
    static const std::regex re( "\\D*(\\d{5,24})\\D*" );
    std::smatch match;

    auto response = executeCmd( "AT+QCCID\r\n" );
    if( !std::regex_match( response, match, re ) )
        throw std::runtime_error( "SIM ICCID not found" );
    return match[ 1 ];
}

int LteModule::getRSSI() {
    static const std::regex re( "CSQ: (\\d{1,2}),\\d{1,2}" );
    std::smatch match;

    auto response = executeCmd( "AT+CSQ\r\n", std::chrono::milliseconds( 500 ) );
    if( !std::regex_search( response, match, re ) ) {
        throw std::runtime_error( "Failed to get modem rssi, " + response );
    }

    auto rssiUnits = std::stoi( match[ 1 ] );
    if( rssiUnits == 99 ) {
        return -115;
    }

    return -113 + 2 * rssiUnits;
}

void LteModule::reattach() {
    executeCmd( "AT+COPS=2\r\n", std::chrono::seconds( 2 ) );
    executeCmd( "AT+COPS=0\r\n", std::chrono::seconds( 3 ) );
}

void LteModule::disattach() {
    executeCmd( "AT+COPS=2\r\n", std::chrono::seconds( 1 ) );
}

double LteModule::getADCValue() {
    auto response = executeCmd( "AT+QADC=0\r\n", std::chrono::milliseconds( 500 ) );

    static const std::regex re( "QADC: 1,(.*)" );
    std::smatch match;
    if( std::regex_search( response, match, re ) ) {
        return std::stof( match[ 1 ] );
    }

    throw std::runtime_error( "Failed to get battery ADC value" );
}

}  // namespace HardwareTest
}  // namespace HubPlatform
