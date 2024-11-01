#include "base_utils/zwave/zwave_stick.h"
#include "base_utils/zwave/zwave_environment.h"
#include "base_utils/utils.h"
#include "config_reader/config_reader.h"

#include <sys/stat.h>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <cerrno>

using namespace HubPlatform::Environment::Zwave;

namespace {

static constexpr char const ZwaveRegionConfig[] = "zwave/region";

// assume default region US
static constexpr char const DefaultZwaveRegion[] = "US";
static const std::string DefaultFlashFilename = ZwaveBinariesPath + "/" + DefaultZwaveRegion + "_flash.bin";
static const std::array< const std::string, 10 > ZWaveRegions = { "ANZ", "CN", "EU", "HK", "IL",
                                                                  "IN",  "JP", "KR", "RU", "US" };

// TODO move NVR to separate class, implement serializtion/deserialization
// NVR data { nvr.binary.dataOffset, nvrValue }
// nvr data offset
static const std::unordered_map< uint8_t, uint8_t > envmRelatedValues = {
#if( defined( __HUB_TYPE_A20__ ) )
    { 0x0a, 0x04 },  // NVMCS - NVM chip select P0.4
    { 0x10, 0x00 },  // NVMS - NVM chip size in KiloBytes - high byte
    { 0x11, 0x80 },  // NVMS - NVM chip size in KiloBytes - low byte
    { 0x12, 0x01 },  // NVMP - NVM chip page size in Bytes - high byte
    { 0x13, 0x00 },  // NVMP - NVM chip page size in Bytes - low byte
#elif( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) || defined( __HUB_TYPE_BEEWAVE__ ) )
    { 0x07, 0x02 },  // NVR revision
    { 0x08, 0xFF },  // CCAL value
    { 0x09, 0xFF },  // PIN swap - 0xFF - NO PIN swap
    { 0x0a, 0x15 },  // NVMCS - NVM chip select P1.5
    { 0x0F, 0x02 },  // NVMS - NVM chip type - 0x02 is DATA FLASH
    { 0x10, 0x02 },  // NVMS - NVM chip size in KiloBytes - high byte
    { 0x11, 0x00 },  // NVMS - NVM chip size in KiloBytes - low byte
    { 0x12, 0x01 },  // NVMP - NVM chip page size in Bytes - high byte
    { 0x13, 0x00 },  // NVMP - NVM chip page size in Bytes - low byte
    { 0x28, 0x00 },  // TXCAL1
    { 0x29, 0x00 },  // TXCAL2
#else
    { 0, 0 }
#endif
};

}  // namespace

bool fileExists( const std::string& filename ) {
    struct stat buf;
    return ( stat( filename.c_str(), &buf ) != -1 ? true : false );
}

bool isRegionSupported( const std::string& region ) {
    auto result = std::find( ZWaveRegions.begin(), ZWaveRegions.end(), region );
    return ( result != ZWaveRegions.end() );
}

void saveRegion( const std::string& region, int nvrCenterFrequency ) {
    std::ofstream ofs( "/etc/region", std::ofstream::out | std::ofstream::trunc );
    ofs << region << ", " << nvrCenterFrequency << "kHz";
    ofs.close();
}

void setNvrCrc( std::vector< uint8_t >& zwNvrData, const uint16_t crcValue ) {}

std::string nvrCrcToString( uint16_t crcValue, bool upperCase = true ) {
    std::stringstream ss;
    ss << ( upperCase ? std::uppercase : std::nouppercase ) << std::hex << std::setfill( '0' ) << std::setw( 2 )
       << static_cast< int >( crcValue >> 8 ) << std::setw( 2 ) << static_cast< int >( crcValue & 0xFF );
    return ss.str();
}

const std::string& nvmTypeToString( const uint8_t nvmTypeValue ) {
    static const std::string noMemory = "No external NVM chip";
    static const std::string unknownMemory = "Unknown NVM chip: " + hexToString( &nvmTypeValue, 1, true );
    static const std::string eepromType1 = "EEPROM ( AT25256B, CAT25256B )";
    static const std::string flashType1 = "FLASH ( M25PExx, AT45DBxxxE )";
    static const std::string eepromType2 = "EEPROM ( CAT25M01 )";
    static const std::string dataFlashType1 = "DATAFLASH ( AT45DBxxxE with deep-sleep )";
    switch( nvmTypeValue ) {
    case 0x00:
        return ( noMemory );
    case 0x01:
        return ( eepromType1 );
    case 0x02:
        return ( flashType1 );
    case 0x03:
        return ( eepromType2 );
    case 0x04:
        return ( dataFlashType1 );
    default:
        return ( unknownMemory );
    }
}

int zw_flasher( int argc, char* argv[] ) {
    HubPlatform::ConfigReader m_configReader;
    ZWaveStick zWaveStik;
    std::vector< uint8_t > zwFlashData, zwNvrData, envmData;

    bool forceFlashWithoutCRCCheck = false;
    bool testMode = false;
    std::string homeId;
    if( argc == 2 ) {
        std::string argument = argv[ 1 ];
        if( argument == "e" || argument == "-e" ) {
            if( zWaveStik.envmErase() ) {
                std::cout << "ENVM chip erased" << std::endl;
                exit( EXIT_SUCCESS );
            }
        }
        if( argument == "ez" || argument == "-ez" ) {
            if( zWaveStik.zwChipErase() ) {
                std::cout << "ZWave chip erased" << std::endl;
                exit( EXIT_SUCCESS );
            }
        }
        else if( argument == "of" || argument == "-of" ) {
            zWaveStik.zwChipRead( zwNvrData, zwFlashData );
            std::fstream dumpFileOut;
            if( zwFlashData.size() > 0 ) {
                dumpFileOut.open( ZwaveFlashOutBinaryPath, std::ios::out | std::ios::binary );
                dumpFileOut.write( reinterpret_cast< char* >( &zwFlashData[ 0 ] ),
                                   zwFlashData.size() * sizeof( char ) );
                dumpFileOut.close();
                std::cout << "Z-Wave chip FLASH contents saved to " << ZwaveFlashOutBinaryPath << std::endl;
            }
            else {
                std::cerr << "Z-Wave chip FLASH contents read FAILED" << std::endl;
            }
            if( zwNvrData.size() > 0 ) {
                dumpFileOut.open( ZwaveNvrOutBinaryPath, std::ios::out | std::ios::binary );
                dumpFileOut.write( reinterpret_cast< char* >( &zwNvrData[ 0 ] ), zwNvrData.size() * sizeof( char ) );
                dumpFileOut.close();
                std::cout << "Z-Wave chip NVR contents saved to " << ZwaveNvrOutBinaryPath << std::endl;
            }
            else {
                std::cerr << "Z-Wave chip NVR contents read FAILED" << std::endl;
            }

            exit( EXIT_SUCCESS );
        }
        else if( argument == "oe" || argument == "-oe" ) {
            zWaveStik.envmRead( envmData, homeId );
            if( envmData.size() > 0 ) {
                std::fstream dumpFileOut;
                dumpFileOut.open( ZwaveEnvmOutBinaryPath, std::ios::out | std::ios::binary );
                dumpFileOut.write( reinterpret_cast< char* >( &envmData[ 0 ] ), envmData.size() * sizeof( char ) );
                dumpFileOut.close();
                std::cout << "ENVM memory contents saved to " << ZwaveEnvmOutBinaryPath << std::endl;
                exit( EXIT_SUCCESS );
            }
            else {
                std::cerr << "ENVM memory contents read FAILED" << std::endl;
                exit( EXIT_FAILURE );
            }
        }
        else if( argument == "r" || argument == "-r" ) {
            int result = EXIT_SUCCESS;
            std::fstream nvrBinFactoryFile;
            nvrBinFactoryFile.open( ZwaveNvrFactoryBinaryPath, std::ios::in | std::ios::binary | std::ios::ate );
            if( nvrBinFactoryFile.is_open() ) {
                size_t nvrFileLength = nvrBinFactoryFile.tellg();
                if( nvrFileLength == zWaveStik.zwGetNvrSize() ) {
                    zwNvrData.clear();
                    zwNvrData.resize( nvrFileLength );
                    nvrBinFactoryFile.seekg( 0, std::ios::beg );
                    nvrBinFactoryFile.read( (char*) &zwNvrData[ 0 ], nvrFileLength );
                    nvrBinFactoryFile.close();
                    std::cout << "Factory default NVR file read successfully: "
                              << static_cast< int >( zwNvrData.size() ) << " bytes" << std::endl;
                }
                else {
                    std::cerr << "NVR file: " << ZwaveNvrFactoryBinaryPath << " length is NOT valid" << std::endl;
                    result = EXIT_FAILURE;
                }
                zWaveStik.zwChipWrite( zwNvrData, zwFlashData, true );
            }
            else {
                std::cerr << "Opening default NVR file: " << ZwaveNvrFactoryBinaryPath << " FAILED" << std::endl;
                result = EXIT_FAILURE;
            }
            exit( result );
        }
        else if( argument == "f" || argument == "-f" ) {
            forceFlashWithoutCRCCheck = true;
        }
        else if( argument == "fe" || argument == "-fe" ) {
            std::fstream envmDataFileIn;
            envmDataFileIn.open( ZwaveEnvmOutBinaryPath, std::ios::in | std::ios::binary | std::ios::ate );
            if( envmDataFileIn.is_open() ) {
                size_t envmDataFileLength = envmDataFileIn.tellg();
                envmData.clear();
                envmData.resize( envmDataFileLength );
                envmDataFileIn.seekg( 0, std::ios::beg );
                envmDataFileIn.read( (char*) &envmData[ 0 ], envmDataFileLength );
                envmDataFileIn.close();
                std::cout << (int) envmDataFileLength << "  " << (int) envmData.size() << std::endl;
                const bool result = zWaveStik.envmWrite( envmData );
                std::cout << "ENVM memory contents write to chip " << ( result ? "DONE !" : "FAILED !" ) << std::endl;
                exit( ( result ? EXIT_SUCCESS : EXIT_FAILURE ) );
            }
            else {
                std::cerr << "ENVM memory data file open FAILED" << std::endl;
                exit( EXIT_FAILURE );
            }
        }
        else if( argument == "l" || argument == "-l" ) {
            std::string locksResult;
            zWaveStik.envmReadLocks( locksResult );
            std::cout << "ENVM locks: " << locksResult << std::endl;
            exit( EXIT_SUCCESS );
        }
        else if( argument == "t" || argument == "-t" ) {
            testMode = true;
        }
        else {
            std::cout << "Arguments:"
                         "\n\t-e\terase ENVM chip\n\t-ez\terase Z-Wave chip"
                         "\n\t-of\tread flash to file flash.bin.out\n\t-oe\tread ENVM to file envm.bin.out"
                         "\n\t-f\tforced flashing\n\t-t\ttest mode\n\t-h\tthis message"
                      << std::endl;
            exit( EXIT_FAILURE );
        }
    }
    std::string zwChipSignature;
    std::string zwChipCrc;
    std::string envmSignature;
    zWaveStik.zwGetChipInfo( zwChipSignature, zwChipCrc );
    zWaveStik.envmGetChipInfo( envmSignature );

    if( !zwChipSignature.empty() ) {
        std::cout << "ZW JEDEC            : " << zwChipSignature << std::endl;
    }
    if( !zwChipCrc.empty() ) {
        std::cout << "ZW Flash CRC        : " << zwChipCrc << std::endl;
    }
    if( !envmSignature.empty() ) {
        std::cout << "ENVM JEDEC          : " << envmSignature << std::endl;
    }

    static const bool readNvrOnly = true;
    zWaveStik.zwChipRead( zwNvrData, zwFlashData, readNvrOnly );
    std::fstream dumpFileOut;

    uint16_t nvrCrcRead = 0xFFFF, nvrCrcCalculated;
    bool calCoefficientsInitialized = false;
    bool crcMatched = false;
    if( zWaveStik.zwGetNvrSize() > 0 && zwNvrData.size() == zWaveStik.zwGetNvrSize() ) {
        nvrCrcRead = zwNvrData[ crc16HighNvrDataOffset ];
        nvrCrcRead = nvrCrcRead << 8;
        nvrCrcRead += zwNvrData[ crc16LowNvrDataOffset ];

        static const uint8_t nvrRevision = zwNvrData[ revNvrDataOffset ];
        static const uint8_t pinsSwapValue = zwNvrData[ pinsNvrDataOffset ];
        static const uint8_t nvmCsPinValue = zwNvrData[ nvmcsNvrDataOffset ];
        static const uint8_t nvmTypeValue = zwNvrData[ nvmTypeNvrDataOffset ];
        static const uint16_t nvmSizeValue = htons( *(uint16_t*) &zwNvrData[ nvmSizeNvrDataOffset ] );
        static const uint16_t nvmPageSizeValue = htons( *(uint16_t*) &zwNvrData[ nvmPageSizeNvrDataOffset ] );
        static const uint8_t ccalFactoryValue = zwNvrData[ ccalNvrDataOffset ];
        static const uint8_t txcal1FactoryValue = zwNvrData[ txcal1NvrDataOffset ];
        static const uint8_t txcal2FactoryValue = zwNvrData[ txcal2NvrDataOffset ];
        std::cout << "NVR revision        : "
                  << "v" << (int) ( nvrRevision >> 4 ) << "." << ( nvrRevision & 0x0F ) << std::endl;
        std::cout << "Pins swap           : " << hexToString( &pinsSwapValue, sizeof( pinsNvrDataOffset ), true )
                  << std::endl;
        std::cout << "NVM chip select pin : "
                  << "P" << (int) ( nvmCsPinValue >> 4 ) << "_" << ( nvmCsPinValue & 0x0F ) << std::endl;

        std::cout << "NVM chip type       : " << nvmTypeToString( nvmTypeValue ) << std::endl;
        std::cout << "NVM chip size       : " << std::to_string( nvmSizeValue ) << " KBytes" << std::endl;
        std::cout << "NVM chip page size  : " << (int) nvmPageSizeValue << " bytes" << std::endl;

        std::cout << "Factory CCAL        : " << hexToString( &ccalFactoryValue, 1, true ) << std::endl;
        std::cout << "Factory TXCAL1      : " << hexToString( &txcal1FactoryValue, 1, true ) << std::endl;
        std::cout << "Factory TXCAL2      : " << hexToString( &txcal2FactoryValue, 1, true ) << std::endl;
        if( nvrRevision > nvrRevision_v01 ) {
            std::string keyValue;
            keyValue = hexToString( (const uint8_t*) &zwNvrData[ prkNvrDataOffset ], prkNvrDataSize );
            std::cout << "PRK ( private key)  : " << keyValue << std::endl;
            keyValue = hexToString( (const uint8_t*) &zwNvrData[ pukNvrDataOffset ], pukNvrDataSize );
            std::cout << "PUK ( public key)   : " << keyValue << std::endl;
            keyValue = zwaveDskToString( (const uint8_t*) &zwNvrData[ pukNvrDataOffset ] );
            std::cout << "DSK                 : " << keyValue << std::endl;
        }
        std::cout << "NVR CRC16 ( read )  : " << nvrCrcToString( nvrCrcRead ) << std::endl;

        uint32_t calibrationCoefficients = ccalFactoryValue;
        calibrationCoefficients = ( calibrationCoefficients << 8 ) + txcal1FactoryValue;
        calibrationCoefficients = ( calibrationCoefficients << 8 ) + txcal2FactoryValue;

        nvrCrcCalculated = zWaveStik.zwCalcNvrCrc( zwNvrData );
        std::cout << "NVR CRC16 ( calc )  : " << nvrCrcToString( nvrCrcCalculated ) << std::endl;

        crcMatched = ( nvrCrcRead == nvrCrcCalculated );
        std::cout << "NVR CRC16" << ( crcMatched ? " " : " NOT " ) << "matched !" << std::endl;
        calCoefficientsInitialized =
            ( ccalFactoryValue != 0x80 && txcal1FactoryValue != 0xFF && txcal1FactoryValue != 0xFF );
        std::cout << "Calibration coefficients" << ( calCoefficientsInitialized ? " " : " NOT " ) << "initialized !"
                  << std::endl;
    }

    if( crcMatched && calCoefficientsInitialized ) {
        if( !fileExists( ZwaveNvrFactoryBinaryPath ) ) {
            // if read from chip failed
            // TODO get factory default NVR for current build type
            if( zwNvrData.size() == 0 ) {
                zwNvrData.reserve( zWaveStik.zwGetNvrSize() );
                std::fill( zwNvrData.begin(), zwNvrData.end(), 0xFF );
                for( auto const& nvrDataPair : envmRelatedValues ) {
                    zwNvrData[ nvrDataPair.first ] = nvrDataPair.second;
                }
                uint16_t wCRC16 = zWaveStik.zwCalcNvrCrc( zwNvrData );
                setNvrCrc( zwNvrData, wCRC16 );
            }

            dumpFileOut.open( ZwaveNvrFactoryBinaryPath, std::ios::out | std::ios::binary );
            dumpFileOut.write( reinterpret_cast< char* >( &zwNvrData[ 0 ] ), zwNvrData.size() * sizeof( char ) );
            dumpFileOut.close();
            std::cout << "No NVR factory data exist at:     " << ZwaveNvrFactoryBinaryPath << std::endl;
            std::cout << "NVR data read from chip saved to: " << ZwaveNvrFactoryBinaryPath << std::endl;
        }
        else {
            std::cout << "NVR factory exists:   " << ZwaveNvrFactoryBinaryPath << std::endl;
        }
    }
    else {
        if( !forceFlashWithoutCRCCheck ) {
            exit( EXIT_FAILURE );
        }
        else {
            if( zWaveStik.zwGetNvrSize() > 0 ) {
                exit( EXIT_FAILURE );
            }
            std::fstream nvrBinFactoryFile;
            nvrBinFactoryFile.open( ZwaveNvrFactoryBinaryPath, std::ios::in | std::ios::binary | std::ios::ate );
            if( nvrBinFactoryFile.is_open() ) {
                size_t nvrFileLength = nvrBinFactoryFile.tellg();
                if( nvrFileLength == zWaveStik.zwGetNvrSize() ) {
                    zwNvrData.clear();
                    zwNvrData.resize( nvrFileLength );
                    nvrBinFactoryFile.seekg( 0, std::ios::beg );
                    nvrBinFactoryFile.read( (char*) &zwNvrData[ 0 ], nvrFileLength );
                    nvrBinFactoryFile.close();
                    std::cout << "Factory default NVR file read successfully: "
                              << static_cast< int >( zwNvrData.size() ) << " bytes" << std::endl;
                }
                else {
                    std::cerr << "NVR file: " << ZwaveNvrFactoryBinaryPath << " length is NOT valid" << std::endl;
                    exit( EXIT_FAILURE );
                }
            }
            else {
                std::cerr << "Opening default NVR file: " << ZwaveNvrFactoryBinaryPath << " FAILED" << std::endl;
                exit( EXIT_FAILURE );
            }
        }
    }

    auto nvrCenterFrequency = zWaveStik.nvrGetSawC( zwNvrData );

    std::string region;
    auto setRegion = [&]( const std::string& value ) {
        region = value;
        zWaveStik.nvrSetRegion( region, zwNvrData );
    };

    if( m_configReader.hasValue( ZwaveRegionConfig ) &&
        isRegionSupported( region = m_configReader.getString( ZwaveRegionConfig ) ) ) {
        setRegion( region );
        std::cout << "Z-Wave region detected via config file: " << region << std::endl;
    }
    else if( !( region = zWaveStik.nvrGetRegion( zwNvrData ) ).empty() ) {
        std::cout << "Region detected through NVR data: " << region << "\n";
    }
    else {
        std::cout << "Region can't be detected through NVR data or config file!" << std::endl;
    }

    if( !isRegionSupported( region ) ) {
        setRegion( DefaultZwaveRegion );
        std::cout << "Will use default Z-Wave region \"" << region << "\" !" << std::endl;
    }

    nvrCenterFrequency = zWaveStik.nvrGetSawC( zwNvrData );
    std::cout << "SAWC( frequency )   : " << std::to_string( nvrCenterFrequency ) << " kHz" << std::endl;
    std::cout << "SAWC( value )       : " << std::hex << std::uppercase << std::setw( 6 ) << std::setfill( '0' )
              << (std::uint_fast32_t) nvrCenterFrequency << std::endl;

    if( testMode ) {
        std::cout << "Test mode finished.\n";
        exit( EXIT_SUCCESS );
    }

    const std::string flashFilename =
        std::string( ZwaveBinariesPath ).append( "/" ).append( region ).append( "_flash.bin" );
    int result = EXIT_SUCCESS;

    saveRegion( region, nvrCenterFrequency );

    std::fstream dumpFile;

    std::cout << "Flashing binary data: " << flashFilename << "\n";
    dumpFile.open( flashFilename, std::ios::in | std::ios::binary | std::ios::ate );
    if( dumpFile.is_open() ) {
        int flashFileLength = dumpFile.tellg();
        if( flashFileLength > 0 ) {
            std::vector< uint8_t > flashReadBuffer( flashFileLength );
            dumpFile.seekg( 0, std::ios::beg );
            dumpFile.read( (char*) &flashReadBuffer[ 0 ], flashFileLength );
            std::cout << "Flashing started...\n";
            if( !zWaveStik.zwChipWrite( zwNvrData, flashReadBuffer, forceFlashWithoutCRCCheck ) )
                result = EXIT_FAILURE;
        }
        else {
            std::cerr << "Failed to determine filesize: " << flashFilename << " " << strerror( errno ) << "\n";
            result = EXIT_FAILURE;
        }
    }
    else {
        std::cerr << "Failed to open file: " << flashFilename << " " << strerror( errno ) << "\n";
        result = EXIT_FAILURE;
    }
    dumpFile.close();
    exit( result );
}
