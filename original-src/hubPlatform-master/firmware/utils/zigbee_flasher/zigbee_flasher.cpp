#include "zigbee_flasher/zigbee_flasher.h"

#include "base_utils/timeutils.h"
#include "base_utils/zigbee/zigbee_environment.h"
#include "system_utils/filesystem.h"
#include "ssl_utils/sha256.h"
#include "tiny/tiny_command_line_parser.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace HubPlatform::Environment::Zigbee;

namespace HubPlatform {
namespace ZigbeeFlasher {

namespace {
static const std::string ZigbeeFirmwareFolder( HubPlatform::Environment::getImagesFolder() + "/zigbee" );
// ncp-uart-sw.gbl is a symlink to recent NCP image
static const std::string ZigbeeFirmwareFilename( "ncp-uart-sw.gbl" );
static const std::string ZigbeeFirmwareFilepath( ZigbeeFirmwareFolder + "/" + ZigbeeFirmwareFilename );
static const std::string loadFirmwareOptionCommand = "1\n";

const auto ZigbeeFirmwareIdObj = JSON( "_id"
                                       << "zigbee_firmware_id" );
}  // namespace

ZigbeeFlasher::ZigbeeFlasher( const volatile sig_atomic_t& exitSignalRef )
: m_exitSignalRef( exitSignalRef )
, m_uart( HubPlatform::Environment::getZigbeeDevice(), true )
, m_firmwareFilepath( ZigbeeFirmwareFilepath )
, m_xmodemSender( m_uart )
, m_dbClient( createDbClient() ) {}

int ZigbeeFlasher::setUp( int argc, char* argv[] ) {
    tiny::command_line::parser parser;
    std::string firmwareFilePath;

    parser.on( "Firmware file\nfile", "Select firmware file", [&firmwareFilePath]( const std::string& filePath ) {
        firmwareFilePath = filePath;
        ;
    } );
    parser.on( "help", "Show usage", [&parser]() {
        std::cout << parser.help_text() << std::endl;
        ::_exit( EXIT_SUCCESS );
    } );
    parser.parse( argc, argv );

#if !defined( __HUB_TYPE_H2__ ) && !defined( __HUB_TYPE_H2_SECURE__ ) && !defined( __HUB_TYPE_BEEWAVE__ )
    std::cout << "This utility is not intended to run on this platform" << std::endl;
    ::_exit( EXIT_SUCCESS );
#endif

    if( !firmwareFilePath.empty() ) {
        m_firmwareFilepath = firmwareFilePath;
    }

    bootInit();
    resetInit();
    return 0;
}

int ZigbeeFlasher::run() {
    auto result = m_dbClient->findOne( ZigbeeFlasherDBName, ZigbeeFirmwareIdObj, nullptr );

    bool updateNeeded = false;
    if( !validateDbRecord( result ) || checkUpdateNeeded( result ) ) {
        updateNeeded = true;
    }

    int updateResult = Result::UNDEFINED;
    if( updateNeeded ) {
        std::cout << "Firmware file: " << ZigbeeFirmwareFolder << "/"
                  << filesystem::getSymlinkValue( ZigbeeFirmwareFilepath ) << std::endl;
        updateResult = updateFirmware();
        if( updateResult == Result::SUCCESS ) {
            updateDbRecord();
        }
    }
    else {
        std::cout << "Zigbee NCP image is up to date" << std::endl;
    }

    return updateResult;
}

int ZigbeeFlasher::updateFirmware() {
    enterBootMode();
    bool bootPromtReceived = waitForBootloaderPrompt();
    if( !bootPromtReceived ) {
        std::cerr << "Bootloader prompt was not received" << std::endl;
        return Result::BOOTPROMPT_MISSING;
    }
    sendLoadFirmwareOption();
    auto sendResult = m_xmodemSender.sendFile( m_firmwareFilepath );
    leaveBootMode();
    return sendResult;
}

void ZigbeeFlasher::enterBootMode() {
    bootAssert();
    resetCycle();
}

void ZigbeeFlasher::leaveBootMode() {
    bootDeAssert();
    resetCycle();
}

bool ZigbeeFlasher::waitForBootloaderPrompt() {
    static constexpr size_t timeoutSeconds = 2;
    static constexpr size_t retriesMax = 5;
    auto startTime = TimeUtils::getMonotonicTime();
    size_t retriesCount = 0;

    std::cout << "Waiting for bootloader prompt..." << std::endl;
    while( !m_exitSignalRef && retriesCount < retriesMax ) {
        uint8_t dataRead;
        auto dataCount = m_uart.read( (void*) &dataRead, 1 );
        if( dataCount > 0 ) {
            if( dataRead == '>' ) {
                std::cout << std::endl << "Bootloader prompt received..." << std::endl;
                return true;
            }
            else if(::isprint( dataRead ) || ::iscntrl( dataRead ) ) {
                std::cout << dataRead;
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );

        if( TimeUtils::getDiffMonotonicTime( startTime ) >= timeoutSeconds ) {
            startTime = TimeUtils::getMonotonicTime();
            ++retriesCount;
            std::cout << "Retrying " << retriesCount << "..." << std::endl;
            resetAssert();
            m_uart.flush();
            enterBootMode();
        }
    }
    return false;
}

void ZigbeeFlasher::sendLoadFirmwareOption() {
    m_uart.flush();
    m_uart.write( (void*) loadFirmwareOptionCommand.data(), loadFirmwareOptionCommand.size() );
}

bool ZigbeeFlasher::validateDbRecord( const Json& record ) {
    return ( !record.empty() && !record[ "firmware_info" ].empty() &&
             !record[ "firmware_info" ][ "sha256sum" ].empty() );
}

bool ZigbeeFlasher::checkUpdateNeeded( const Json& record ) {
    const auto& shaSumString = record[ "firmware_info" ][ "sha256sum" ];
    const auto shaSumResult = calculateFileSha256Sum( m_firmwareFilepath );

    return ( shaSumString != shaSumResult );
}

void ZigbeeFlasher::updateDbRecord() {
    Json firmwareInfo = JSON_EMPTY_OBJ;
    firmwareInfo[ "file" ] = filesystem::getSymlinkValue( m_firmwareFilepath );
    firmwareInfo[ "size" ] = filesystem::getFileSize( m_firmwareFilepath );
    firmwareInfo[ "sha256sum" ] = calculateFileSha256Sum( m_firmwareFilepath );
    Json record = JSON( ZigbeeFirmwareIdObj << JSON( "firmware_info" << firmwareInfo ) );
    m_dbClient->update( ZigbeeFlasherDBName, JSON_EMPTY_OBJ, record, true );
}

std::string ZigbeeFlasher::calculateFileSha256Sum( const std::string& filepath ) const {
    std::ifstream ifs( m_firmwareFilepath );
    std::stringstream sstream;
    sstream << ifs.rdbuf();
    return SslUtils::sha256( sstream.str() );
}

}  // namespace ZigbeeFlasher
}  // namespace HubPlatform
