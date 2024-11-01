#include "zwave/zwave_addon.h"
#include "zwave/zwave_client.h"
#include "zwave/resources/zwave_resources.h"
#include "zwave/znet.h"
#include "zwave/znet_factory.h"

#include "zwave/tests/mocks/znet_mock.h"

#include "bus/base_bus.h"
#include "service_interfaces/bus_wrapper.h"
#include "plugin_events_subscriber/plugin_events_subscriber.h"
#include "database_client/fake_db/fake_db_client.h"
#include "common_utils/cpp14.h"
#include "test_utils/test_utils.h"
#include "database_client/dbqueryparser.h"
#include "provision_data_utils/provision_data_manager.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/firmware_info_helper.h"
#include "zwave/anti_theft_helper.h"

#include "test_utils/temporary_folder_wrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sp.h>

#include <type_traits>
#include <fstream>

namespace HubPlatform {
namespace Zwave {
namespace UT {

using namespace ::testing;

class TestBus final : public BaseBus {
public:
    TestBus( const std::string& privateName, const RxCallBack& rxCallBack, bool silent, int connectionAttempts )
    : BaseBus( privateName, rxCallBack ) {}

    void start() override {}

    void stop() override {}

    void join( const char* /*group*/ ) override {}

    bool multicast( const char* group, int msgClass, const std::string& msg ) override {
        bus_rx_data_t data;
        memset( &data, 0, sizeof( bus_rx_data_t ) );
        strcpy( data.sender, getPrivateName().c_str() );
        strcpy( data.msg, msg.c_str() );
        data.msg_size = msg.size();
        data.num_groups = 1;
        strcpy( data.target_groups[ 0 ], group );
        data.msg_type = RESPONSE;
        data.service_type = REGULAR_MESS;
        getRxCallback()( &data );
        return true;
    }

    bool isRegularMessage( int msgType ) const override {
        return true;
    }
};

class ZwaveAddonTestSuite : public Test {
public:
    ZwaveAddonTestSuite() {
        HubPlatform::Environment::setProvisionDataFolder( m_tmpDir.getPath() );
    }

    void zwaveNodeAdd( const uint8_t nodeId ) {
        getAddon().interviewDone( nodeId );
    }

    void zwaveNodeRemove( const uint8_t nodeId ) {
        getAddon().nodeRemove( nodeId );
    }

    void zwaveNodeCreate( const uint8_t nodeId ) {
        getAddon().nodeCreate( nodeId );
    }

    void setEnvironment( const char* region, const char* hubType ) {
        ProvisionDataUtils::ProvisionData data = HubPlatform::ProvisionDataUtils::getManager().getProvisionData();
        ++data.configVersion;
        data.zwaveRegion = region;

        HubPlatform::ProvisionDataUtils::getManager().update( data );

        HubInfoHelper::setExplicitHubType( hubType );
    }

public:
    void SetUp() override {
        m_addon = std::unique_ptr< ZwaveAddon >(
            new ZwaveAddon( "test",
                            "",
                            "",
                            std::make_unique< Services::BusWrapper >( Services::BusWrapper::BusTypeTag< TestBus >{} ),
                            std::make_unique< HubPlatform::UT::FakeDbClient >(),
                            std::make_unique< ZWaveStick >(),
                            std::make_unique< ZNetFactory >( ZNetFactory::ZNetTypeTag< ZNetMock >{} ) ) );
        m_addon->setTestMode();
        cleanDB();
    }

    ZwaveAddon& getAddon() {
        return *m_addon;
    }

    void cleanDB() {
        getAddon().getDBClient().clean();
    }

    std::vector< Json > getAllFormTable( const char* const tableName ) {
        return getAddon().getDBClient().findAll( tableName, JSON_EMPTY_OBJ, nullptr );
    }

protected:
    std::unique_ptr< ZwaveAddon > m_addon;
    HubPlatform::Tests::TemporaryFolderWrapper m_tmpDir;
};

class ZwaveAddonTestSuiteManual : public ZwaveAddonTestSuite {
protected:
    void SetUp() override {
        m_addon = std::make_unique< ZwaveAddon >( "/dev/ttyACM0", "znet_store.bin" );
        cleanDB();
    }
};

// Note: It is manual test. Can be runed only in dev environment.
TEST_F( ZwaveAddonTestSuiteManual, DISABLED_testStartInclude ) {
    getAddon().addTask( [this]() { getAddon().startInclude( {} ); } );
    getAddon().addTask(
        [this] {
            const auto result = getAllFormTable( Resources::DB::TableNames::Devices );
            for( const auto& j : result ) {
                std::cout << j << std::endl;
            }
            if( !result.empty() ) {
                getAddon().stop();
            }
        },
        true );

    getAddon().run();
}

// Note: It is manual test. Can be runed only in dev environment.
TEST_F( ZwaveAddonTestSuiteManual, DISABLED_testStartIncludeFromClient ) {
    ZwaveClient client( getAddon() );

    getAddon().addTask( [&client]() { client.startInclude( "" ); } );
    getAddon().addTask(
        [this] {
            const auto result = getAllFormTable( Resources::DB::TableNames::Devices );
            for( const auto& j : result ) {
                std::cout << j << std::endl;
            }
            if( !result.empty() ) {
                getAddon().stop();
            }
        },
        true );

    getAddon().run();
}

// Note: It is manual test. Can be runed only in dev environment.
TEST_F( ZwaveAddonTestSuiteManual, DISABLED_testStartExclude ) {
    getAddon().addTask( [this]() { getAddon().startExclude(); } );

    getAddon().run();
}

TEST_F( ZwaveAddonTestSuite, testDeviceAdded ) {
    getAddon().addTask( [this] {
        zwaveNodeCreate( 1U );
        zwaveNodeAdd( 1U );

        getAddon().addTask( [this] {
            getAddon().addTask( [this]() { getAddon().stop(); } );
            const auto result = getAllFormTable( Resources::DB::TableNames::Devices );
            ASSERT_EQ( 1U, result.size() );

            std::string id;
            EXPECT_TRUE( db::getId( result.front(), id ) );
            EXPECT_EQ( "1", id );
        } );
    } );

    getAddon().run();
}

TEST_F( ZwaveAddonTestSuite, testDeviceRemovedEvent ) {
    getAddon().addTask( [this] {
        zwaveNodeCreate( 1U );
        zwaveNodeAdd( 1U );
        zwaveNodeRemove( 1U );

        getAddon().addTask( [this] {
            getAddon().addTask( [this]() { getAddon().stop(); } );
            const auto result = getAllFormTable( Resources::DB::TableNames::Devices );
            EXPECT_EQ( 0U, result.size() );
        } );
    } );

    getAddon().run();
}

TEST_F( ZwaveAddonTestSuite, testClientGetNode ) {
    const auto nodeId = 5U;

    getAddon().addTask( [this, nodeId] {
        zwaveNodeCreate( nodeId );
        zwaveNodeAdd( nodeId );

        getAddon().addTask( [this, nodeId] {
            getAddon().addTask( [this]() { getAddon().stop(); } );

            ZwaveClient client( getAddon() );
            auto result = client.getNodeAsJson( nodeId );

            EXPECT_EQ( std::to_string( nodeId ), result[ Zwave::Resources::Fields::Node::Id ] );
        } );
    } );

    getAddon().run();
}

TEST_F( ZwaveAddonTestSuite, testFillZwaveVersion ) {
    uint8_t buff[ sizeof( znet_version_report_t ) + 10 ];
    memset( buff, 0, sizeof( znet_version_report_t ) + 10 );
    auto report = reinterpret_cast< znet_version_report_t* >( buff );

    {
        setEnvironment( "eu", "h2.1" );
        getAddon().fillVersion( *report );

        EXPECT_EQ( report->lib_type, 0 );
        EXPECT_EQ( report->hard_ver, 0x02 );
        EXPECT_EQ( report->firms_count, 2 );

        EXPECT_EQ( report->firms[ 0 ].ver, 0x07 );
        EXPECT_EQ( report->firms[ 0 ].sub_ver, 0x0F );
        EXPECT_EQ( report->firms[ 1 ].ver, FirmwareInfoHelper::getFirmwareVersion().getMajor() );
        EXPECT_EQ( report->firms[ 1 ].sub_ver, FirmwareInfoHelper::getFirmwareVersion().getMinor() );
    }

    {
        setEnvironment( "us", "h2.1" );
        getAddon().fillVersion( *report );

        EXPECT_EQ( report->lib_type, 0 );
        EXPECT_EQ( report->hard_ver, 0x01 );
        EXPECT_EQ( report->firms_count, 2 );

        EXPECT_EQ( report->firms[ 0 ].ver, 0x07 );
        EXPECT_EQ( report->firms[ 0 ].sub_ver, 0x0F );
    }

    {
        setEnvironment( "", "h2.1" );
        getAddon().fillVersion( *report );

        EXPECT_EQ( report->lib_type, 0 );
        EXPECT_EQ( report->hard_ver, 0x00 );
        EXPECT_EQ( report->firms_count, 2 );

        EXPECT_EQ( report->firms[ 0 ].ver, 0x07 );
        EXPECT_EQ( report->firms[ 0 ].sub_ver, 0x0F );
    }
}

TEST_F( ZwaveAddonTestSuite, testFillZwavePlusInfo ) {
    znet_zwaveplus_info_report_t report{};
    getAddon().fillZWavePlusInfo( report );

    EXPECT_EQ( report.role_type, 0 );
    EXPECT_EQ( report.node_type, 0 );

    EXPECT_EQ( report.installer_icon, 0x0500 );
    EXPECT_EQ( report.user_icon, 0x0500 );
}

TEST_F( ZwaveAddonTestSuite, testFillManufacturerSpecific ) {
    for( auto region : { "eu", "EU" } ) {
        setEnvironment( region, "h2.1" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x04 );
        EXPECT_EQ( report.product_id, 0x02 );
    }

    for( auto region : { "us", "US" } ) {
        setEnvironment( region, "h2.1" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x04 );
        EXPECT_EQ( report.product_id, 0x01 );
    }

    for( auto region : { "unknown", "" } ) {
        setEnvironment( region, "h2.1" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x04 );
        EXPECT_EQ( report.product_id, 0x00 );
    }

    {
        setEnvironment( "eu", "h2_secure.1" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x05 );
        EXPECT_EQ( report.product_id, 0x02 );
    }

    {
        setEnvironment( "eu", "g150" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x06 );
        EXPECT_EQ( report.product_id, 0x02 );
    }

    {
        setEnvironment( "eu", "beewave.1" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x07 );
        EXPECT_EQ( report.product_id, 0x02 );
    }

    {
        setEnvironment( "eu", "" );

        znet_manufacturer_specific_report_t report{};
        getAddon().fillManufacturerSpecific( report );

        EXPECT_EQ( report.manufacturer_id, 0x025B );
        EXPECT_EQ( report.product_type, 0x00 );
        EXPECT_EQ( report.product_id, 0x02 );
    }
}

TEST_F( ZwaveAddonTestSuite, testFillFirmwareUpdateInfo ) {
    {
        setEnvironment( "us", "h2_secure.1" );

        uint16_t buff[ 20 ] = {
            0,
        };
        znet_firmware_md_report_v1_t dataV1{};
        znet_firmware_md_report_v3_t dataV3{};
        dataV3.additional_firmware = buff;
        znet_firmware_md_report_v5_t dataV5{};
        getAddon().fillFirmwareUpdateInfo( dataV1, dataV3, dataV5 );

        EXPECT_EQ( dataV1.manufacturer_id, 0x025B );
        EXPECT_EQ( dataV1.firmware_id, 0x0701 );
        EXPECT_EQ( dataV1.checksum, 0 );

        EXPECT_FALSE( dataV3.firmware_upgradable );
        EXPECT_EQ( dataV3.max_fragment_size, 0 );
        EXPECT_EQ( dataV3.additional_firmware_count, 1 );
        EXPECT_EQ( dataV3.additional_firmware[ 0 ], 5 );

        EXPECT_EQ( dataV5.hardware_revision, 1 );
    }

    {
        setEnvironment( "eu", "h2_secure.1" );

        uint16_t buff[ 20 ] = {
            0,
        };
        znet_firmware_md_report_v1_t dataV1{};
        znet_firmware_md_report_v3_t dataV3{};
        dataV3.additional_firmware = buff;
        znet_firmware_md_report_v5_t dataV5{};
        getAddon().fillFirmwareUpdateInfo( dataV1, dataV3, dataV5 );

        EXPECT_EQ( dataV1.manufacturer_id, 0x025B );
        EXPECT_EQ( dataV1.firmware_id, 0x0702 );
        EXPECT_EQ( dataV1.checksum, 0 );

        EXPECT_FALSE( dataV3.firmware_upgradable );
        EXPECT_EQ( dataV3.max_fragment_size, 0 );
        EXPECT_EQ( dataV3.additional_firmware_count, 1 );
        EXPECT_EQ( dataV3.additional_firmware[ 0 ], 5 );

        EXPECT_EQ( dataV5.hardware_revision, 2 );
    }

    {
        setEnvironment( "us", "h2.1" );

        uint16_t buff[ 20 ] = {
            0,
        };
        znet_firmware_md_report_v1_t dataV1{};
        znet_firmware_md_report_v3_t dataV3{};
        dataV3.additional_firmware = buff;
        znet_firmware_md_report_v5_t dataV5{};
        getAddon().fillFirmwareUpdateInfo( dataV1, dataV3, dataV5 );

        EXPECT_EQ( dataV1.manufacturer_id, 0x025B );
        EXPECT_EQ( dataV1.firmware_id, 0x0701 );
        EXPECT_EQ( dataV1.checksum, 0 );

        EXPECT_FALSE( dataV3.firmware_upgradable );
        EXPECT_EQ( dataV3.max_fragment_size, 0 );
        EXPECT_EQ( dataV3.additional_firmware_count, 1 );
        EXPECT_EQ( dataV3.additional_firmware[ 0 ], 4 );

        EXPECT_EQ( dataV5.hardware_revision, 1 );
    }

    {
        setEnvironment( "us", "g150" );

        uint16_t buff[ 20 ] = {
            0,
        };
        znet_firmware_md_report_v1_t dataV1{};
        znet_firmware_md_report_v3_t dataV3{};
        dataV3.additional_firmware = buff;
        znet_firmware_md_report_v5_t dataV5{};
        getAddon().fillFirmwareUpdateInfo( dataV1, dataV3, dataV5 );

        EXPECT_EQ( dataV1.manufacturer_id, 0x025B );
        EXPECT_EQ( dataV1.firmware_id, 0x0701 );
        EXPECT_EQ( dataV1.checksum, 0 );

        EXPECT_FALSE( dataV3.firmware_upgradable );
        EXPECT_EQ( dataV3.max_fragment_size, 0 );
        EXPECT_EQ( dataV3.additional_firmware_count, 1 );
        EXPECT_EQ( dataV3.additional_firmware[ 0 ], 6 );

        EXPECT_EQ( dataV5.hardware_revision, 1 );
    }

    {
        setEnvironment( "eu", "beewave.1" );

        uint16_t buff[ 20 ] = {
            0,
        };
        znet_firmware_md_report_v1_t dataV1{};
        znet_firmware_md_report_v3_t dataV3{};
        dataV3.additional_firmware = buff;
        znet_firmware_md_report_v5_t dataV5{};
        getAddon().fillFirmwareUpdateInfo( dataV1, dataV3, dataV5 );

        EXPECT_EQ( dataV1.manufacturer_id, 0x025B );
        EXPECT_EQ( dataV1.firmware_id, 0x0702 );
        EXPECT_EQ( dataV1.checksum, 0 );

        EXPECT_FALSE( dataV3.firmware_upgradable );
        EXPECT_EQ( dataV3.max_fragment_size, 0 );
        EXPECT_EQ( dataV3.additional_firmware_count, 1 );
        EXPECT_EQ( dataV3.additional_firmware[ 0 ], 7 );

        EXPECT_EQ( dataV5.hardware_revision, 2 );
    }
}

TEST_F( ZwaveAddonTestSuite, testFillLocale ) {
    {
        setEnvironment( "us", "h2_secure.1" );

        std::string locale;
        getAddon().fillFirmwareLocale( locale );
        EXPECT_EQ( locale, "US" );
    }

    {
        setEnvironment( "eu", "h2_secure.1" );

        std::string locale;
        getAddon().fillFirmwareLocale( locale );
        EXPECT_EQ( locale, "EU" );
    }

    {
        setEnvironment( "us", "h2.1" );

        std::string locale;
        getAddon().fillFirmwareLocale( locale );
        EXPECT_EQ( locale, "US" );
    }

    {
        setEnvironment( "us", "g150" );

        std::string locale;
        getAddon().fillFirmwareLocale( locale );
        EXPECT_EQ( locale, "US" );
    }

    {
        setEnvironment( "eu", "beewave.1" );

        std::string locale;
        getAddon().fillFirmwareLocale( locale );
        EXPECT_EQ( locale, "EU" );
    }
}

TEST_F( ZwaveAddonTestSuite, antiTheftHelper ) {
    EXPECT_EQ( AntiTheftHelper::generateHint().size(), 10 );
    EXPECT_EQ( AntiTheftHelper::getMagicCode( {}, "user string" ), AntiTheftHelper::BytesOpt() );
    EXPECT_EQ( AntiTheftHelper::getMagicCode( { 0x14, 0x67, 0x87, 0xA5, 0x76, 0x98, 0xCC, 0x90, 0x7D }, "user string" ),
               AntiTheftHelper::BytesOpt() );
    EXPECT_EQ( AntiTheftHelper::getMagicCode( { 0x14, 0x67, 0x87, 0xA5, 0x76, 0x98, 0xCC, 0x90, 0x7D, 0xE4, 0x60 },
                                              "user string" ),
               AntiTheftHelper::BytesOpt() );
    EXPECT_EQ(
        AntiTheftHelper::getMagicCode( { 0x14, 0x67, 0x87, 0xA5, 0x76, 0x98, 0xCC, 0x90, 0x7D, 0x02 }, "user string" ),
        AntiTheftHelper::BytesOpt() );

    EXPECT_EQ(
        AntiTheftHelper::getMagicCode( { 0x14, 0x67, 0x87, 0xA5, 0x76, 0x98, 0xCC, 0x90, 0x7D, 0x01 }, "user string" ),
        AntiTheftHelper::BytesOpt(
            AntiTheftHelper::Bytes( { 0x7d, 0x2a, 0x1b, 0xd4, 0x86, 0xba, 0x98, 0x20, 0xf5, 0xa5 } ) ) );
}

TEST_F( ZwaveAddonTestSuite, testClientSchedule ) {
    const auto nodeId = 2U;
    /*
        getAddon().addTask( [this, nodeId] {
            zwaveNodeCreate( nodeId );
            //        zwaveNodeAdd( nodeId );
            Json jsonNode =
                JSON( "_id"
                      << "2"
                      << "base64Data"
                      << std::string(
                             "GgAAAAIdAsgAAwFABAEDAwADAAAAAEAAAAEAEiACAQBwBAEABQABAAEBAAAAAAACAAAAAQAAABYAAABDb25maWd1cmUgd"
                             "m9sdW1lIGxldmVsFgAAAENvbmZpZ3VyZSB2b2x1bWUgbGV2ZWwCAAEBAAAAAAABAAAAAAAAAA0AAABDb25maWd1cmUgbG"
                             "VkEgAAAEVuYWJsZS9kaXNhYmxlIGxlZAMAAQIAAAAAAEA4AAAAAAAAFAAAAFRpbWVkIGJhdHRlcnkgcmVwb3J0HgAAAEN"
                             "vbmZpZ3VyZSB0aW1lZCBiYXR0ZXJ5IHJlcG9ydAQAAQEAAAAAAAYAAAABAAAAHAAAAEFzc29jaWF0aW9uIEdyb3VwIDIg"
                             "U2V0dGluZ3MdAAAAQ29uZmlndXJlIHRoZSBCYXNpYyBTZXQgdmFsdWUFAAEBAQAAAAABAAAAAAAAABEAAABDaGVjayBma"
                             "W5nZXJwcmludCUAAABJcyBhbnkgZmluZ2VycHJpbnQgc3RvcmVkIGluIHRoZSBsb2NrXgIBigEBmAEBnwEBhQIBAAAAAA"
                             "AAAFkDAQIAAAAAAAIAAAACDwAAAE9uL09mZiBjb250cm9sACAAAQAAACABAQkAAABMaWZlbGluZQAAAQgAAACAA3AGWgF"
                             "iA3EFYwNjEGMKjgMBAgABAAAAAQEAAAABAAIAAAAFBXEIAQABAgYQAQIDBAUGCQsMDQ4PEhMWFwcBA2IEAQCDAgD/"
                             "YwIB+gACAAMAAQIDAAECBQAAAFMEAQEKAAADAAAAAQAAAAIAAAADAAAAAQAAAAAAAAAAAQEAAwEAGwEAEgEAAAEA"
    ) );

            getAddon().getDBClient().insert( Resources::DB::TableNames::Devices, jsonNode );

            getAddon().addTask( [this, nodeId] {
                getAddon().addTask( [this]() { getAddon().stop(); } );

    //                        znet_callbacks_t m_callbacks;
    //                        EXPECT_EQ( znet_init( &m_callbacks ), ZNET_ERR_OK );

                ZwaveClient client( getAddon() );

                uint8_t channelId;
                uint8_t scheduleId;
                common::optional< uint8_t > scheduleIdBlock;
                Json value =
                    JSON( "commands" << JSON_ARRAY( JSON_ARRAY( 5 << 6 )
                                                    << JSON_ARRAY( 7 << 8 << 9 << 1 ) << JSON_ARRAY( 6 << 3 << 2 ) )
                                     << "hourandminute" << JSON( "start_hour" << 10 << "start_minute" << 33 ) <<
    "options"
                                     << JSON( "duration" << 1 << "duration_type"
                                                         << "hours"
                                                         << "recurrence_mode"
                                                         << "days"
                                                         << "recurrence_offset" << 3 << "relative" << true
                                                         << "report_to_follow" << 0 )
                                     << "weekdays"
                                     << JSON_ARRAY( "monday"
                                                    << "friday" )
                                     << "yearday"
                                     << JSON( "start_day_of_month" << 30 << "start_month" << 11 << "start_year" << 2025
    ) );

//            printf( "  json = %s \n", value.dump().c_str() );
//
//            printf( "  jsonNode = %s \n", jsonNode.dump().c_str() );

                auto result = client.isScheduleStructureAcceptable( nodeId, channelId, scheduleId, scheduleIdBlock,
    value );

                EXPECT_EQ( result.first, false );

                scheduleIdBlock = 1;
                client.isScheduleStructureAcceptable( nodeId, channelId, scheduleId, scheduleIdBlock, value );
                EXPECT_EQ( result.first, true );

                printf( "  result = %d \n", (int) result.first );
                printf( "  result = %s \n", result.second.c_str() );
                //     EXPECT_NE( std::to_string( nodeId ), result[ Zwave::Resources::Fields::Node::Id ] );
            } );
        } );

        getAddon().run();
        */
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
