#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>

class ZwaveChipInterface {
public:
    using ChipMemoryVector = std::vector< uint8_t >;

public:
    ZwaveChipInterface() = default;
    virtual ~ZwaveChipInterface() = default;

public:
    virtual const std::string& getName() const = 0;
    virtual bool isSupported() = 0;
    //    virtual bool readStatus( ChipMemoryVector& outResult ) = 0;
    //    virtual bool readLocks( ChipMemoryVector& outResult ) = 0;
    virtual bool readID( ChipMemoryVector& outResult ) = 0;
    virtual bool readFlashCrc( ChipMemoryVector& outResult ) = 0;
    virtual bool checkFlashCrc() = 0;
    virtual size_t readFlash( ChipMemoryVector& outResult ) = 0;
    virtual size_t writeFlash( const ChipMemoryVector& data ) = 0;
    virtual size_t readNvr( ChipMemoryVector& outResult ) = 0;
    virtual size_t writeNvr( const ChipMemoryVector& data ) = 0;
    virtual uint16_t calcNvrCrc( const ChipMemoryVector& data ) = 0;
    virtual const size_t getNvrSize() const = 0;
    virtual bool bulkErase() = 0;
    virtual std::string nvrGetRegion( const std::vector< uint8_t >& nvrData ) = 0;
    virtual void nvrSetRegion( const std::string& newRegion, ChipMemoryVector& nvrData ) = 0;
    virtual uint32_t nvrGetSawC( const ChipMemoryVector& nvrData ) = 0;
    virtual void nvrSetSawC( uint32_t nvrSawCValue, ChipMemoryVector& nvrData ) = 0;

protected:
    virtual bool setProgrammingOn() = 0;
    virtual bool setProgrammingOff() = 0;
    virtual int32_t init( uint32_t speed, const bool keepCSActivated = true ) = 0;
    virtual void deinit() = 0;
    virtual int getDescriptor() const = 0;
    virtual int sendRequest( const std::vector< uint8_t >& command,
                             int32_t resultSize,
                             std::vector< uint8_t >& outResult ) = 0;
    virtual bool isValid() const = 0;
};
