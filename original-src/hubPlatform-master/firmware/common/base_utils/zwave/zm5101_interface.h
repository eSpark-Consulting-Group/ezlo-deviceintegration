#pragma once

#include "base_utils/spi/envm_interface.h"
#include "base_utils/spi/spi_transport.h"
#include "base_utils/zwave/zwave_interface.h"
#include "base_utils/zwave/zwave_environment.h"

using HubPlatform::Environment::Zwave::nvrAddressFirst;
using HubPlatform::Environment::Zwave::nvrAddressLast;
using HubPlatform::Environment::Zwave::resetAssertPeriodUs;

class Zm5101 final : public ZwaveChipInterface {
public:
    Zm5101();
    ~Zm5101();

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
    const size_t getNvrSize() const override;
    uint16_t calcNvrCrc( const ChipMemoryVector& data ) override;
    bool bulkErase() override;

public:
    std::string nvrGetRegion( const std::vector< uint8_t >& nvrData );
    void nvrSetRegion( const std::string& newRegion, std::vector< uint8_t >& nvrData );
    std::string nvrGetS2DskKey( const std::vector< uint8_t >& nvrData, std::vector< uint8_t >& outKey );
    void nvrSetS2DskKey( const std::vector< uint8_t >& newKey, std::vector< uint8_t >& nvrData );
    uint32_t nvrGetSawC( const ChipMemoryVector& nvrData );
    void nvrSetSawC( uint32_t nvrSawCValue, ChipMemoryVector& nvrData );

public:
    static void hwResetCycle( size_t afterDelayUs = 0 );
    static void hwResetAssert();
    static void hwResetDeassert();

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

private:
    bool waitChipFlashFsm( const int32_t delayUs = 500, int triesNumber = 10 );
    bool waitChipContinue( const int32_t delayUs = 500, int triesNumber = 10 );
    bool waitRunChipCrcReady( const int32_t delayUs = 1000, int triesNumber = 10 );
    uint16_t ccittCrc16( const uint8_t* pData, uint16_t dataLength );

    void printProgress( const char* message, const size_t done, const size_t max );

private:
    ZwaveTransport m_transport;
    static constexpr int32_t m_maxFrequency = 1000000;
    static constexpr size_t m_programmingOnTResetLowUs = resetAssertPeriodUs;

    static constexpr size_t m_nvrSize = nvrAddressLast - nvrAddressFirst + 1;

    static constexpr size_t m_flashSectorLength = 2048;
    static constexpr size_t m_flashTotalSectors = 64;
    static constexpr size_t m_flashSize = m_flashTotalSectors * m_flashSectorLength;
    static constexpr size_t m_signatureLength = 7;
    static constexpr size_t m_crcLength = 4;

    const EnvmCommand< ChipMemoryVector, 0 > m_cmdEnableInterface;
    const EnvmCommand< ChipMemoryVector, 1 > m_cmdCheckState;

    const EnvmCommand< ChipMemoryVector, 1 > m_cmdReadFlash;
    const EnvmCommand< ChipMemoryVector, 1 > m_cmdReadSram;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdReadContinue;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdWriteFlash;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdWriteSram;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdWriteContinue;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdEraseSector;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdEraseChip;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdResetChip;

    const EnvmCommand< ChipMemoryVector, 8 > m_cmdEooSEnable;
    const EnvmCommand< ChipMemoryVector, 1 > m_cmdReadSignature;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdEooSDisable;

    const EnvmCommand< ChipMemoryVector, 0 > m_cmdSetLockBits;
    const EnvmCommand< ChipMemoryVector, 1 > m_cmdReadLockBits;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdSetNvr;
    const EnvmCommand< ChipMemoryVector, 1 > m_cmdReadNvr;
    const EnvmCommand< ChipMemoryVector, 0 > m_cmdRunCrcCheck;
};