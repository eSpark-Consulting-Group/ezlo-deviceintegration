#pragma once

#include "base_utils/zwave/zwave_interface.h"

class ZwaveMockInterface final : public ZwaveChipInterface {
public:
    const std::string& getName() const override;
    bool isSupported() override;
    bool readID( ChipMemoryVector& outResult ) override;
    bool readFlashCrc( ChipMemoryVector& outResult ) override;
    bool checkFlashCrc() override;
    size_t readFlash( ChipMemoryVector& outResult ) override;
    size_t writeFlash( const ChipMemoryVector& data ) override;
    size_t readNvr( ChipMemoryVector& outResult ) override;
    size_t writeNvr( const ChipMemoryVector& data ) override;
    uint16_t calcNvrCrc( const ChipMemoryVector& data ) override;
    const size_t getNvrSize() const override;
    bool bulkErase() override;
    std::string nvrGetRegion( const std::vector< uint8_t >& nvrData ) override;
    void nvrSetRegion( const std::string& newRegion, ChipMemoryVector& nvrData ) override;
    uint32_t nvrGetSawC( const ChipMemoryVector& nvrData ) override;
    void nvrSetSawC( uint32_t nvrSawCValue, ChipMemoryVector& nvrData ) override;

protected:
    bool setProgrammingOn() override;
    bool setProgrammingOff() override;
    int32_t init( uint32_t speed, const bool keepCSActivated = true ) override;
    void deinit() override;
    int getDescriptor() const override;
    int sendRequest( const std::vector< uint8_t >& command,
                     int32_t resultSize,
                     std::vector< uint8_t >& outResult ) override;
    bool isValid() const override;
};
