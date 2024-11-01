#pragma once

#include <memory>

namespace HubPlatform {
namespace HardwareManager {
namespace Bq27441g1 {

class Bq27441g1;

class DataMemory {
public:
    DataMemory( const Bq27441g1& chip, uint8_t subclassId, uint8_t blockIdx, bool configUpdateMode );

    void commit();

    float readF4( uint8_t offset ) const;

    uint8_t readH1( uint8_t offset ) const;
    uint16_t readH2( uint8_t offset ) const;
    uint32_t readH4( uint8_t offset ) const;

    int8_t readI1( uint8_t offset ) const;
    int16_t readI2( uint8_t offset ) const;

    uint8_t readU1( uint8_t offset ) const;
    uint16_t readU2( uint8_t offset ) const;

    void ensureH1( uint8_t offset, uint8_t value );
    void ensureH2( uint8_t offset, uint16_t value );

    void ensureI1( uint8_t offset, int8_t value );
    void ensureI2( uint8_t offset, int16_t value );

    void ensureU1( uint8_t offset, uint8_t value );
    void ensureU2( uint8_t offset, uint16_t value );

    void rewriteH1( uint8_t offset, uint8_t oldValue, uint8_t newValue );
    void rewriteH2( uint8_t offset, uint16_t oldValue, uint16_t newValue );

private:
    uint8_t address( uint8_t offset ) const;
    bool verifyBlock() const;

private:
    Bq27441g1* m_chip;
    uint8_t m_subclassId;
    uint8_t m_blockIdx;
    uint8_t m_checksum;
    bool m_configUpdateMode;
};

}  // namespace Bq27441g1
}  // namespace HardwareManager
}  // namespace HubPlatform
