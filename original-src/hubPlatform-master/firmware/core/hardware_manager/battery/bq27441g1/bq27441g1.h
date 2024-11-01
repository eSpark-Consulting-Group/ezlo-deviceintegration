#pragma once

#include "hardware_manager/battery/bq27441g1/standard_commands.h"
#include "hardware_manager/battery/bq27441g1/extended_commands.h"
#include "hardware_manager/battery/bq27441g1/control_functions.h"
#include "hardware_manager/battery/bq27441g1/control_status.h"
#include "hardware_manager/battery/bq27441g1/flags.h"
#include "hardware_manager/battery/bq27441g1/soh_status.h"
#include "hardware_manager/battery/bq27441g1/data_memory.h"
#include "hardware_manager/battery/bq27441g1/exceptions.h"

#include "json_utils/json_fwd.h"

#include <memory>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

// Product page: http://www.ti.com/product/BQ27441-G1
// Quick start:  http://www.ti.com/lit/ug/sluuap7/sluuap7.pdf
// Datasheet:    http://www.ti.com/lit/ds/symlink/bq27441-g1.pdf
// Manual:       http://www.ti.com/lit/ug/sluuac9a/sluuac9a.pdf

class Bq27441g1 final {
public:
    Bq27441g1();
    ~Bq27441g1();

    bool ensureOpen();

    Json basicDump() const;
    Json readLearned() const;
    void writeData( const Json& data );

public:
    ControlStatus controlReadStatus() const;
    uint16_t controlReadDeviceType() const;
    uint16_t controlReadFirmwareVersion() const;
    uint16_t controlReadDmCode() const;
    uint16_t controlReadPreviousCommand() const;
    uint16_t controlReadChemicalIdentifier() const;

public:
    float readTemperature() const;
    uint16_t readVoltage() const;
    Flags readFlags() const;
    uint16_t readNominalAvailableCapacity() const;
    uint16_t readFullAvailableCapacity() const;
    uint16_t readRemainingCapacity() const;
    uint16_t readFullChargeCapacity() const;
    int16_t readAverageCurrent() const;
    int16_t readStandbyCurrent() const;
    int16_t readMaxLoadCurrent() const;
    int16_t readAveragePower() const;
    uint16_t readStateOfCharge() const;
    uint16_t readInternalTemperature() const;
    std::pair< uint8_t, SOHStatus > readStateOfHealth() const;
    uint16_t readRemainingCapacityUnfiltered() const;
    uint16_t readRemainingCapacityFiltered() const;
    uint16_t readFullChargeCapacityUnfiltered() const;
    uint16_t readFullChargeCapacityFiltered() const;
    uint16_t readStateOfChargeUnfiltered() const;

public:
    std::unique_ptr< const DataMemory > readDataMemory( uint8_t subclassID, uint8_t blockIdx ) const;
    std::unique_ptr< DataMemory > updateDataMemory( uint8_t subclassID, uint8_t blockIdx );

public:
    int writeUint16( uint8_t address, uint16_t value );
    int writeUint8( uint8_t address, uint8_t value );

    float readFloat( uint8_t address ) const;
    uint16_t readUint16( uint8_t address ) const;
    uint32_t readUint32( uint8_t address ) const;
    uint8_t readUint8( uint8_t address ) const;
    int16_t readInt16( uint8_t address ) const;
    int8_t readInt8( uint8_t address ) const;
    float readUint16Temperature( uint8_t address ) const;

    void control( ControlFunction function );
    uint16_t controlRead( ControlFunction function ) const;
    void extendedCommand( ExtendedCommand cmd, uint8_t param1, uint8_t param2 );

    static uint8_t msb( StandardCommand cmd );
    static uint8_t lsb( StandardCommand cmd );

    static std::string toString( SOHStatus status );
    static std::string toHexString( uint32_t value );
    static std::string toHexString( uint16_t value );
    static std::string toHexString( uint8_t value );

private:
    int open();
    void close();
    int reopen();
    bool isOpen() const;

private:
    int m_i2c;
    static bool m_reopened;
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
