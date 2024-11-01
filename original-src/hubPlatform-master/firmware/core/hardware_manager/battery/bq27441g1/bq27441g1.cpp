#include "hardware_manager/battery/bq27441g1/bq27441g1.h"
#include "hardware_manager/battery/bq27441g1/op_config_register.h"
#include "hardware_manager/battery/bq27441g1/classes.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include "i2c/i2c-dev.h"

#include <fcntl.h>
#include <unistd.h>
#include <thread>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

namespace {
const char* I2cDevicePath = "/dev/battery";
constexpr uint8_t I2cChipAddress = 0x55;

}  // namespace

bool Bq27441g1::m_reopened = false;

Bq27441g1::Bq27441g1()
: m_i2c( open() ) {
    if( !m_reopened ) {
        m_i2c = reopen();
        m_reopened = true;
    }
}

Bq27441g1::~Bq27441g1() {
    close();
}

int Bq27441g1::open() {
    int fd = i2c_dev_open( I2cDevicePath );

    if( fd < 0 ) {
        Log::error( cc::error( "Can not open " ) + cc::note( I2cDevicePath ) + cc::error( " I2C bus" ) );
        return -1;
    }

    fd = i2c_set_slave_addr( fd, I2cChipAddress, true );

    if( fd < 0 ) {
        Log::error( cc::error( "Can not set " ) + cc::note( I2cDevicePath ) + cc::error( " I2C bus slave address " ) +
                    cc::num0x( I2cChipAddress ) );
        return -1;
    }

    return fd;
}

void Bq27441g1::close() {
    ::close( m_i2c );
}

int Bq27441g1::reopen() {
    close();
    return open();
}

bool Bq27441g1::isOpen() const {
    return m_i2c >= 0;
}

bool Bq27441g1::ensureOpen() {
    if( !isOpen() ) {
        m_i2c = reopen();
    }
    if( !isOpen() )
        return false;

    union i2c_smbus_data data;
    return i2c_smbus_access( m_i2c, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data ) == 0;
}

Json Bq27441g1::basicDump() const {
    Json result;

    result[ "temperature" ] = readTemperature();
    result[ "voltage" ] = readVoltage();
    result[ "nominalAvailableCapacity" ] = readNominalAvailableCapacity();
    result[ "fullAvailableCapacity" ] = readFullAvailableCapacity();
    result[ "remainingCapacity" ] = readRemainingCapacity();
    result[ "fullChargeCapacity" ] = readFullChargeCapacity();
    result[ "averageCurrent" ] = readAverageCurrent();
    result[ "averagePower" ] = readAveragePower();
    result[ "standbyCurrent" ] = readStandbyCurrent();
    result[ "maxLoadCurrent" ] = readMaxLoadCurrent();
    result[ "averagePower" ] = readAveragePower();
    result[ "stateOfCharge" ] = readStateOfCharge();
    result[ "internalTemperature" ] = readInternalTemperature();
    result[ "remainingCapacityUnfiltered" ] = readRemainingCapacityUnfiltered();
    result[ "remainingCapacityFiltered" ] = readRemainingCapacityFiltered();
    result[ "fullChargeCapacityUnfiltered" ] = readFullChargeCapacityUnfiltered();
    result[ "fullChargeCapacityFiltered" ] = readFullChargeCapacityFiltered();
    result[ "stateOfChargeUnfiltered" ] = readStateOfChargeUnfiltered();
    result[ "OpConfig" ] = toHexString( readUint16( msb( StandardCommand::OpConfig ) ) );
    result[ "DesignCapacity" ] = readInt16( msb( StandardCommand::DesignCapacity ) );

    auto soh = readStateOfHealth();
    result[ "stateOfHealth" ] = soh.first;
    result[ "stateOfHealthState" ] = toString( soh.second );

    result[ "flags" ] = readFlags().toJson();

    Json contorlJson;
    contorlJson[ "status" ] = controlReadStatus().toJson();
    contorlJson[ "deviceType" ] = toHexString( controlReadDeviceType() );
    contorlJson[ "firmwareVersion" ] = controlReadFirmwareVersion();
    contorlJson[ "dmCode" ] = toHexString( controlReadDmCode() );
    contorlJson[ "chemicalIdentifier" ] = toHexString( controlReadChemicalIdentifier() );
    result[ "control" ] = contorlJson;

    return result;
}

#define ENSURE_DATA_MEMORY( TYPE, NAME )                                         \
    if( block.count( #NAME ) > 0 ) {                                             \
        dataMemory->ensure##TYPE( Offsets::NAME, json_get_int( block, #NAME ) ); \
    }

void Bq27441g1::writeData( const Json& data ) {
    Log::debug( cc::debug( "Writing " ) + cc::j( data ) );
    {
        auto block = data[ "ConfigurationSafety" ];
        if( !block.empty() ) {
            using namespace ConfigurationClass::SafetySubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, OverTemp )
            ENSURE_DATA_MEMORY( I2, UnderTemp )
            ENSURE_DATA_MEMORY( U1, TempHys )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "ConfigurationChargeTermination" ];
        if( !block.empty() ) {
            using namespace ConfigurationClass::ChargeTerminationSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I1, TCASet )
            ENSURE_DATA_MEMORY( I1, TCAClear )
            ENSURE_DATA_MEMORY( I1, FCSet )
            ENSURE_DATA_MEMORY( I1, FCClear )
            ENSURE_DATA_MEMORY( I2, DODatEOCDeltaT )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "ConfigurationData" ];
        if( !block.empty() ) {
            using namespace ConfigurationClass::DataSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I1, InitialStandby )
            ENSURE_DATA_MEMORY( I2, InitialMaxLoad )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "ConfigurationDischarge" ];
        if( !block.empty() ) {
            using namespace ConfigurationClass::DischargeSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( U1, SOC1SetThreshold )
            ENSURE_DATA_MEMORY( U1, SOC1ClearThreshold )
            ENSURE_DATA_MEMORY( U1, SOCFSetThreshold )
            ENSURE_DATA_MEMORY( U1, SOCFClearThreshold )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "ConfigurationPower" ];
        if( !block.empty() ) {
            using namespace ConfigurationClass::PowerSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, HibernateI )
            ENSURE_DATA_MEMORY( I2, HibernateV )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "GasGaugingITCfgBlock0" ];
        if( !block.empty() ) {
            using namespace GasGaugingClass::ITCfgSubclassBlock0;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( U2, RaFilter )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "GasGaugingITCfgBlock1" ];
        if( !block.empty() ) {
            using namespace GasGaugingClass::ITCfgSubclassBlock1;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( U1, FastQmaxStartDOD )
            ENSURE_DATA_MEMORY( U1, FastQmaxEndDOD )
            ENSURE_DATA_MEMORY( I2, FastQmaxStartVoltDelta )
            ENSURE_DATA_MEMORY( U2, FastQmaxCurrentThreshold )
            ENSURE_DATA_MEMORY( U1, FastQmaxMinPoints )
            ENSURE_DATA_MEMORY( U1, MaxQmaxChange )
            ENSURE_DATA_MEMORY( U1, QmaxMaxDelta )
            ENSURE_DATA_MEMORY( U1, MaxDefaultQmax )
            ENSURE_DATA_MEMORY( U1, QmaxFilter )
            ENSURE_DATA_MEMORY( U2, ResRelaxTime )
            ENSURE_DATA_MEMORY( I2, UserRatemA )
            ENSURE_DATA_MEMORY( I2, UserRatemW )
            ENSURE_DATA_MEMORY( U1, MaxSimRate )
            ENSURE_DATA_MEMORY( U1, MinSimRate )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "GasGaugingITCfgBlock2" ];
        if( !block.empty() ) {
            using namespace GasGaugingClass::ITCfgSubclassBlock2;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, MinDeltaVoltage )
            ENSURE_DATA_MEMORY( I2, MaxDeltaVoltage )
            ENSURE_DATA_MEMORY( I2, DeltaVMaxdV )
            ENSURE_DATA_MEMORY( U1, TermVValidt )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "GasGaugingCurrentThresholds" ];
        if( !block.empty() ) {
            using namespace GasGaugingClass::CurrentThresholdsSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, DsgCurrentThreshold )
            ENSURE_DATA_MEMORY( I2, ChgCurrentThreshold )
            ENSURE_DATA_MEMORY( I2, QuitCurrent )
            ENSURE_DATA_MEMORY( U2, DsgRelaxTime )
            ENSURE_DATA_MEMORY( U1, ChgRelaxTime )
            ENSURE_DATA_MEMORY( U1, QuitRelaxTime )
            ENSURE_DATA_MEMORY( U2, MaxIRCorrect )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "GasGaugingStateBlock0" ];
        if( !block.empty() ) {
            using namespace GasGaugingClass::StateSubclassBlock0;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, QmaxCell0 )
            ENSURE_DATA_MEMORY( I2, ReserveCapmAh )
            ENSURE_DATA_MEMORY( I2, QInvalidMaxV )
            ENSURE_DATA_MEMORY( I2, QInvalidMinV )
            ENSURE_DATA_MEMORY( I2, DesignCapacity )
            ENSURE_DATA_MEMORY( I2, DesignEnergy )
            ENSURE_DATA_MEMORY( I2, DefaultDesignCap )
            ENSURE_DATA_MEMORY( I2, TerminateVoltage )
            ENSURE_DATA_MEMORY( I2, TRise )
            ENSURE_DATA_MEMORY( I2, TTimeConstant )
            ENSURE_DATA_MEMORY( U1, SOCIDelta )
            ENSURE_DATA_MEMORY( I2, TaperRate )
            ENSURE_DATA_MEMORY( I2, TaperVoltage )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "GasGaugingStateBlock1" ];
        if( !block.empty() ) {
            using namespace GasGaugingClass::StateSubclassBlock1;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, VatChgTerm )
            ENSURE_DATA_MEMORY( I2, AvgILastRun )
            ENSURE_DATA_MEMORY( I2, AvgPLastRun )
            ENSURE_DATA_MEMORY( I2, DeltaVoltage )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "RaTablesR_aRAM" ];
        if( !block.empty() ) {
            using namespace RaTablesClass::R_aRAMSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, R_a0_0 )
            ENSURE_DATA_MEMORY( I2, R_a0_1 )
            ENSURE_DATA_MEMORY( I2, R_a0_2 )
            ENSURE_DATA_MEMORY( I2, R_a0_3 )
            ENSURE_DATA_MEMORY( I2, R_a0_4 )
            ENSURE_DATA_MEMORY( I2, R_a0_5 )
            ENSURE_DATA_MEMORY( I2, R_a0_6 )
            ENSURE_DATA_MEMORY( I2, R_a0_7 )
            ENSURE_DATA_MEMORY( I2, R_a0_8 )
            ENSURE_DATA_MEMORY( I2, R_a0_9 )
            ENSURE_DATA_MEMORY( I2, R_a0_10 )
            ENSURE_DATA_MEMORY( I2, R_a0_11 )
            ENSURE_DATA_MEMORY( I2, R_a0_12 )
            ENSURE_DATA_MEMORY( I2, R_a0_13 )
            ENSURE_DATA_MEMORY( I2, R_a0_14 )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "CalibrationData" ];
        if( !block.empty() ) {
            using namespace CalibrationClass::DataSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I1, BoardOffset )
            ENSURE_DATA_MEMORY( I1, IntTempOffset )
            ENSURE_DATA_MEMORY( I1, PackVOffset )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "CalibrationCCCal" ];
        if( !block.empty() ) {
            using namespace CalibrationClass::CCCalSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( I2, CCOffset )
            ENSURE_DATA_MEMORY( I2, CCCalTemp )
            // TODO: add support
            // ENSURE_DATA_MEMORY( F4, CCGain )
            // ENSURE_DATA_MEMORY( F4, CCDelta )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "CalibrationCurrent" ];
        if( !block.empty() ) {
            using namespace CalibrationClass::CurrentSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            ENSURE_DATA_MEMORY( U1, Deadband )
            dataMemory->commit();
        }
    }
    {
        auto block = data[ "SecurityClass" ];
        if( !block.empty() ) {
            using namespace SecurityClass::CodesSubclass;
            auto dataMemory = updateDataMemory( ID, BlockIdx );
            // TODO: add support
            // ENSURE_DATA_MEMORY( H4, SealedtoUnsealed )
            dataMemory->commit();
        }
    }
}

#undef ENSURE_DATA_MEMORY

Json Bq27441g1::readLearned() const {
    Json result;

    {
        using namespace ConfigurationClass::SafetySubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "OverTemp" ] = dataMemory->readI2( Offsets::OverTemp );
        data[ "UnderTemp" ] = dataMemory->readI2( Offsets::UnderTemp );
        data[ "TempHys" ] = dataMemory->readU1( Offsets::TempHys );
        result[ "ConfigurationSafety" ] = data;
    }
    {
        using namespace ConfigurationClass::ChargeTerminationSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "TCASet" ] = dataMemory->readI1( Offsets::TCASet );
        data[ "TCAClear" ] = dataMemory->readI1( Offsets::TCAClear );
        data[ "FCSet" ] = dataMemory->readI1( Offsets::FCSet );
        data[ "FCClear" ] = dataMemory->readI1( Offsets::FCClear );
        data[ "DODatEOCDeltaT" ] = dataMemory->readI2( Offsets::DODatEOCDeltaT );
        result[ "ConfigurationChargeTermination" ] = data;
    }
    {
        using namespace ConfigurationClass::DataSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "InitialStandby" ] = dataMemory->readI1( Offsets::InitialStandby );
        data[ "InitialMaxLoad" ] = dataMemory->readI2( Offsets::InitialMaxLoad );
        result[ "ConfigurationData" ] = data;
    }
    {
        using namespace ConfigurationClass::DischargeSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "SOC1SetThreshold" ] = dataMemory->readU1( Offsets::SOC1SetThreshold );
        data[ "SOC1ClearThreshold" ] = dataMemory->readU1( Offsets::SOC1ClearThreshold );
        data[ "SOCFSetThreshold" ] = dataMemory->readU1( Offsets::SOCFSetThreshold );
        data[ "SOCFClearThreshold" ] = dataMemory->readU1( Offsets::SOCFClearThreshold );
        result[ "ConfigurationDischarge" ] = data;
    }
    //    {
    //        using namespace ConfigurationClass::RegistersSubclass;
    //        auto dataMemory = readDataMemory( ID, BlockIdx );
    //
    //        Json data;
    //        data[ "OpConfig" ] = OpConfigRegister( dataMemory->readH2( Offsets::OpConfig ) ).toJson();
    //        data[ "OpConfigB" ] = OpConfigBRegister( dataMemory->readH1( Offsets::OpConfigB ) ).toJson();
    //
    //        result[ "ConfigurationRegisters" ] = data;
    //    }
    {
        using namespace ConfigurationClass::PowerSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "HibernateI" ] = dataMemory->readI2( Offsets::HibernateI );
        data[ "HibernateV" ] = dataMemory->readI2( Offsets::HibernateV );
        result[ "ConfigurationPower" ] = data;
    }

    {
        using namespace GasGaugingClass::ITCfgSubclassBlock0;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "RaFilter" ] = dataMemory->readU2( Offsets::RaFilter );
        result[ "GasGaugingITCfgBlock0" ] = data;
    }
    {
        using namespace GasGaugingClass::ITCfgSubclassBlock1;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "FastQmaxStartDOD" ] = dataMemory->readU1( Offsets::FastQmaxStartDOD );
        data[ "FastQmaxEndDOD" ] = dataMemory->readU1( Offsets::FastQmaxEndDOD );
        data[ "FastQmaxStartVoltDelta" ] = dataMemory->readI2( Offsets::FastQmaxStartVoltDelta );
        data[ "FastQmaxCurrentThreshold" ] = dataMemory->readU2( Offsets::FastQmaxCurrentThreshold );
        data[ "FastQmaxMinPoints" ] = dataMemory->readU1( Offsets::FastQmaxMinPoints );
        data[ "MaxQmaxChange" ] = dataMemory->readU1( Offsets::MaxQmaxChange );
        data[ "QmaxMaxDelta" ] = dataMemory->readU1( Offsets::QmaxMaxDelta );
        data[ "MaxDefaultQmax" ] = dataMemory->readU1( Offsets::MaxDefaultQmax );
        data[ "QmaxFilter" ] = dataMemory->readU1( Offsets::QmaxFilter );
        data[ "ResRelaxTime" ] = dataMemory->readU2( Offsets::ResRelaxTime );
        data[ "UserRatemA" ] = dataMemory->readI2( Offsets::UserRatemA );
        data[ "UserRatemW" ] = dataMemory->readI2( Offsets::UserRatemW );
        data[ "MaxSimRate" ] = dataMemory->readU1( Offsets::MaxSimRate );
        data[ "MinSimRate" ] = dataMemory->readU1( Offsets::MinSimRate );
        result[ "GasGaugingITCfgBlock1" ] = data;
    }
    {
        using namespace GasGaugingClass::ITCfgSubclassBlock2;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "MinDeltaVoltage" ] = dataMemory->readI2( Offsets::MinDeltaVoltage );
        data[ "MaxDeltaVoltage" ] = dataMemory->readI2( Offsets::MaxDeltaVoltage );
        data[ "DeltaVMaxdV" ] = dataMemory->readI2( Offsets::DeltaVMaxdV );
        data[ "TermVValidt" ] = dataMemory->readU1( Offsets::TermVValidt );
        result[ "GasGaugingITCfgBlock2" ] = data;
    }
    {
        using namespace GasGaugingClass::CurrentThresholdsSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "DsgCurrentThreshold" ] = dataMemory->readI2( Offsets::DsgCurrentThreshold );
        data[ "ChgCurrentThreshold" ] = dataMemory->readI2( Offsets::ChgCurrentThreshold );
        data[ "QuitCurrent" ] = dataMemory->readI2( Offsets::QuitCurrent );
        data[ "DsgRelaxTime" ] = dataMemory->readU2( Offsets::DsgRelaxTime );
        data[ "ChgRelaxTime" ] = dataMemory->readU1( Offsets::ChgRelaxTime );
        data[ "QuitRelaxTime" ] = dataMemory->readU1( Offsets::QuitRelaxTime );
        data[ "MaxIRCorrect" ] = dataMemory->readU2( Offsets::MaxIRCorrect );
        result[ "GasGaugingCurrentThresholds" ] = data;
    }
    {
        using namespace GasGaugingClass::StateSubclassBlock0;
        auto dataMemory = readDataMemory( ID, BlockIdx );

        //        auto loadSelect = LoadSelectModeValue( dataMemory->readH1( Offsets::LoadSelectMode ) );
        //        Json loadSelectJson;
        //        loadSelectJson[ "LoadSelect" ] = loadSelect.LoadSelect;
        //        loadSelectJson[ "LoadMode" ] = loadSelect.LoadMode;

        Json data;
        data[ "QmaxCell0" ] = dataMemory->readI2( Offsets::QmaxCell0 );
        //        data[ "UpdateStatus" ] = dataMemory->readH1( Offsets::UpdateStatus );
        data[ "ReserveCapmAh" ] = dataMemory->readI2( Offsets::ReserveCapmAh );
        //        data[ "LoadSelectMode" ] = loadSelectJson;
        data[ "QInvalidMaxV" ] = dataMemory->readI2( Offsets::QInvalidMaxV );
        data[ "QInvalidMinV" ] = dataMemory->readI2( Offsets::QInvalidMinV );
        data[ "DesignCapacity" ] = dataMemory->readI2( Offsets::DesignCapacity );
        data[ "DesignEnergy" ] = dataMemory->readI2( Offsets::DesignEnergy );
        data[ "DefaultDesignCap" ] = dataMemory->readI2( Offsets::DefaultDesignCap );
        data[ "TerminateVoltage" ] = dataMemory->readI2( Offsets::TerminateVoltage );
        data[ "TRise" ] = dataMemory->readI2( Offsets::TRise );
        data[ "TTimeConstant" ] = dataMemory->readI2( Offsets::TTimeConstant );
        data[ "SOCIDelta" ] = dataMemory->readU1( Offsets::SOCIDelta );
        data[ "TaperRate" ] = dataMemory->readI2( Offsets::TaperRate );
        data[ "TaperVoltage" ] = dataMemory->readI2( Offsets::TaperVoltage );
        result[ "GasGaugingStateBlock0" ] = data;
    }
    {
        using namespace GasGaugingClass::StateSubclassBlock1;
        auto dataMemory = readDataMemory( ID, BlockIdx );

        Json data;
        data[ "VatChgTerm" ] = dataMemory->readI2( Offsets::VatChgTerm );
        data[ "AvgILastRun" ] = dataMemory->readI2( Offsets::AvgILastRun );
        data[ "AvgPLastRun" ] = dataMemory->readI2( Offsets::AvgPLastRun );
        data[ "DeltaVoltage" ] = dataMemory->readI2( Offsets::DeltaVoltage );
        result[ "GasGaugingStateBlock1" ] = data;
    }

    {
        using namespace RaTablesClass::R_aRAMSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "R_a0_0" ] = dataMemory->readI2( Offsets::R_a0_0 );
        data[ "R_a0_1" ] = dataMemory->readI2( Offsets::R_a0_1 );
        data[ "R_a0_2" ] = dataMemory->readI2( Offsets::R_a0_2 );
        data[ "R_a0_3" ] = dataMemory->readI2( Offsets::R_a0_3 );
        data[ "R_a0_4" ] = dataMemory->readI2( Offsets::R_a0_4 );
        data[ "R_a0_5" ] = dataMemory->readI2( Offsets::R_a0_5 );
        data[ "R_a0_6" ] = dataMemory->readI2( Offsets::R_a0_6 );
        data[ "R_a0_7" ] = dataMemory->readI2( Offsets::R_a0_7 );
        data[ "R_a0_8" ] = dataMemory->readI2( Offsets::R_a0_8 );
        data[ "R_a0_9" ] = dataMemory->readI2( Offsets::R_a0_9 );
        data[ "R_a0_10" ] = dataMemory->readI2( Offsets::R_a0_10 );
        data[ "R_a0_11" ] = dataMemory->readI2( Offsets::R_a0_11 );
        data[ "R_a0_12" ] = dataMemory->readI2( Offsets::R_a0_12 );
        data[ "R_a0_13" ] = dataMemory->readI2( Offsets::R_a0_13 );
        data[ "R_a0_14" ] = dataMemory->readI2( Offsets::R_a0_14 );
        result[ "RaTablesR_aRAM" ] = data;
    }

    {
        using namespace CalibrationClass::DataSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "BoardOffset" ] = dataMemory->readI1( Offsets::BoardOffset );
        data[ "IntTempOffset" ] = dataMemory->readI1( Offsets::IntTempOffset );
        data[ "PackVOffset" ] = dataMemory->readI1( Offsets::PackVOffset );
        result[ "CalibrationData" ] = data;
    }
    {
        using namespace CalibrationClass::CCCalSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "CCOffset" ] = dataMemory->readI2( Offsets::CCOffset );
        data[ "CCCalTemp" ] = dataMemory->readI2( Offsets::CCCalTemp );
        data[ "CCGain" ] = dataMemory->readF4( Offsets::CCGain );
        data[ "CCDelta" ] = dataMemory->readF4( Offsets::CCDelta );
        result[ "CalibrationCCCal" ] = data;
    }
    {
        using namespace CalibrationClass::CurrentSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "Deadband" ] = dataMemory->readU1( Offsets::Deadband );
        result[ "CalibrationCurrent" ] = data;
    }

    {
        using namespace SecurityClass::CodesSubclass;
        auto dataMemory = readDataMemory( ID, BlockIdx );
        Json data;
        data[ "SealedToUnsealed" ] = toHexString( dataMemory->readH4( Offsets::SealedtoUnsealed ) );
        result[ "SecurityCodes" ] = data;
    }
    return result;
}

ControlStatus Bq27441g1::controlReadStatus() const {
    return ControlStatus( controlRead( ControlFunction::CONTROL_STATUS ) );
}

uint16_t Bq27441g1::controlReadDeviceType() const {
    return controlRead( ControlFunction::DEVICE_TYPE );
}

uint16_t Bq27441g1::controlReadFirmwareVersion() const {
    return controlRead( ControlFunction::FW_VERSION );
}

uint16_t Bq27441g1::controlReadDmCode() const {
    return controlRead( ControlFunction::DM_CODE );
}

uint16_t Bq27441g1::controlReadPreviousCommand() const {
    return controlRead( ControlFunction::PREV_MACWRITE );
}

uint16_t Bq27441g1::controlReadChemicalIdentifier() const {
    return controlRead( ControlFunction::CHEM_ID );
}

std::unique_ptr< const DataMemory > Bq27441g1::readDataMemory( uint8_t subclassID, uint8_t blockIdx ) const {
    return std::make_unique< const DataMemory >( *this, subclassID, blockIdx, false );
}

std::unique_ptr< DataMemory > Bq27441g1::updateDataMemory( uint8_t subclassID, uint8_t blockIdx ) {
    return std::make_unique< DataMemory >( *this, subclassID, blockIdx, true );
}

float Bq27441g1::readTemperature() const {
    return readUint16Temperature( msb( StandardCommand::Temperature ) );
}

uint16_t Bq27441g1::readVoltage() const {
    return readUint16( msb( StandardCommand::Voltage ) );
}

Flags Bq27441g1::readFlags() const {
    return Flags( readUint16( msb( StandardCommand::Flags ) ) );
}

uint16_t Bq27441g1::readNominalAvailableCapacity() const {
    return readUint16( msb( StandardCommand::NominalAvailableCapacity ) );
}

uint16_t Bq27441g1::readFullAvailableCapacity() const {
    return readUint16( msb( StandardCommand::FullAvailableCapacity ) );
}

uint16_t Bq27441g1::readRemainingCapacity() const {
    return readUint16( msb( StandardCommand::RemainingCapacity ) );
}

uint16_t Bq27441g1::readFullChargeCapacity() const {
    return readUint16( msb( StandardCommand::FullChargeCapacity ) );
}

int16_t Bq27441g1::readAverageCurrent() const {
    return readInt16( msb( StandardCommand::AverageCurrent ) );
}

int16_t Bq27441g1::readStandbyCurrent() const {
    return readInt16( msb( StandardCommand::StandbyCurrent ) );
}

int16_t Bq27441g1::readMaxLoadCurrent() const {
    return readInt16( msb( StandardCommand::MaxLoadCurrent ) );
}

int16_t Bq27441g1::readAveragePower() const {
    return readInt16( msb( StandardCommand::AveragePower ) );
}

uint16_t Bq27441g1::readStateOfCharge() const {
    return readUint16( msb( StandardCommand::StateOfCharge ) );
}

uint16_t Bq27441g1::readInternalTemperature() const {
    return readUint16Temperature( msb( StandardCommand::InternalTemperature ) );
}

std::pair< uint8_t, SOHStatus > Bq27441g1::readStateOfHealth() const {
    uint8_t percentage = readUint8( msb( StandardCommand::StateOfHealth ) );
    auto status = static_cast< SOHStatus >( readUint8( lsb( StandardCommand::StateOfHealth ) ) );
    if( status >= SOHStatus::Reserved ) {
        status = SOHStatus::Reserved;
    }
    return std::make_pair( percentage, status );
}

uint16_t Bq27441g1::readRemainingCapacityUnfiltered() const {
    return readUint16( msb( StandardCommand::RemainingCapacityUnfiltered ) );
}

uint16_t Bq27441g1::readRemainingCapacityFiltered() const {
    return readUint16( msb( StandardCommand::RemainingCapacityFiltered ) );
}

uint16_t Bq27441g1::readFullChargeCapacityUnfiltered() const {
    return readUint16( msb( StandardCommand::FullChargeCapacityUnfiltered ) );
}

uint16_t Bq27441g1::readFullChargeCapacityFiltered() const {
    return readUint16( msb( StandardCommand::FullChargeCapacityFlitered ) );
}

uint16_t Bq27441g1::readStateOfChargeUnfiltered() const {
    return readUint16( msb( StandardCommand::StateOfChargeUnfiltered ) );
}

int Bq27441g1::writeUint16( uint8_t address, uint16_t value ) {
    return i2c_smbus_write_word_data( m_i2c, address, value );
}

int Bq27441g1::writeUint8( uint8_t address, uint8_t value ) {
    return i2c_smbus_write_byte_data( m_i2c, address, value );
}

float Bq27441g1::readFloat( uint8_t address ) const {
    auto val = readUint32( address );
    return reinterpret_cast< float& >( val );
}

uint16_t Bq27441g1::readUint16( uint8_t address ) const {
    return i2c_smbus_read_word_data( m_i2c, address );
}

uint32_t Bq27441g1::readUint32( uint8_t address ) const {
    uint16_t lsb = readUint16( address );
    uint16_t msb = readUint16( address + 2 );
    return msb << 16u | lsb;
}

uint8_t Bq27441g1::readUint8( uint8_t address ) const {
    return i2c_smbus_read_byte_data( m_i2c, address );
}

int8_t Bq27441g1::readInt8( uint8_t address ) const {
    return static_cast< int8_t >( readUint8( address ) );
}
int16_t Bq27441g1::readInt16( uint8_t address ) const {
    return static_cast< int16_t >( readUint16( address ) );
}

float Bq27441g1::readUint16Temperature( uint8_t address ) const {
    return static_cast< float >( readUint16( address ) ) / 10.f - 273.15f;
}

void Bq27441g1::control( ControlFunction function ) {
    if( i2c_smbus_write_word_data( m_i2c, 0x00, static_cast< uint16_t >( StandardCommand::Control ) ) < 0 ) {
        throw CommunicationError( cc::error( "Can not write command for " ) +
                                  cc::num0x( static_cast< uint16_t >( function ) ) );
    }
    if( i2c_smbus_write_word_data( m_i2c, 0x00, static_cast< uint16_t >( function ) ) < 0 ) {
        throw CommunicationError( cc::error( "Can not write sub-command for " ) +
                                  cc::num0x( static_cast< uint16_t >( function ) ) );
    }
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
}

uint16_t Bq27441g1::controlRead( ControlFunction function ) const {
    if( i2c_smbus_write_word_data( m_i2c, 0x00, static_cast< uint16_t >( StandardCommand::Control ) ) < 0 ) {
        throw CommunicationError( cc::error( "Can not write command read for " ) +
                                  cc::num0x( static_cast< uint16_t >( function ) ) );
    }
    if( i2c_smbus_write_word_data( m_i2c, 0x00, static_cast< uint16_t >( function ) ) < 0 ) {
        throw CommunicationError( cc::error( "Can not write sub-command read for " ) +
                                  cc::num0x( static_cast< uint16_t >( function ) ) );
    }
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    return readUint16( 0x00 );
}

void Bq27441g1::extendedCommand( ExtendedCommand cmd, uint8_t param1, uint8_t param2 ) {
    uint16_t param = param2 << 8u | param1;
    if( i2c_smbus_write_word_data( m_i2c, static_cast< uint8_t >( cmd ), param ) < 0 ) {
        throw CommunicationError( cc::error( "Can not write extended command " ) +
                                  cc::num0x( static_cast< uint8_t >( cmd ) ) + cc::error( " with param " ) +
                                  cc::num0x( param ) );
    }
}

uint8_t Bq27441g1::msb( StandardCommand cmd ) {
    return static_cast< uint16_t >( cmd ) >> 8u;
}

uint8_t Bq27441g1::lsb( StandardCommand cmd ) {
    return static_cast< uint16_t >( cmd ) & 0xFFu;
}

std::string Bq27441g1::toString( SOHStatus status ) {
    switch( status ) {
    case SOHStatus::SOHNotValid:
        return "SOH not valid (initialization)";
    case SOHStatus::InstantSOHValueReady:
        return "Instant SOH value ready";
    case SOHStatus::InitialSOHValueReady:
        return "Initial SOH value ready";
    case SOHStatus::SOHValueReady:
        return "SOH value ready";
    default:
        return "Reserved";
    }
}

std::string Bq27441g1::toHexString( uint32_t value ) {
    std::stringstream stream;
    stream << "0x" << std::hex << std::setfill( '0' ) << std::setw( 8 ) << value;
    std::string result( stream.str() );

    return result;
}

std::string Bq27441g1::toHexString( uint16_t value ) {
    std::stringstream stream;
    stream << "0x" << std::hex << std::setfill( '0' ) << std::setw( 4 ) << value;
    std::string result( stream.str() );

    return result;
}

std::string Bq27441g1::toHexString( uint8_t value ) {
    std::stringstream stream;
    stream << "0x" << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast< unsigned >( value );
    std::string result( stream.str() );

    return result;
}
}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
