#pragma once

#include <vector>
#include <string>
#include <termios.h>
#include <chrono>
#include <memory>

#include "common_utils/cpp14.h"
#include "base_utils/hubtypes.h"
#include "base_utils/spi/envm_transport.h"
#include "base_utils/spi/actual_transport.h"
#include "base_utils/spi/envm_interface.h"
#include "base_utils/spi/envm_mock.h"
#include "base_utils/zwave/zwave_interface.h"
#include "base_utils/zwave/zm5101_interface.h"
#include "base_utils/zwave/efr32zg14_interface.h"
#include "base_utils/zwave/zwave_mock_interface.h"

#if defined( __HUB_TYPE_A20__ )
#include "base_utils/spi/envm_ST25PE20.h"
using EnvmChip = EnvmSt25Pe20;
using ZwaveChip = Zm5101;
#elif defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ )
#include "base_utils/spi/envm_AT45DB041.h"
using EnvmChip = EnvmAt45Db041;
using ZwaveChip = Zm5101;
#elif defined( __HUB_TYPE_G150__ )
#include "base_utils/spi/envm_AT25256.h"
using EnvmChip = EnvmAt25256;
using ZwaveChip = Zm5101;
#elif defined( __HUB_TYPE_BEEWAVE__ )
using EnvmChip = EnvmMock;
using ZwaveChip = HubPlatform::Efr32Zg14;
#elif defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ ) || defined( __HUB_TYPE_G450__ ) || \
    defined( __HUB_TYPE_G550__ )
#include "base_utils/spi/envm_mock.h"
using EnvmChip = EnvmMock;
using ZwaveChip = ZwaveMockInterface;
#else
#error "Specify valid HUB_TYPE for build..."
#endif

class ZWaveStick {
public:
    ZWaveStick();
    ~ZWaveStick();

public:
    static void zwResetCycle( const size_t delayAfterUs = 0 );
    static void zwResetAssert();
    static void zwResetRelease();

public:
    bool envmSaveToFile( const std::string& filePath, std::string& reason );
    bool envmWriteFromFile( const std::string& filePath, std::string& reason );
    bool envmReadStatus( std::string& outStatus );
    bool envmReadLocks( std::string& outLocks );
    bool envmErase();
    bool envmRead( std::vector< uint8_t >& envmData );
    bool envmRead( std::vector< uint8_t >& envmData, std::string& homeId );
    bool envmWrite( const std::vector< uint8_t >& envmData );
    bool envmGetChipInfo( std::string& envmSignature );

    bool zwGetChipInfo( std::string& chipSignature, std::string& chipFlashCrc );
    bool zwChipRead( std::vector< uint8_t >& nvrData, std::vector< uint8_t >& flashData, const bool nvrOnly = false );
    bool zwChipWrite( const std::vector< uint8_t >& nvrData,
                      const std::vector< uint8_t >& flashData,
                      bool forced = false );
    bool zwChipErase();
    uint16_t zwCalcNvrCrc( const std::vector< uint8_t >& nvrData );
    const size_t zwGetNvrSize() const;

public:
    std::string nvrGetRegion( const std::vector< uint8_t >& nvrData );
    void nvrSetRegion( const std::string& newRegion, std::vector< uint8_t >& nvrData );

    uint32_t nvrGetSawC( const std::vector< uint8_t >& nvrData );
    void nvrSetSawC( uint32_t nvrSawCValue, std::vector< uint8_t >& nvrData );

private:
    static bool envmGetDataFromFile( const std::string& filePath,
                                     std::vector< uint8_t >& envmData,
                                     std::string& reason );
    static std::string getHomeId( const std::vector< uint8_t >& envmData );

private:
    std::unique_ptr< ZwaveChipInterface > m_zwave;
    std::unique_ptr< EnvmInterface > m_envm;
};
