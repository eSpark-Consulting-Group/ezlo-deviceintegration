#pragma once

#include "base_utils/uart/uart.h"
#include "base_utils/spi/actual_transport.h"
#include "base_utils/zwave/zwave_environment.h"
#include "base_utils/zwave/zwave_interface.h"
#include "base_utils/zwave/zwave_serial_api.h"

#include "system_utils/environment.h"
namespace HubPlatform {

class Efr32Zg14 final : public ZwaveChipInterface {
public:
    Efr32Zg14();
    ~Efr32Zg14();

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
    uint16_t ccittCrc16( const uint8_t* pData, uint16_t dataLength );

private:
    ZwaveSerialApi m_serialApi;
};

}  // namespace HubPlatform