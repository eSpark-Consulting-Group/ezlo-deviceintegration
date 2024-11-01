#include "hardware_manager/battery/empirical_battery_manager/battery_manager.h"

#include "networkd/remote_interface/network_remote_interface.h"
#include "tiny/tiny_consolecolors.h"
#include "log_utils/ezlo_log.h"
#include "config_reader/config_reader.h"

#include <cmath>
#include <chrono>
#include <map>

namespace HubPlatform {
namespace HardwareManager {
namespace EmpiricalBatteryManager {

namespace {
/* average measured (emperical) data */
const auto BatteryADCPollingDefaultInterval = std::chrono::seconds( 30 );
const auto BatteryADCPollingIntervalConfigPath = "hardware_manager/battery/modemADCPoolingInterval";
const auto MissingBatteryTriggerLevelVoltage = 500.0;
const auto BatteryFullTriggerLevelVoltage = 2000.0;
const auto BattteryEmptyTriggerLevelVoltage = 1790.0;
const auto BatteryDischargingDrainedVoltage = 1500.0;
const auto BatteryDischargingMinVoltage = 1600.0;
const auto BatteryChargingMinVoltage = 1700.0;

using ADCVoltage = int;
using Percentaage = int;

/* average measured (emperical) data */
std::map< ADCVoltage, Percentaage > ChargindCurve = {
    { 1810, 2 },  { 1825, 4 },  { 1842, 6 },  { 1853, 8 },  { 1870, 10 }, { 1876, 12 }, { 1881, 14 },
    { 1886, 16 }, { 1896, 18 }, { 1902, 20 }, { 1906, 22 }, { 1915, 24 }, { 1925, 26 }, { 1929, 28 },
    { 1936, 30 }, { 1943, 32 }, { 1951, 34 }, { 1958, 36 }, { 1966, 38 }, { 1973, 40 }, { 1979, 42 },
    { 1987, 44 }, { 1993, 46 }, { 1998, 48 }, { 2005, 50 }, { 2009, 52 }, { 2014, 54 }, { 2019, 56 },
    { 2023, 58 }, { 2027, 60 }, { 2030, 62 }, { 2035, 64 }, { 2039, 66 }, { 2043, 68 }, { 2047, 70 },
    { 2052, 72 }, { 2055, 74 }, { 2060, 76 }, { 2063, 78 }, { 2066, 80 }, { 2076, 82 }, { 2077, 84 },
    { 2078, 86 }, { 2081, 88 }, { 2082, 90 }, { 2083, 92 }, { 2085, 94 }, { 2085, 96 }, { 2087, 98 }
};

std::map< ADCVoltage, Percentaage > DischargindCurve = {
    { 1783, 2 },  { 1789, 4 },  { 1794, 6 },  { 1799, 8 },  { 1805, 10 }, { 1811, 12 }, { 1817, 14 },
    { 1821, 16 }, { 1827, 18 }, { 1835, 20 }, { 1843, 22 }, { 1851, 24 }, { 1859, 26 }, { 1866, 28 },
    { 1872, 30 }, { 1877, 32 }, { 1883, 34 }, { 1888, 36 }, { 1893, 38 }, { 1898, 40 }, { 1903, 42 },
    { 1907, 44 }, { 1913, 46 }, { 1918, 48 }, { 1923, 50 }, { 1928, 52 }, { 1933, 54 }, { 1937, 56 },
    { 1941, 58 }, { 1946, 60 }, { 1951, 62 }, { 1956, 64 }, { 1961, 66 }, { 1966, 68 }, { 1971, 70 },
    { 1976, 72 }, { 1980, 74 }, { 1985, 76 }, { 1989, 78 }, { 1992, 80 }, { 1995, 82 }, { 1998, 84 },
    { 2001, 86 }, { 2004, 88 }, { 2005, 90 }, { 2010, 92 }, { 2015, 94 }, { 2021, 96 }, { 2030, 98 }
};

}  // namespace

BatteryManager::BatteryManager( Services::Service& hostService )
: m_hostService( hostService )
, m_batteryInfoCheckTask( m_hostService.createPostponedTask( [this]( Services::PostponedTask& ) { collectInfo(); } ) )
, m_isCharging( false ) {}

void BatteryManager::setUp( BatteryInfoChangedCallback callback ) {
    m_batteryInfoChangedCallback = std::move( callback );

    auto batteryPoolingInterval = BatteryADCPollingDefaultInterval;
    try {
        ConfigReader configReader;
        if( configReader.hasValue( BatteryADCPollingIntervalConfigPath ) )
            batteryPoolingInterval = std::chrono::seconds( configReader.getInt( BatteryADCPollingIntervalConfigPath ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to read battery ADC pooling interval" ) );
    }
    LOGGER_DEBUG_RAW( cc::debug( "Battery ADC pooling interval: " ) + cc::num10( batteryPoolingInterval.count() ) );

    m_batteryInfoCheckTask->start( std::chrono::seconds( 0 ), batteryPoolingInterval );
}

void BatteryManager::tearDown() {
    m_batteryInfoCheckTask->stop();
}

void BatteryManager::reset() {
    resetBatteryADCValuesFilter();
}

BatteryInfo BatteryManager::getInfo() const {
    return m_lastInfo;
}

bool BatteryManager::isHardwareBased() const {
    return false;
}

void BatteryManager::collectInfo() {
    Network::RemoteInterface networkdRemoteInterface( m_hostService );

    networkdRemoteInterface.requestADCValue( [this]( float batteryADCValue ) {
        try {
            BatteryInfo info;
            info.health = 50;  // we have no way to determine/evaluate this value right now...

            if( std::isnan( batteryADCValue ) ) {
                LOGGER_WARN_RAW( cc::warn( "Failed to check battery status: invalid ADC value" ) );
                return;
            }

            const auto isCharging = m_batteryCharger.isCharging();
            if( isCharging != m_isCharging ) {
                m_isCharging = isCharging;
                resetBatteryADCValuesFilter();
                return;  // skip a measurement in a transitional state
            }

            batteryADCValue = filterBatteryADCValues( batteryADCValue );
            // LOGGER_DEBUG_RAW( cc::debug( "ADC value: " ) + cc::note( std::to_string( batteryADCValue ) ) );

            if( batteryADCValue <= MissingBatteryTriggerLevelVoltage ) {
                info.status = BatteryStatus::Missing;
                info.health = 0;
                info.remainingTime = 0;
            }
            else if( m_batteryCharger.isCharging() ) {
                info.status = BatteryStatus::Charging;
                info.stateOfCharge = evaluateSoC( batteryADCValue, true );

                if( batteryADCValue < BatteryChargingMinVoltage ) {
                    info.depleted = true;
                }
            }
            else if( batteryADCValue >= BatteryFullTriggerLevelVoltage ) {
                info.status = BatteryStatus::Full;
                info.stateOfCharge = 100;
                info.remainingTime = 0;
            }
            else {
                info.status = BatteryStatus::Discharging;
                info.stateOfCharge = evaluateSoC( batteryADCValue, false );

                if( batteryADCValue <= BattteryEmptyTriggerLevelVoltage ) {
                    info.stateOfCharge = 0;
                }
                if( batteryADCValue < BatteryDischargingMinVoltage ) {
                    info.depleted = true;
                }
                if( batteryADCValue <= BatteryDischargingDrainedVoltage ) {
                    info.status = BatteryStatus::Drained;
                    info.remainingTime = 0;
                    info.stateOfCharge = 0;
                }
            }

            m_lastInfo = info;
            m_batteryInfoChangedCallback( m_lastInfo );
        }
        catch( const std::exception& ex ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to check battery status: failed to handle info " ) +
                             cc::note( ex.what() ) );
        }
        catch( ... ) {
            LOGGER_WARN_RAW( cc::warn( "Failed to check battery status: failed to handle info" ) );
        }
    } );
}

int BatteryManager::evaluateSoC( float batteryADCValue, bool isCharging ) const {
    const int roundedADCValue = std::floor( batteryADCValue );

    if( isCharging ) {
        auto it = ChargindCurve.lower_bound( roundedADCValue );
        if( it != ChargindCurve.end() ) {
            return it->second;
        }
    }
    else {
        auto it = DischargindCurve.lower_bound( roundedADCValue );
        if( it != DischargindCurve.end() ) {
            return it->second;
        }
    }

    throw std::runtime_error( "Failed to evaluate SoC based on ADC value" );
}

float BatteryManager::filterBatteryADCValues( float batteryADCValue ) {
    const int queueSizeMax = 5;

    if( m_ADCValuesQueue.size() >= queueSizeMax ) {
        m_ADCValuesQueue.pop_front();
    }

    m_ADCValuesQueue.push_back( batteryADCValue );

    float sumValue = 0;
    for( const auto& value : m_ADCValuesQueue ) {
        sumValue += value;
    }

    return sumValue /= m_ADCValuesQueue.size();
}

void BatteryManager::resetBatteryADCValuesFilter() {
    m_ADCValuesQueue.clear();
}

}  // namespace EmpiricalBatteryManager
}  // namespace HardwareManager
}  // namespace HubPlatform
