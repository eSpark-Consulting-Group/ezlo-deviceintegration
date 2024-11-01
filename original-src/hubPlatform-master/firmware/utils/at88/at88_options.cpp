#include "at88/at88_options.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "json_utils/json_utils.h"

#include <iostream>
#include <unistd.h>
#include <log_utils/ezlo_log.h>

At88Options::At88Options()
: m_jsonFormattedOutput( false )
, m_appendPrefix( false ) {}

void At88Options::processCliOptions( int argc, char** argv ) {
    m_parser.on( "Formatting\njson", "Performs output in JSON format", [&]() { m_jsonFormattedOutput = true; } )
        .on( "Formatting\nj", "Same as json", [&]() { m_jsonFormattedOutput = true; } )
        .on( "Formatting\nprefix",
             "Appends prefix before value ( e.g \"mac\", \"serial\" )",
             [&]() { m_appendPrefix = true; } )
        .on( "Formatting\np", "Same as prefix", [&]() { m_appendPrefix = true; } )
        .on( "Hardware\nmac", "Reads wired interface MAC address", [&]() { readEthernetMacAddress(); } )
        .on( "Hardware\nm", "Same as mac", [&]() { readEthernetMacAddress(); } )
        .on( "Hardware\nwmac", "Reads wireless interface MAC address", [&]() { readWifiMacAddress(); } )
        .on( "Hardware\nserial", "Reads hub serial", [&]() { readSerial(); } )
        .on( "Hardware\ns", "Same as serial", [&]() { readSerial(); } )
        .on( "Hardware\ntype", "Reads hub type", [&]() { readHubType(); } )
        .on( "Hardware\nt", "Same as type", [&]() { readHubType(); } )
        .on( "Hardware\nid", "Reads hub ID ( for now same as serial )", [&]() { readSerial(); } )
        .on( "Hardware\ncpu-sid", "Reads CPU serial ID ( unique chip serial number )", [&]() { readCpuSid(); } )
        .on( "Hardware\nunique-json",
             "Reads hub unique data",  // further used for provisioning
             [&]() { readUniqueHardwareJson(); } )
        .on( "Firmware\nversion", "Reads firmware version", [&]() { readFirmwareVersion(); } )
        .on( "Firmware\nv", "Same as version", [&]() { readFirmwareVersion(); } )
        .on( "Firmware\npk_device", "Reads pk_device value from controller information", [&]() { readPkDevice(); } )
        .on( "Firmware\nuuid", "Reads UUID value from controller information", [&]() { readUuid(); } )
        .on( "Formatting\nverbose",
             "Appends prefix before value ( e.g \"mac\", \"serial\" )",
             [&]() { Log::setLocalLogLevel( Log::PriorityLevel::INFO ); } )
        .on( "help", "System\nShow help info", [&]() {
            std::cout << m_parser.help_text() << "\n";
            ::_exit( 0 );
        } );

    m_parser.parse( argc, argv );
    ::_exit( 0 );
}

void At88Options::readEthernetMacAddress() {
    static const std::string prefix = "mac";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getMacAddress() ) :
                            printOut( prefix, HubInfoHelper::getMacAddress() );
}

void At88Options::readWifiMacAddress() {
    static const std::string prefix = "wmac";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getWifiMacAddress() ) :
                            printOut( prefix, HubInfoHelper::getWifiMacAddress() );
}

void At88Options::readSerial() {
    static const std::string prefix = "serial";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getSerial() ) :
                            printOut( prefix, HubInfoHelper::getSerial() );
}

void At88Options::readHubType() {
    static const std::string prefix = "hubtype";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getHubType() ) :
                            printOut( prefix, HubInfoHelper::getHubType() );
}

void At88Options::readCpuSid() {
    static const std::string prefix = "cpuSid";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getCpuSid() ) :
                            printOut( prefix, HubInfoHelper::getCpuSid() );
}

void At88Options::readPkDevice() {
    static const std::string prefix = "pkDevice";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getPkDevice() ) :
                            printOut( prefix, HubInfoHelper::getPkDevice() );
}

void At88Options::readUuid() {
    static const std::string prefix = "uuid";
    m_jsonFormattedOutput ? printOutJson( prefix, HubInfoHelper::getControllerUUID() ) :
                            printOut( prefix, HubInfoHelper::getControllerUUID() );
}

void At88Options::readFirmwareVersion() {
    static const std::string prefix = "version";
    m_jsonFormattedOutput ? printOutJson( prefix, FirmwareInfoHelper::getFirmwareVersionString() ) :
                            printOut( prefix, FirmwareInfoHelper::getFirmwareVersionString() );
}

void At88Options::readUniqueHardwareJson() {
    static const std::string prefix = "unique-json";
    Json jUnique = JSON( "hardware" << JSON( "mac_eth_0" << HubInfoHelper::getMacAddress() << "mac_wifi_0"
#if( defined( __HUB_TYPE_H2__ ) || defined( __HUB_TYPE_H2_SECURE__ ) )
                                                         << HubInfoHelper::getWifiMacAddress() << "cpu_serial_cortex_a7"
#endif
                                                         << HubInfoHelper::getCpuSid() ) );
    Json result = JSON( "id" << HubInfoHelper::getSerial() << "uuid" << HubInfoHelper::getControllerUUID() << "unique"
                             << jUnique );
    printOut( prefix, result.dump() );
}

void At88Options::printOut( const std::string& prefix, const std::string& value ) {
    std::cout << ( m_appendPrefix ? std::string( prefix ).append( ": " ) : "" ) << value << std::endl;
}

void At88Options::printOutJson( const std::string& prefix, const std::string& value ) {
    std::cout << ( m_appendPrefix ? std::string( prefix ).append( ": " ) : "" ) << JSON( prefix << value ) << std::endl;
}
