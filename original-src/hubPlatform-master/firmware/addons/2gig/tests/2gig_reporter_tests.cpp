
#include "2gig/client.h"
#include "2gig/2gig_reporter.h"
#include "2gig/2gig_devices_reachability_manager.h"

#include "fakes/fake_devices_repository.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

namespace HubPlatform {
namespace TwoGig {
namespace UT {

using namespace ::testing;

class FakeClient final : public ::TwoGig::Client {
public:
    FakeClient() {}
    virtual void onReset() override {}
    virtual void onNewDeviceDiscovered( uint32_t serial, ::TwoGig::DeviceType::Enum deviceType ) override {}
    virtual void onDeviceAdded( const ::TwoGig::DeviceDescriptor& descriptor ) override {}
    virtual void onDeviceRemoved( const std::vector< uint32_t >& txIds ) override {}
    virtual void onDeviceReachableChanged( uint32_t serial, bool reachable ) override {}
    virtual void onSmokeDetectorUpdated( uint32_t serial, ::TwoGig::SmokeDetectorStatus status ) override {}
    virtual void onPanicButtonUpdated( uint32_t serial, ::TwoGig::PanicButtonStatus status ) override {}
    virtual void onPirDetectorUpdated( uint32_t serial, ::TwoGig::PirDetectorStatus status ) override {}
    virtual void onFloodTemperatureUpdated( uint32_t serial, ::TwoGig::FloodTemperatureStatus status ) override {}
    virtual void onGlassBreakDetectorUpdated( uint32_t serial, ::TwoGig::GlassBreakStatus status ) override {}
    virtual void onDoorWindowSensorUpdated( uint32_t serial, ::TwoGig::DoorWindowSensorStatus status ) override {}
    virtual void onCoDetectorUpdated( uint32_t serial, ::TwoGig::CoDetectorStatus status ) override {}
    virtual void onTiltDetectorUpdated( uint32_t serial, ::TwoGig::TiltDetectorStatus status ) override {}
    virtual void onRemoteKeyFobUpdated( uint32_t serial, ::TwoGig::RemoteKeyFobStatus status ) override {}
    virtual void onPadUpdated( uint32_t serial, ::TwoGig::PadStatus status ) override {}
    virtual void onPinUpdated( uint32_t serial,
                               const std::string& pin,
                               ::TwoGig::PinCodeState::Enum pinState ) override {}
    virtual void onRepeaterUpdated( uint32_t serial, ::TwoGig::RepeaterStatus status ) override {}
    virtual void onHoneywellSensorUpdated( uint32_t serial, ::TwoGig::HoneywellSensorStatus status ) override {}
};

class TwoGigReporterTestSuite : public ::testing::Test {
public:
    TwoGigReporterTestSuite() {}

    void SetUp() override {
        m_devices.reset();
        m_devicesReachabilityManager.reset();
        m_twoGigReporter.reset();
    }

    void init( std::chrono::system_clock::duration unreachableInterval = std::chrono::milliseconds{ 50 } ) {
        m_devicesReachabilityManager =
            std::make_shared< DevicesReachabilityManagerImpl >( m_client, m_devices, unreachableInterval );
        m_twoGigReporter = std::make_unique< TwoGigReporter >( "test", m_devicesReachabilityManager );
    }

    void getNodeUpdateTimes( JsonObject& nodesInfo ) {
        JsonObject reportData;
        EXPECT_TRUE( parseJson( getTwoGigReporter().prepareReportData(), reportData ) );
        EXPECT_TRUE( json_get( reportData, { "service", "test", "2gig_node_update_times" }, nodesInfo ) );
    }

    void getNodeInfo( const JsonObject& nodesInfo, const std::string& key, bool& reachable, int64_t& lastUpdate ) {
        JsonObject nodeInfo;
        EXPECT_TRUE( json_get( nodesInfo, key, nodeInfo ) );
        reachable = json_get_bool( nodeInfo, "reachable" );
        lastUpdate = json_get_int64( nodeInfo, "last_update" );
    }

    FakeDevicesRepository& getDevices() {
        return m_devices;
    }

    int64_t getNowBySeconds() {
        return std::chrono::time_point_cast< std::chrono::seconds >( std::chrono::steady_clock::now() )
            .time_since_epoch()
            .count();
    }

    DevicesReachabilityManagerImpl& getReachabilityManager() {
        EXPECT_TRUE( m_devicesReachabilityManager.get() != nullptr );
        return *m_devicesReachabilityManager;
    }

    TwoGigReporter& getTwoGigReporter() {
        EXPECT_TRUE( m_twoGigReporter.get() != nullptr );
        return *m_twoGigReporter;
    }

    void waitInterval( int64_t interval = 60 ) {
        std::this_thread::sleep_for( std::chrono::milliseconds{ interval } );
    }

private:
    FakeClient m_client;
    FakeDevicesRepository m_devices;
    std::unique_ptr< TwoGigReporter > m_twoGigReporter;
    std::shared_ptr< DevicesReachabilityManagerImpl > m_devicesReachabilityManager;
};

TEST_F( TwoGigReporterTestSuite, noDevice ) {
    init();
    waitInterval();
    getReachabilityManager().checkAllDevicesReachableStates();
    JsonObject nodesInfo;
    getNodeUpdateTimes( nodesInfo );
    EXPECT_TRUE( nodesInfo.is_null() );
}

TEST_F( TwoGigReporterTestSuite, oneDevice ) {
    ino64_t initTime = getNowBySeconds();

    getDevices().addDevice( { 1 } );
    init();
    waitInterval();
    getReachabilityManager().checkAllDevicesReachableStates();

    JsonObject nodesInfo;
    int64_t lastUpdate;
    bool reachable;

    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 1 );
    getNodeInfo( nodesInfo, std::to_string( 1 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, initTime );
}

TEST_F( TwoGigReporterTestSuite, addMoreDevices ) {
    ino64_t initTime = getNowBySeconds();

    getDevices().addDevice( { 1 } );
    init();
    waitInterval();
    getReachabilityManager().checkAllDevicesReachableStates();
    getReachabilityManager().onDeviceAdded( { 3, 4 } );

    JsonObject nodesInfo;
    int64_t lastUpdate;
    bool reachable;

    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 3 );
    getNodeInfo( nodesInfo, std::to_string( 1 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, initTime );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, initTime );
    getNodeInfo( nodesInfo, std::to_string( 4 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, initTime );
}

TEST_F( TwoGigReporterTestSuite, dataReceived ) {
    ino64_t initTime = getNowBySeconds();
    getDevices().addDevice( { 1 } );
    init( std::chrono::milliseconds( 100 ) );

    ino64_t afterInitTime = getNowBySeconds();
    getReachabilityManager().checkAllDevicesReachableStates();

    JsonObject nodesInfo;
    int64_t lastUpdate;
    bool reachable;

    waitInterval( 90 );
    getReachabilityManager().onDeviceReceivedData( 3 );
    waitInterval( 20 );

    getReachabilityManager().checkAllDevicesReachableStates();
    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 2 );
    getNodeInfo( nodesInfo, std::to_string( 1 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, initTime );
    EXPECT_LE( lastUpdate, afterInitTime );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, initTime );
    EXPECT_GE( lastUpdate, afterInitTime );
}

TEST_F( TwoGigReporterTestSuite, checkDeviceTimeout ) {
    ino64_t initTime = getNowBySeconds();
    init();
    getReachabilityManager().checkAllDevicesReachableStates();
    waitInterval();
    getReachabilityManager().onDeviceReceivedData( 3 );
    getReachabilityManager().onDeviceReceivedData( 5 );

    JsonObject nodesInfo;
    int64_t lastUpdate;
    bool reachable;

    getReachabilityManager().checkAllDevicesReachableStates();
    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 2 );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, initTime );
    getNodeInfo( nodesInfo, std::to_string( 5 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, initTime );

    waitInterval( 40 );
    getReachabilityManager().onDeviceReceivedData( 5 );
    waitInterval( 20 );

    getReachabilityManager().checkAllDevicesReachableStates();
    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 2 );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, initTime );
    getNodeInfo( nodesInfo, std::to_string( 5 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, initTime );
}

TEST_F( TwoGigReporterTestSuite, removeSomeDevices ) {
    getDevices().addDevice( { 1, 4 } );
    ino64_t beforeInit = getNowBySeconds();
    init();
    waitInterval();
    getReachabilityManager().checkAllDevicesReachableStates();
    getReachabilityManager().onDeviceReceivedData( 1 );
    getReachabilityManager().onDeviceReceivedData( 2 );
    getReachabilityManager().onDeviceReceivedData( 3 );

    JsonObject nodesInfo;
    int64_t lastUpdate;
    bool reachable;

    getReachabilityManager().checkAllDevicesReachableStates();
    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 4 );
    getNodeInfo( nodesInfo, std::to_string( 1 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 2 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 4 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );

    getReachabilityManager().onDeviceRemoved( { 1, 2 } );

    getReachabilityManager().checkAllDevicesReachableStates();
    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 2 );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 4 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
}

TEST_F( TwoGigReporterTestSuite, removeAllDevices ) {
    getDevices().addDevice( { 1, 4 } );
    ino64_t beforeInit = getNowBySeconds();
    init();
    waitInterval();
    getReachabilityManager().checkAllDevicesReachableStates();
    getReachabilityManager().onDeviceReceivedData( 1 );
    getReachabilityManager().onDeviceReceivedData( 2 );
    getReachabilityManager().onDeviceReceivedData( 3 );

    JsonObject nodesInfo;
    int64_t lastUpdate;
    bool reachable;

    getReachabilityManager().checkAllDevicesReachableStates();
    getNodeUpdateTimes( nodesInfo );
    EXPECT_EQ( nodesInfo.size(), 4 );
    getNodeInfo( nodesInfo, std::to_string( 1 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 2 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 3 ), reachable, lastUpdate );
    EXPECT_TRUE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );
    getNodeInfo( nodesInfo, std::to_string( 4 ), reachable, lastUpdate );
    EXPECT_FALSE( reachable );
    EXPECT_GE( lastUpdate, beforeInit );

    getReachabilityManager().onDeviceRemoved( { 1, 2, 3, 4 } );
    getNodeUpdateTimes( nodesInfo );
    EXPECT_TRUE( nodesInfo.is_null() );
}

}  // namespace UT
}  // namespace TwoGig
}  // namespace HubPlatform
