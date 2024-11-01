#include "luad/modules/network/network_manager.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "luad/modules/network/dhcp/dhcp.h"
#include "luad/modules/network/dhcp/response_builder.h"

#include <gtest/gtest.h>

#include <memory>
#include <fstream>
#include <iterator>

/*

TESTPLAN

DONE    1. DHCP report creation tests from binary dumps:
DONE        1.1 Make report for DHCPDISCOVERY command
DONE        1.2 Make report for DHCPREQUEST command
DONE        1.3 Make report for DHCPOFFER command
DONE        1.4 Make report for DHCPACK command
DONE    2. Check reports built by binary dumps from config.json, compare with expected report from config.json.

*/

namespace HubPlatform {
namespace UT {

using namespace LuaNetwork;
using namespace LuaNetwork::Dhcp;

class DhcpTests : public ::testing::Test {
public:
    using Bytes = std::vector< unsigned char >;
    using Strings = std::map< unsigned, std::string >;
    using AllHeaders = DhcpPacketUnpack::AllHeaders;
    using AllHeadersPtr = DhcpPacketUnpack::AllHeadersPtr;

public:
    void SetUp() override;

    DhcpTests& setDumpFamily( const std::string& dumpFamily );

    DhcpTests& setDumps( Dhcp::ECommand op, const std::string& relativeName );

    static Bytes readEthernetDump( const std::string& fileNameName );

    AllHeadersPtr fetchHeaders( Dhcp::ECommand op );

    DhcpTests::AllHeadersPtr fetchHeaders( const std::string& fileName );

    void fetchAllData( const std::string& fileName, DhcpTests::AllHeadersPtr& headersPtr, DhcpPacketData& packetData );
    void fetchAllData( Dhcp::ECommand op, AllHeadersPtr& headersPtr, DhcpPacketData& packetData );

    void checkDiscoverySchema( const Json& params ) const;

    void checkRequestSchema( const Json& params ) const;

    void checkOfferAckSchema( const Json& params ) const;

    Json testFormatParams( DhcpPacket& packet, DhcpPacketData& packetData );

    Json testFormatFullResponse( const AllHeaders& headers );

    Json testHeader( Json& header );

protected:
    std::string m_dumpFamilyName;
    Strings m_opFiles;
    Json m_testConfig;
};

void DhcpTests::SetUp() {
    // should end with '/'
    // DONE: Find a way not to set absolute path
    std::string theFile = __FILE__;
    auto lastPos = theFile.find_last_of( "/", std::string::npos );
    std::string packetsPath = theFile.substr( 0, lastPos ) + "/resources/";
    // make path to ./resources/ dir
    setDumpFamily( packetsPath );

    setDumps( Dhcp::DHCPDISCOVERY, "rawsock.dhcp-discovery.packet" );
    setDumps( Dhcp::DHCPREQUEST, "rawsock.dhcp-request.packet" );
    setDumps( Dhcp::DHCPOFFER, "rawsock.dhcp-offer.packet" );
    setDumps( Dhcp::DHCPACK, "rawsock.dhcp-ack.packet" );

    {
        std::string configFile = packetsPath + "config.json";

        std::ifstream input( configFile );
        try {
            m_testConfig = Json::parse( input );
            ASSERT_FALSE( m_testConfig.empty() ) << "DhcpTest config is empty: " << configFile;
        }
        catch( std::exception const& e ) {
            Log::error( "DhcpTests: Could not parse DHCP tests config file '" + configFile + "'" );
        }
    }
}

DhcpTests& DhcpTests::setDumpFamily( const std::string& dumpFamily ) {
    m_dumpFamilyName = dumpFamily;
    return *this;
}

DhcpTests& DhcpTests::setDumps( Dhcp::ECommand op, const std::string& fileName ) {
    std::string fullPath = m_dumpFamilyName + fileName;
    m_opFiles[ op ] = fullPath;
    return *this;
}

/** Read a binary file by name. Throw runtime_error on fail. */
DhcpTests::Bytes DhcpTests::readEthernetDump( const std::string& fileName ) {
    std::ifstream input( fileName, std::ios::binary );
    Bytes buffer( std::istreambuf_iterator< char >( input ), {} );

    return buffer;
}

DhcpTests::AllHeadersPtr DhcpTests::fetchHeaders( const std::string& fileName ) {
    Bytes commandDump;
    EXPECT_NO_THROW( commandDump = readEthernetDump( fileName ) );
    EXPECT_FALSE( commandDump.empty() ) << "Packet from a file: " + fileName;

    DhcpPacketUnpack unpacker;
    DhcpPacketUnpack::AllHeadersPtr headersPtr;
    EXPECT_NO_THROW( headersPtr = unpacker.networkPacketUnpack( commandDump.data(), commandDump.size() ) )
        << "Should not be empty";
    EXPECT_NE( headersPtr, nullptr );

    return headersPtr;
}

DhcpTests::AllHeadersPtr DhcpTests::fetchHeaders( Dhcp::ECommand op ) {
    auto fileName = m_opFiles[ op ];

    return fetchHeaders( fileName );
}

void DhcpTests::fetchAllData( Dhcp::ECommand op, DhcpTests::AllHeadersPtr& headersPtr, DhcpPacketData& packetData ) {
    std::string fileName = m_opFiles[ op ];
    ASSERT_EQ( fileName.empty(), false );  //  << "DHCP Packet file should have name, op: " << std::to_string(op) ;
    headersPtr = fetchHeaders( fileName );

    DhcpPacket rawPacket;
    std::tie( std::ignore, std::ignore, std::ignore, rawPacket ) = *headersPtr;

    EXPECT_NO_THROW( packetData = DhcpPacketUnpack::deserialize( rawPacket ) ) << "Packet should not be empty";
}

void DhcpTests::fetchAllData( const std::string& fileName,
                              DhcpTests::AllHeadersPtr& headersPtr,
                              DhcpPacketData& packetData ) {
    headersPtr = fetchHeaders( fileName );

    DhcpPacket rawPacket;
    std::tie( std::ignore, std::ignore, std::ignore, rawPacket ) = *headersPtr;

    EXPECT_NO_THROW( packetData = DhcpPacketUnpack::deserialize( rawPacket ) ) << "Packet should not be empty";
}

Json DhcpTests::testFormatParams( DhcpPacket& packet, DhcpPacketData& packetData ) {
    DhcpResponseBuilder builder( nullptr, nullptr, packet );
    Json params = builder.formatParams( packet, packetData );

    EXPECT_FALSE( params.empty() );
    auto dump = params.dump();
    int messageType = packetData.getNumber( Dhcp::MESSAGE_TYPE );
    printf( "DBG: params for response type %d, message type: %d: %s\n", (int) packet.op, messageType, dump.c_str() );
    return params;
}

Json DhcpTests::testHeader( Json& header ) {
    EXPECT_TRUE( header.count( Fields::clientAddress ) > 0 );
    EXPECT_TRUE( header.count( Fields::destinationAddress ) > 0 );
    EXPECT_TRUE( header.count( Fields::ipSource ) > 0 );
    EXPECT_TRUE( header.count( Fields::ipDestination ) > 0 );
    return header;
}

Json DhcpTests::testFormatFullResponse( const AllHeaders& headers ) {
    ethhdr eth;
    iphdr ip;
    DhcpPacket packet;

    std::tie( eth, ip, std::ignore, packet ) = headers;
    DhcpResponseBuilder builder( &eth, &ip, packet );
    Json response = builder.buildResponse();
    auto dump = response.dump();
    Log::debug( cc::warn( "Full response : " ) + cc::j( response ) );

    testHeader( response );
    return response;
}

void DhcpTests::checkDiscoverySchema( const Json& params ) const {
    ASSERT_TRUE( params.is_object() );
    // optional field
    EXPECT_GE( params.count( Fields::hostname ), 0 );
}

void DhcpTests::checkRequestSchema( const Json& params ) const {
    // Check required fields list here
    EXPECT_TRUE( params.count( Fields::ip ) > 0 );       // required
    EXPECT_TRUE( params.count( Fields::gateway ) > 0 );  // required
    if( params.count( Fields::hostname ) > 0 )
        EXPECT_FALSE( json_get_str( params, Fields::hostname ).empty() );  // optional
    if( params.count( Fields::ntp ) > 0 ) {
        EXPECT_TRUE( params[ Fields::ntp ].is_array() );  // optional
    }
}

void DhcpTests::checkOfferAckSchema( const Json& params ) const {
    EXPECT_TRUE( params.count( Fields::ip ) > 0 );           // required
    EXPECT_TRUE( params.count( Fields::mask ) > 0 );         // required
    EXPECT_TRUE( params.count( Fields::gateway ) > 0 );      // required
    EXPECT_TRUE( params.count( Fields::leaseTime ) > 0 );    // required
    EXPECT_TRUE( params[ Fields::leaseTime ].is_number() );  // required
    EXPECT_TRUE( params.count( Fields::dhcp ) > 0 );         // required
    EXPECT_TRUE( params.count( Fields::domainName ) > 0 );   // required
    if( params.count( Fields::dns ) > 0 ) {
        EXPECT_TRUE( params[ Fields::dns ].is_array() );  // required
    }
    if( params.count( Fields::ntp ) > 0 ) {
        EXPECT_TRUE( params[ Fields::ntp ].is_array() );  // optional
    }
}

TEST_F( DhcpTests, commandDhcpDiscovery /* 1.1 */ ) {
    AllHeadersPtr headersPtr;
    DhcpPacketData packetData;
    fetchAllData( Dhcp::DHCPDISCOVERY, headersPtr, packetData );

    ASSERT_FALSE( headersPtr == nullptr );
    Json params, response = testFormatFullResponse( *headersPtr );
    EXPECT_TRUE( json_get_obj( response, Fields::params, params ) );

    checkDiscoverySchema( params );
}

TEST_F( DhcpTests, commandDhcpRequest /* 1.2 */ ) {
    AllHeadersPtr headersPtr;
    DhcpPacketData packetData;
    fetchAllData( Dhcp::DHCPREQUEST, headersPtr, packetData );

    ASSERT_FALSE( headersPtr == nullptr );
    Json params, response = testFormatFullResponse( *headersPtr );
    EXPECT_TRUE( json_get_obj( response, Fields::params, params ) );

    checkRequestSchema( params );
}

TEST_F( DhcpTests, commandDhcpOffer /* 1.3 */ ) {
    AllHeadersPtr headersPtr;
    DhcpPacketData packetData;
    fetchAllData( Dhcp::DHCPOFFER, headersPtr, packetData );

    ASSERT_FALSE( headersPtr == nullptr );
    Json params, response = testFormatFullResponse( *headersPtr );
    EXPECT_TRUE( json_get_obj( response, Fields::params, params ) );

    checkOfferAckSchema( params );
}

TEST_F( DhcpTests, commandDhcpAck /* 1.4 */ ) {
    AllHeadersPtr headersPtr;
    DhcpPacketData packetData;
    fetchAllData( Dhcp::DHCPACK, headersPtr, packetData );

    ASSERT_FALSE( headersPtr == nullptr );
    Json params, response = testFormatFullResponse( *headersPtr );
    EXPECT_TRUE( json_get_obj( response, Fields::params, params ) );

    checkOfferAckSchema( params );
}

TEST_F( DhcpTests, anyFromConfig /* 2 */ ) {
    ASSERT_FALSE( m_testConfig.empty() );
    ASSERT_TRUE( m_testConfig.is_object() );
    Json files;
    ASSERT_TRUE( json_get_obj( m_testConfig, "files", files ) );
    ASSERT_FALSE( files.empty() );

    for( auto jsIt = files.begin(); jsIt != files.end(); jsIt++ ) {
        const auto& test = jsIt;

        auto fileName = test.key();
        const Json& testConfig = test.value();
        auto command = json_get_str( testConfig, "command" );
        Json expectedResponse;
        ASSERT_TRUE( json_get_obj( testConfig, "response", expectedResponse ) );

        AllHeadersPtr headersPtr;
        DhcpPacketData packetData;

        std::string testFile = m_dumpFamilyName + fileName;
        fetchAllData( testFile, headersPtr, packetData );

        ASSERT_NE( headersPtr, nullptr ) << "Empty headers for file '" << testFile << "'";
        Json params, response = testFormatFullResponse( *headersPtr );
        EXPECT_TRUE( json_get_obj( response, Fields::params, params ) )
            << "Empty response.params from '" << testFile << "'";

        DhcpPacket packet;
        std::tie( std::ignore, std::ignore, std::ignore, packet ) = *headersPtr;

        switch( packetData.messageType() ) {
        case Dhcp::DHCPDISCOVERY:
            checkDiscoverySchema( params );
            break;
        case Dhcp::DHCPREQUEST:
            checkRequestSchema( params );
            break;
        case Dhcp::DHCPOFFER:
        case Dhcp::DHCPACK:
            checkOfferAckSchema( params );
            break;
        }

        ASSERT_EQ( response, expectedResponse );
    }
}

}  // namespace UT
}  // namespace HubPlatform
