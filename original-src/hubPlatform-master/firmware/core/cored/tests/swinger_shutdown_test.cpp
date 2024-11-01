#include "cored/core_service.h"
#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "cored/swinger_shutdown/swinger_shutdown_devices_statistic.h"
#include "cored/swinger_shutdown/swinger_shutdown_time_series.h"
#include "cored/swinger_shutdown/swinger_shutdown_settings.h"
#include "cored/swinger_shutdown/swinger_shutdown_cleaner.h"
#include "config_reader/tests/config_reader_factory.h"
#include "cored/tests/service/test_core_service.h"

#include "cored/model/resources/cored_resources.h"
#include "cored/model/configuration/configuration_factory.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "cored/db_modifier/security_item_db_modifier.h"
#include "test_utils/test_utils.h"
#include "cored/tests/fakes/fake_feature_manager.h"
#include "database_client/fake_db/fake_db_client.h"
#include "service_interfaces/bus_wrapper.h"
#include "bus/fake_bus.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

#include "common_utils/cpp14.h"
#include <memory>

// Test plan:

// 1. Temporary statistic storing
// 2. Sync temporary statistic with persistent and check
// 3. Read config parameters
// 4. Temporary time series storing
// 5. Sync temporary time series with persistent and check
// 6. Calculate time frame boundary
// 7. Cleaner old records test
// 8. Active hits in time series
// 9. Check add Hit to 'Deleted' device statistic
// 10 Check and remove 'Deleted' records from summary
// 11 Check and remove 'Deleted' records from time series
// 12 Check and mark as 'Deleted' records in time series
// 13 Check and mark as 'Deleted' all corresponds records to device id
// 14 Check swinger shutdown list command
// 15 Check swinger shutdown reset command
// 16 Set device limits
// 17 Set device limits with cleaner
// 18 Reset device limits
// 19 Security item update for disabled device
// 20 Security item update if "protect.swinger_shutdown" is disabled
// 21 Security item update first time and check 'protect' field in the item updated broadcast
// 22 Security item update more of one time but hits limit not met
// 23 Security item update a few times and hits limit was met

namespace HubPlatform {
namespace UT {

using namespace Services;
using namespace HubPlatform::CoreD::SwingerShutdown;
using namespace CoreD::Model::Resources;

constexpr auto Id1 = "id1";
constexpr auto Id2 = "id2";
constexpr auto Id3 = "id3";
constexpr auto Id4 = "id4";
constexpr auto DeviceId1 = "DeviceId1";
constexpr auto DeviceId2 = "DeviceId2";

constexpr auto TimeFrameSeconds = 2;
constexpr auto UpdateIntervalSeconds = 1;
constexpr auto InactivityWindow = 3;
constexpr auto PeriodForRemoveAfterMarkAsDeleted = 2;

constexpr auto TempStatisticDB = TempFirmwareCoredSwingerShutdownDevices;
constexpr auto PersistentStatisticDB = FirmwareCoredSwingerShutdownDevices;
constexpr auto TempTimeSeriesDB = TempFirmwareCoredSwingerShutdownTimeSeries;
constexpr auto PersistentTimeSeriesDB = FirmwareCoredSwingerShutdownTimeSeries;
constexpr auto DefaultSyncPeriod = 1800;
constexpr auto TestSyncPeriod = 1;

class SwingerShutdownTestSuite : public ::testing::Test, public ConfigReaderFactory {
public:
    void SetUp() override;
    void TearDown() override;
    void cleanDB() const;
    static uint64_t now();
    void setTestData();
    static ConfigReader getConfig();
    Services::Service& getService();
    void setTimeSessionTestData() const;
    int getSummaryHits( const std::string& id, const std::string dbName = PersistentStatisticDB ) const;
    void cleanOldRecords( std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes >& service,
                          int inactivityWindow,
                          int periodForRemoveAfterMarkAsDeleted );
    int getDeletedSeries( const std::string& id );
    int getActiveHitsInTimeSeries( const std::string& id );
    void updateTimeSeriesRecord( const std::string& id,
                                 int hits,
                                 const std::string& deviceId,
                                 int begin,
                                 int updated,
                                 bool deleted,
                                 const std::string& dbName = PersistentTimeSeriesDB );
    void updateStatisticRecord( const std::string& id,
                                int hits,
                                int added,
                                int updated,
                                bool deleted,
                                const std::string& dbName = PersistentStatisticDB );
    void checkAndRemoveDeletedDevicesFromSummary( SwingerShutdownDeviceStatistic& summaryRecord );
    void checkAndRemoveDeletedRecordFromTimeSeries( SwingerShutdownTimeSeries& timeSeries );
    void checkAndRemoveMarkAsDeletedTimeSeries( SwingerShutdownTimeSeries& timeSeries );
    void markAsDeletedDevice( const std::string& id );
    bool getDeletedInSummaryTable( const std::string& id, const std::string& dbName ) const;
    bool getDeletedInTimeSeriesTable( const std::string& deviceId, const std::string& dbName ) const;
    void addLimitsForDevice( const std::string& id, int hitsLimit, int inactivityWindow );
    void addDisabledDevice( const std::string& id );
    CoreD::ItemDbModifier& getItemDbModifier();
    void checkBroadcastWithTimestamp( const Json& message, const Json& expected, int codeLine );
    const JsonObject expectedBroadcast();
    void addSecurityDevice();
    void broadcaster( MessageSubClasses subClass, const Json& broadcast );
    IDbClient& getDBClient();
    const Services::Message& getBroadcastMessage() const;
    CoreD::GatewayDbModifier& getGatewaysDbModifier();
    CoreD::IFeaturesManager& getFeatureManager();
    const JsonObject securityDevice();
    const JsonObject securityItem();
    Json updateSecurityItem( bool value );

public:
    std::unique_ptr< CoreD::IFeaturesManager > m_featureManager;
    Services::Message m_broadcastMessage;
    ProvisionDataUtils::ProvisionData m_provisionData;
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes > m_service;
    std::unique_ptr< CoreD::SwingerShutdown::SwingerShutdown > m_swingerShutdown;
    std::unique_ptr< CoreD::SecurityItemDbModifier > m_securityItemDbModifier;
    std::unique_ptr< CoreD::ItemDbModifier > m_itemDbModifier;

    const CoreD::DeviceDbModifier::Broadcasts m_itemBroadcasts{
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_ITEM_ADDED ), {} },
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_ITEM_UPDATED ), {} },
        { static_cast< MessageSubClasses >( MessageSubClasses::HUB_ITEM_REMOVED ), {} }
    };
};
void SwingerShutdownTestSuite::SetUp() {
    m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >();
    m_featureManager = std::make_unique< CoreD::UT::FakeFeatureManager >();
    m_service = std::make_unique< CoreD::Tests::TestCoreServiceWithoutHouseModes >();
    cleanDB();

    m_swingerShutdown = std::make_unique< CoreD::SwingerShutdown::SwingerShutdown >( *m_service );

    m_securityItemDbModifier =
        std::make_unique< CoreD::SecurityItemDbModifier >( *m_service, *m_featureManager, *m_swingerShutdown );

    m_itemDbModifier = std::make_unique< CoreD::ItemDbModifier >(
        *m_service,
        m_provisionData,
        [this]( MessageSubClasses subClass, const Json& broadcast ) { broadcaster( subClass, broadcast ); },
        *m_featureManager,
        *m_securityItemDbModifier );
    m_service->setUp();
}

void SwingerShutdownTestSuite::TearDown() {
    cleanDB();
    m_service->Services::Service::tearDown();
}

void SwingerShutdownTestSuite::broadcaster( MessageSubClasses subClass, const Json& broadcast ) {
    m_broadcastMessage.msgSubClass = subClass;
    m_broadcastMessage.data = broadcast;
}

void SwingerShutdownTestSuite::cleanDB() const {
    m_service->getDBClient().clean();
}

uint64_t SwingerShutdownTestSuite::now() {
    return std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() )
        .count();
}

ConfigReader SwingerShutdownTestSuite::getConfig() {
    JsonObject configData = JSON( "swinger_shutdown" << JSON( "defaults" << JSON( "hitsLimit" << 2 << "inactivityWindow"
                                                                                              << "8s" ) ) );
    std::string stringData = configData.dump();
    return ConfigReaderFactory::createConfigReader( [stringData]( const std::string& ) { return stringData; } );
}

void SwingerShutdownTestSuite::setTestData() {
    SwingerShutdown manager( *this->m_service, getConfig(), DefaultSyncPeriod );

    manager.addDeviceHit( Id1 );
    manager.addDeviceHit( Id1 );

    manager.addDeviceHit( Id2 );
    manager.addDeviceHit( Id2 );
    manager.addDeviceHit( Id2 );

    manager.setDeviceDeleted( Id1, true );
    manager.setDeviceDeleted( Id2, true );
}

void SwingerShutdownTestSuite::setTimeSessionTestData() const {
    SwingerShutdown manager( *this->m_service, getConfig(), DefaultSyncPeriod );

    manager.addDeviceHitToTimeSeries( Id1, TimeFrameSeconds );
    manager.addDeviceHitToTimeSeries( Id1, TimeFrameSeconds );

    manager.addDeviceHitToTimeSeries( Id2, TimeFrameSeconds );
    manager.addDeviceHitToTimeSeries( Id2, TimeFrameSeconds );
    manager.addDeviceHitToTimeSeries( Id2, TimeFrameSeconds );

    manager.setDeletedForTimeSeries( Id1, true );
    manager.setDeletedForTimeSeries( Id2, true );
}

int SwingerShutdownTestSuite::getSummaryHits( const std::string& id, const std::string dbName ) const {
    auto& dbClient = m_service->getDBClient();
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, dbName, dbClient );
    SwingerShutdownDeviceStatistic info( record );
    return info.getHits();
}

Services::Service& SwingerShutdownTestSuite::getService() {
    return *m_service;
}

bool SwingerShutdownTestSuite::getDeletedInSummaryTable( const std::string& id, const std::string& dbName ) const {
    auto& dbClient = m_service->getDBClient();
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, dbName, dbClient );
    SwingerShutdownDeviceStatistic info( record );
    return info.getDeleted();
}

bool SwingerShutdownTestSuite::getDeletedInTimeSeriesTable( const std::string& deviceId,
                                                            const std::string& dbName ) const {
    auto& dbClient = m_service->getDBClient();
    auto records = SwingerShutdownTimeSeries::loadAllRecordsFromDb( dbName, JSON( "deviceId" << deviceId ), dbClient );

    if( records.empty() )
        return false;

    for( auto record : records ) {
        SwingerShutdownDeviceStatistic info( record );
        if( !info.getDeleted() )
            return false;
    }

    return true;
}

void SwingerShutdownTestSuite::cleanOldRecords(
    std::unique_ptr< CoreD::Tests::TestCoreServiceWithoutHouseModes >& service,
    int inactivityWindow,
    int periodForRemoveAfterMarkAsDeleted ) {
    SwingerShutdownCleaner cleaner( *m_service, 2, inactivityWindow, periodForRemoveAfterMarkAsDeleted );
    cleaner.run();
}

int SwingerShutdownTestSuite::getDeletedSeries( const std::string& id ) {
    auto& dbClient = m_service->getDBClient();
    auto request = JSON( "deviceId" << id );
    auto allRecords = dbClient.findAll( PersistentTimeSeriesDB, request, nullptr );
    int deletedSeriesNumber = 0;

    for( JsonObject& record : allRecords ) {
        SwingerShutdownTimeSeries series( record );
        auto idDeleted = series.getDeleted();
        if( idDeleted ) {
            deletedSeriesNumber++;
        }
    }

    return deletedSeriesNumber;
}

int SwingerShutdownTestSuite::getActiveHitsInTimeSeries( const std::string& id ) {
    SwingerShutdownCleaner cleaner( *m_service, 2, 1, 1 );
    return cleaner.getActiveHitsInTimeSeries( id );
}

void SwingerShutdownTestSuite::updateTimeSeriesRecord( const std::string& id,
                                                       int hits,
                                                       const std::string& deviceId,
                                                       int begin,
                                                       int updated,
                                                       bool deleted,
                                                       const std::string& dbName ) {
    auto& dbClient = m_service->getDBClient();
    auto record = SwingerShutdownTimeSeries::loadFromDb( id, dbName, dbClient );
    SwingerShutdownTimeSeries series( record );
    series.setHits( hits );
    series.setDeviceId( deviceId );
    series.setBegin( begin );
    series.setUpdated( updated );
    series.setDeleted( deleted );
    SwingerShutdownTimeSeries::updateDbRecord( id, series.toJson(), dbName, dbClient );
}

void SwingerShutdownTestSuite::updateStatisticRecord( const std::string& id,
                                                      int hits,
                                                      int added,
                                                      int updated,
                                                      bool deleted,
                                                      const std::string& dbName ) {
    auto& dbClient = m_service->getDBClient();
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( id, dbName, dbClient );
    SwingerShutdownDeviceStatistic series( record );
    series.setHits( hits );
    series.setAdded( added );
    series.setUpdated( updated );
    series.setDeleted( deleted );
    SwingerShutdownDeviceStatistic::updateDbRecord( id, series.toJson(), dbName, dbClient );
}

void SwingerShutdownTestSuite::checkAndRemoveDeletedDevicesFromSummary(
    SwingerShutdownDeviceStatistic& summaryRecord ) {
    SwingerShutdownCleaner cleaner( *m_service, 2, 1, 1 );
    return cleaner.checkAndRemoveDeletedDevicesFromSummary( summaryRecord );
}

void SwingerShutdownTestSuite::checkAndRemoveDeletedRecordFromTimeSeries( SwingerShutdownTimeSeries& timeSeries ) {
    const int InactivityWindow = 1;
    const int DeleteSeriesPeriod = 1;
    SwingerShutdownCleaner cleaner( *m_service, 2, InactivityWindow, DeleteSeriesPeriod );
    auto begin =
        std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() ) -
        std::chrono::seconds( InactivityWindow + DeleteSeriesPeriod );
    return cleaner.checkAndDeleteSeries( begin, timeSeries );
}

void SwingerShutdownTestSuite::checkAndRemoveMarkAsDeletedTimeSeries( SwingerShutdownTimeSeries& timeSeries ) {
    const int InactivityWindow = 1;
    const int DeleteSeriesPeriod = 1;
    SwingerShutdownCleaner cleaner( *m_service, 2, InactivityWindow, DeleteSeriesPeriod );
    auto begin =
        std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() ) -
        std::chrono::seconds( InactivityWindow );
    return cleaner.checkAndMarkAsDeletedSeries( timeSeries );
}

void SwingerShutdownTestSuite::markAsDeletedDevice( const std::string& id ) {
    SwingerShutdown manager( *this->m_service, getConfig(), DefaultSyncPeriod );
    manager.markDeviceRecordsAsDeleted( id );
}

void SwingerShutdownTestSuite::addLimitsForDevice( const std::string& id, int hitsLimit, int inactivityWindow ) {
    auto& dbClient = m_service->getDBClient();
    auto record = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, dbClient );
    SwingerShutdownSettings settings( record );
    settings.setDeviceLimits( id, hitsLimit, inactivityWindow );
    SwingerShutdownSettings::updateDbRecord( settings.toJson(), HouseModesSwingerShutdown, dbClient );
}

void SwingerShutdownTestSuite::addDisabledDevice( const std::string& id ) {
    auto& dbClient = m_service->getDBClient();
    auto record = SwingerShutdownSettings::loadFromDb( HouseModesSwingerShutdown, dbClient );
    SwingerShutdownSettings settings( record );
    settings.addDisabledDevice( id );
    SwingerShutdownSettings::updateDbRecord( settings.toJson(), HouseModesSwingerShutdown, dbClient );
}

IDbClient& SwingerShutdownTestSuite::getDBClient() {
    return m_service->getDBClient();
}

const Services::Message& SwingerShutdownTestSuite::getBroadcastMessage() const {
    return m_broadcastMessage;
}

CoreD::ItemDbModifier& SwingerShutdownTestSuite::getItemDbModifier() {
    return *m_itemDbModifier;
}

CoreD::GatewayDbModifier& SwingerShutdownTestSuite::getGatewaysDbModifier() {
    return m_service->getGatewaysDbModifier();
}

CoreD::IFeaturesManager& SwingerShutdownTestSuite::getFeatureManager() {
    return *m_featureManager;
}
const JsonObject SwingerShutdownTestSuite::securityDevice() {
    return R"(
    {
    "_id": "id1",
    "batteryPowered": true,
    "category": "scene_controller",
    "deviceTypeId": "134_3_25",
    "gatewayId": "gateway_id1",
    "name": "Panic Button",
    "parentDeviceId": "",
    "persistent": false,
    "reachable": true,
    "ready": true,
    "roomId": "",
    "security": "no",
    "status": "idle",
    "info": {},
    "subcategory": "panic",
    "type": "panic.button",
    "armed": true
    }
    )"_json;
}

const JsonObject SwingerShutdownTestSuite::securityItem() {
    return R"(
    {
    "_id": "item_id1",
    "deviceId": "id1",
    "hasGetter": true,
    "hasSetter": true,
    "name": "security_threat",
    "show": true,
    "value": false,
    "valueFormatted": "false",
    "valueType": "bool"
    }
    )"_json;
}

void SwingerShutdownTestSuite::addSecurityDevice() {
    auto& dbClient = m_service->getDBClient();

    dbClient.insert( DBNames::DevicesDB, securityDevice() );
    const JsonObject protectServiceRecord = R"({"_id": "id1", "service": "medical"})"_json;
    dbClient.insert( CoreDHouseModes::Resources::DBNames::HouseModesProtectButtonsDB, protectServiceRecord );

    getItemDbModifier().insert( securityItem() );
    EXPECT_EQ( getBroadcastMessage().msgSubClass, m_itemBroadcasts.addOperation.classType );

    bool isProtectOn = getFeatureManager().getFeatureStatus( "protect" );
    if( !isProtectOn ) {
        getFeatureManager().setFeatureStatus( "protect", true );
    }

    bool isSwingerShutdownOn = getFeatureManager().getFeatureStatus( "protect.swinger_shutdown" );
    if( !isSwingerShutdownOn ) {
        getFeatureManager().setFeatureStatus( "protect.swinger_shutdown", true );
    }

    const bool protectOn = true;
    auto currentHouseModeId = CoreDHouseModes::getCurrentHouseModeId( getDBClient() );
    CoreDHouseModes::setProtect( getService(), getItemDbModifier(), currentHouseModeId, protectOn );
}

Json SwingerShutdownTestSuite::updateSecurityItem( bool value ) {
    JsonObject updatePortion;
    updatePortion[ ItemFields::ValueFieldName ] = value;

    getItemDbModifier().update( "item_id1", updatePortion, false );
    return getBroadcastMessage().data;
}

TEST_F( SwingerShutdownTestSuite, temporaryDBStoring ) {  // 1
    setTestData();
    auto& dbClient = m_service->getDBClient();
    auto record1 = SwingerShutdownDeviceStatistic::loadFromDb( Id1, TempStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic info1( record1 );
    auto record2 = SwingerShutdownDeviceStatistic::loadFromDb( Id2, TempStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic info2( record2 );

    EXPECT_EQ( info1.getHits(), 2 );
    EXPECT_EQ( info2.getHits(), 3 );

    EXPECT_EQ( info1.getDeleted(), true );
    EXPECT_EQ( info2.getDeleted(), true );
}

TEST_F( SwingerShutdownTestSuite, syncDeviceStatisticDB ) {  // 2
    auto timePoint = now();
    setTestData();
    std::this_thread::sleep_for( std::chrono::seconds( UpdateIntervalSeconds ) );
    setTestData();

    auto& dbClient = m_service->getDBClient();
    auto record1 = SwingerShutdownDeviceStatistic::loadFromDb( Id1, TempStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic infoTemp1( record1 );
    auto record2 = SwingerShutdownDeviceStatistic::loadFromDb( Id2, TempStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic infoTemp2( record2 );

    auto addedTemp1 = infoTemp1.getAdded();
    auto addedTemp2 = infoTemp2.getAdded();

    auto updatedTemp1 = infoTemp1.getUpdated();
    auto updatedTemp2 = infoTemp2.getUpdated();

    EXPECT_EQ( addedTemp1, timePoint );
    EXPECT_EQ( addedTemp2, timePoint );
    auto updatedTime = timePoint + UpdateIntervalSeconds;

    EXPECT_EQ( updatedTemp1, updatedTime );
    EXPECT_EQ( updatedTemp2, updatedTime );

    SwingerShutdown manager( *m_service, getConfig(), DefaultSyncPeriod );
    manager.syncWithDeviceStatisticPersistentStore();

    record1 = SwingerShutdownDeviceStatistic::loadFromDb( Id1, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic info1( record1 );
    record2 = SwingerShutdownDeviceStatistic::loadFromDb( Id2, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic info2( record2 );

    EXPECT_EQ( info1.getHits(), 4 );
    EXPECT_EQ( info2.getHits(), 6 );

    EXPECT_EQ( info1.getDeleted(), true );
    EXPECT_EQ( info2.getDeleted(), true );

    auto addedPersistent1 = infoTemp1.getAdded();
    auto addedPersistent2 = infoTemp2.getAdded();

    auto updatedPersistent1 = infoTemp1.getUpdated();
    auto updatedPersistent2 = infoTemp2.getUpdated();

    EXPECT_TRUE( addedTemp1 == addedPersistent1 );
    EXPECT_TRUE( addedTemp2 == addedPersistent2 );
    EXPECT_TRUE( updatedTemp1 == updatedPersistent1 );
    EXPECT_TRUE( updatedTemp2 == updatedPersistent2 );
}

TEST_F( SwingerShutdownTestSuite, getConfigData ) {  // 3
    SwingerShutdown manager( *m_service, getConfig(), DefaultSyncPeriod );
    const auto configReader = getConfig();

    auto config = manager.getConfigLimitsDefault( getConfig() );
    EXPECT_EQ( config.dump(), R"({"defaults":{"hitsLimit":2,"inactivityWindow":8}})" );
    manager.initDefaults();
    auto& dbClient = m_service->getDBClient();
    auto dbData = dbClient.findAll( HouseModesSwingerShutdown, JSON_EMPTY_OBJ, nullptr );

    JsonObject defaults;
    json_get_obj( dbData.front(), "defaults", defaults );
    EXPECT_EQ( json_get_int( defaults, "hitsLimit" ), 2 );
    EXPECT_EQ( json_get_int( defaults, "inactivityWindow" ), 8 );
}

TEST_F( SwingerShutdownTestSuite, temporaryDBStoringTimeSeries ) {  // 4
    auto timePoint = now();
    setTimeSessionTestData();
    std::this_thread::sleep_for( std::chrono::seconds( UpdateIntervalSeconds ) );
    setTimeSessionTestData();

    auto& dbClient = m_service->getDBClient();
    auto record1 = SwingerShutdownTimeSeries::loadFromDb( Id1, TempTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries info1( record1 );
    auto record2 = SwingerShutdownTimeSeries::loadFromDb( Id2, TempTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries info2( record2 );

    EXPECT_EQ( info1.getHits(), 4 );
    EXPECT_EQ( info2.getHits(), 6 );
    EXPECT_EQ( info1.getDeleted(), true );
    EXPECT_EQ( info2.getDeleted(), true );
    EXPECT_EQ( info1.getBegin(), timePoint );
    EXPECT_EQ( info2.getBegin(), timePoint );
    EXPECT_EQ( info1.getUpdated(), timePoint + UpdateIntervalSeconds );
    EXPECT_EQ( info2.getUpdated(), timePoint + UpdateIntervalSeconds );
}

TEST_F( SwingerShutdownTestSuite, syncTimeSeriesDB ) {  // 5
    setTimeSessionTestData();

    auto& dbClient = m_service->getDBClient();
    auto record1 = SwingerShutdownTimeSeries::loadFromDb( Id1, TempTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries infoTemp1( record1 );
    auto record2 = SwingerShutdownTimeSeries::loadFromDb( Id2, TempTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries infoTemp2( record2 );

    auto addedTemp1 = infoTemp1.getBegin();
    auto addedTemp2 = infoTemp2.getBegin();

    auto updatedTemp1 = infoTemp1.getBegin();
    auto updatedTemp2 = infoTemp2.getBegin();

    EXPECT_TRUE( addedTemp1 > 0 );
    EXPECT_TRUE( addedTemp2 > 0 );
    EXPECT_TRUE( updatedTemp1 > 0 );
    EXPECT_TRUE( updatedTemp2 > 0 );

    SwingerShutdown manager( *m_service, getConfig(), DefaultSyncPeriod );
    manager.syncWithTimeSeriesPersistentStore();
    JsonArray storedRecords = dbClient.findAll( PersistentTimeSeriesDB, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( storedRecords.size(), 2 );

    record1 = SwingerShutdownTimeSeries::loadRecordsFromDbWithDeviceId( Id1, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownDeviceStatistic info1( record1.front() );
    record2 = SwingerShutdownTimeSeries::loadRecordsFromDbWithDeviceId( Id2, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownDeviceStatistic info2( record2.front() );

    EXPECT_EQ( info1.getHits(), 2 );
    EXPECT_EQ( info2.getHits(), 3 );

    EXPECT_EQ( info1.getDeleted(), true );
    EXPECT_EQ( info2.getDeleted(), true );

    auto addedPersistent1 = infoTemp1.getBegin();
    auto addedPersistent2 = infoTemp2.getBegin();

    auto updatedPersistent1 = infoTemp1.getBegin();
    auto updatedPersistent2 = infoTemp2.getBegin();

    EXPECT_TRUE( addedTemp1 == addedPersistent1 );
    EXPECT_TRUE( addedTemp2 == addedPersistent2 );
    EXPECT_TRUE( updatedTemp1 == updatedPersistent1 );
    EXPECT_TRUE( updatedTemp2 == updatedPersistent2 );

    auto temporaryData = dbClient.findAll( TempTimeSeriesDB, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( temporaryData.empty(), true );

    setTimeSessionTestData();
    manager.syncWithTimeSeriesPersistentStore();
    storedRecords = dbClient.findAll( PersistentTimeSeriesDB, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( storedRecords.size(), 4 );
}

TEST_F( SwingerShutdownTestSuite, calculateTimeFrameBeginBoundary ) {  // 6
    SwingerShutdown manager( *m_service, getConfig(), DefaultSyncPeriod );
    auto& dbClient = m_service->getDBClient();

    auto setBegin = [&dbClient]( const std::string& id, uint64_t beginTime ) {
        auto record = SwingerShutdownTimeSeries::loadFromDb( id, TempTimeSeriesDB, dbClient );

        if( record.empty() ) {
            SwingerShutdownTimeSeries::createDbRecord( id, JSON_EMPTY_OBJ, TempTimeSeriesDB, dbClient );
        }

        SwingerShutdownTimeSeries series( record );
        series.setBegin( beginTime );
        SwingerShutdownTimeSeries::updateDbRecord( id, series.toJson(), TempTimeSeriesDB, dbClient );
    };

    auto id = Id3;
    auto timePoint = now();
    auto boundary1 = manager.calculateBeginOfIntervalUtc( id, timePoint, TimeFrameSeconds );
    setBegin( id, boundary1 );
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

    timePoint = now();
    auto boundary2 = manager.calculateBeginOfIntervalUtc( Id3, timePoint, TimeFrameSeconds );
    setBegin( id, boundary2 );
    EXPECT_EQ( boundary1, boundary2 );

    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    timePoint = now();
    auto boundary3 = manager.calculateBeginOfIntervalUtc( Id3, timePoint, TimeFrameSeconds );
    setBegin( id, boundary3 );
    EXPECT_EQ( boundary1, boundary3 );

    std::this_thread::sleep_for( std::chrono::microseconds( 499 ) );
    timePoint = now();
    auto boundary4 = manager.calculateBeginOfIntervalUtc( Id3, timePoint, TimeFrameSeconds );
    setBegin( id, boundary4 );
    EXPECT_EQ( boundary1, boundary4 );

    std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );
    timePoint = now();
    auto boundary5 = manager.calculateBeginOfIntervalUtc( Id3, timePoint, TimeFrameSeconds );
    setBegin( id, boundary5 );
    EXPECT_EQ( boundary1 + TimeFrameSeconds, boundary5 );

    std::this_thread::sleep_for( std::chrono::milliseconds( 4000 ) );
    timePoint = now();
    auto boundary6 = manager.calculateBeginOfIntervalUtc( Id3, timePoint, TimeFrameSeconds );
    EXPECT_EQ( boundary1 + 3 * TimeFrameSeconds, boundary6 );
}

TEST_F( SwingerShutdownTestSuite, cleanerTest ) {  // 7
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();

    auto delay = []() { std::this_thread::sleep_for( std::chrono::seconds( 2 ) ); };

    manager.addHit( Id1, TimeFrameSeconds );

    manager.addHit( Id2, TimeFrameSeconds );
    manager.addHit( Id3, TimeFrameSeconds );

    delay();
    manager.syncTask();
    auto summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 1 );
    manager.addHit( Id1, TimeFrameSeconds );
    delay();
    manager.syncTask();
    summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 3 );
    cleanOldRecords( m_service, InactivityWindow, PeriodForRemoveAfterMarkAsDeleted );
    summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 1 );
    int deletedSeriesNumber = getDeletedSeries( Id1 );
    EXPECT_EQ( deletedSeriesNumber, 1 );
    int deletedSeriesNumber2 = getDeletedSeries( Id2 );
    EXPECT_EQ( deletedSeriesNumber2, 1 );
    delay();
    cleanOldRecords( m_service, InactivityWindow, PeriodForRemoveAfterMarkAsDeleted );
    summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 0 );
    deletedSeriesNumber = getDeletedSeries( Id1 );
    EXPECT_EQ( deletedSeriesNumber, 1 );
    delay();
    cleanOldRecords( m_service, InactivityWindow, PeriodForRemoveAfterMarkAsDeleted );
    deletedSeriesNumber = getDeletedSeries( Id1 );
    EXPECT_EQ( deletedSeriesNumber, 0 );
    auto allTimeframeRecords = dbClient.findAll( PersistentTimeSeriesDB, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( allTimeframeRecords.size(), 0 );
}

TEST_F( SwingerShutdownTestSuite, getActiveHitsInTimeSeries ) {  // 8
    auto& dbClient = m_service->getDBClient();
    JsonObject record;

    // device1
    SwingerShutdownTimeSeries::createDbRecord( Id1, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id3, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );

    const int Hits1 = 3;
    int begin = 1;
    int updated = 2;
    bool deleted = true;

    updateTimeSeriesRecord( Id1, Hits1, DeviceId1, begin, updated, deleted );

    const int Hits2 = 111;
    begin = 2;
    updated = 3;
    deleted = false;

    updateTimeSeriesRecord( Id2, Hits2, DeviceId1, begin, updated, deleted );

    const int Hits3 = 50;
    begin = 3;
    updated = 4;
    deleted = false;

    updateTimeSeriesRecord( Id3, Hits3, DeviceId1, begin, updated, deleted );

    const int Hits4 = 345;
    begin = 4;
    updated = 5;
    deleted = false;

    // device2
    SwingerShutdownTimeSeries::createDbRecord( Id4, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );

    updateTimeSeriesRecord( Id4, Hits4, DeviceId2, begin, updated, deleted );

    auto hitsOfDevice1 = getActiveHitsInTimeSeries( DeviceId1 );
    EXPECT_EQ( hitsOfDevice1, Hits2 + Hits3 );

    auto hitsOfDevice2 = getActiveHitsInTimeSeries( DeviceId2 );
    EXPECT_EQ( hitsOfDevice2, Hits4 );
}

TEST_F( SwingerShutdownTestSuite, addHitToDeletedDevice ) {  // 9
    auto& dbClient = m_service->getDBClient();
    JsonObject record;
    SwingerShutdownDeviceStatistic::createDbRecord( Id1, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( Id3, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );

    bool deleted = true;
    updateStatisticRecord( Id1, 1, 1, 2, deleted );
    auto record1 = SwingerShutdownDeviceStatistic::loadFromDb( Id1, PersistentStatisticDB, dbClient );

    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    EXPECT_FALSE( manager.addHit( Id1 ) );
}

TEST_F( SwingerShutdownTestSuite, checkAndRemoveDeletedDevicesFromSummary ) {  // 10
    auto& dbClient = m_service->getDBClient();

    SwingerShutdownDeviceStatistic::createDbRecord( Id1, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( Id3, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );

    bool deleted = true;
    updateStatisticRecord( Id1, 1, 1, 2, deleted );
    auto record = SwingerShutdownDeviceStatistic::loadFromDb( Id1, PersistentStatisticDB, dbClient );

    EXPECT_FALSE( record.empty() );
    SwingerShutdownDeviceStatistic statistic( record );
    checkAndRemoveDeletedDevicesFromSummary( statistic );
    record = SwingerShutdownDeviceStatistic::loadFromDb( Id1, PersistentStatisticDB, dbClient );
    EXPECT_TRUE( record.empty() );
}

TEST_F( SwingerShutdownTestSuite, checkAndRemoveDeletedRecordFromTimeSeries ) {  // 11
    auto& dbClient = m_service->getDBClient();

    SwingerShutdownTimeSeries::createDbRecord( Id1, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id3, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );

    const int Hits1 = 3;
    int begin = 1;
    int updated = 2;
    bool deleted = true;

    updateTimeSeriesRecord( Id1, Hits1, DeviceId1, begin, updated, deleted );

    auto record = SwingerShutdownTimeSeries::loadFromDb( Id1, PersistentTimeSeriesDB, dbClient );
    EXPECT_FALSE( record.empty() );
    SwingerShutdownTimeSeries series( record );
    checkAndRemoveDeletedRecordFromTimeSeries( series );
    record = SwingerShutdownTimeSeries::loadFromDb( Id1, PersistentTimeSeriesDB, dbClient );
    EXPECT_TRUE( record.empty() );
}

TEST_F( SwingerShutdownTestSuite, checkAndMarkAsDeletedTimeSeries ) {  // 12
    auto& dbClient = m_service->getDBClient();

    SwingerShutdownTimeSeries::createDbRecord( Id1, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id3, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );

    const int Hits1 = 3;
    int begin = 1;
    int updated = 2;
    bool deleted = true;

    updateTimeSeriesRecord( Id1, Hits1, DeviceId1, begin, updated, deleted );

    auto record = SwingerShutdownTimeSeries::loadFromDb( Id1, PersistentTimeSeriesDB, dbClient );
    EXPECT_FALSE( record.empty() );
    SwingerShutdownTimeSeries series( record );
    checkAndRemoveMarkAsDeletedTimeSeries( series );
    record = SwingerShutdownTimeSeries::loadFromDb( Id1, PersistentTimeSeriesDB, dbClient );
    EXPECT_TRUE( !record.empty() );
    SwingerShutdownTimeSeries seriesAfterProcess( record );
    EXPECT_TRUE( seriesAfterProcess.getDeleted() );
}

TEST_F( SwingerShutdownTestSuite, markAsAllCorrespondRecordsRecordsAsDevicesForRemovedDevices ) {  // 13
    auto& dbClient = m_service->getDBClient();
    bool deleted = false;

    SwingerShutdownDeviceStatistic::createDbRecord( DeviceId1, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( DeviceId1, JSON_EMPTY_OBJ, TempStatisticDB, dbClient );
    updateStatisticRecord( DeviceId1, 1, 1, 2, deleted );
    updateStatisticRecord( DeviceId1, 1, 1, 2, deleted, TempStatisticDB );

    SwingerShutdownTimeSeries::createDbRecord( Id1, JSON_EMPTY_OBJ, TempTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    updateTimeSeriesRecord( Id1, 1, DeviceId1, 1, 2, deleted, TempTimeSeriesDB );
    updateTimeSeriesRecord( Id2, 1, DeviceId1, 1, 2, deleted );

    markAsDeletedDevice( DeviceId1 );
    EXPECT_TRUE( getDeletedInSummaryTable( DeviceId1, TempStatisticDB ) );
    EXPECT_TRUE( getDeletedInSummaryTable( DeviceId1, PersistentStatisticDB ) );

    EXPECT_TRUE( getDeletedInTimeSeriesTable( DeviceId1, TempTimeSeriesDB ) );
    EXPECT_TRUE( getDeletedInTimeSeriesTable( DeviceId1, PersistentTimeSeriesDB ) );
}

TEST_F( SwingerShutdownTestSuite, swingerShutdownList ) {  // 14
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();

    // Check all fields of one record:
    manager.addHit( Id1, TimeFrameSeconds );
    const int DeviceHitsLimits = 3;
    const int DeviceInactivityWindow = 5;
    addLimitsForDevice( Id1, DeviceHitsLimits, DeviceInactivityWindow );

    addDisabledDevice( Id2 );
    auto params = JSON( "version"
                        << "0" );
    auto list = manager.list( params );
    JsonArray devices;
    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 1 );
    auto device = devices.front();
    EXPECT_EQ( json_get_str( device, "deviceId" ), Id1 );
    JsonObject deviceLimits;
    EXPECT_TRUE( json_get_obj( device, "limits", deviceLimits ) );
    EXPECT_EQ( json_get_int( deviceLimits, "hitsLimit" ), DeviceHitsLimits );
    EXPECT_EQ( json_get_int( deviceLimits, "inactivityWindow" ), DeviceInactivityWindow );

    JsonObject stat;

    EXPECT_TRUE( json_get_obj( device, "stat", stat ) );
    EXPECT_EQ( json_get_int( stat, "hits" ), 1 );
    auto timeNow = now();

    EXPECT_TRUE( json_get_int( stat, "added" ) >= timeNow );
    EXPECT_TRUE( json_get_int( stat, "updated" ) >= timeNow );

    JsonArray disabledDevices;
    EXPECT_TRUE( json_get_arr( list, "disabledDevices", disabledDevices ) );
    EXPECT_EQ( devices.size(), 1 );
    EXPECT_EQ( disabledDevices.front(), Id2 );

    JsonObject limits;

    EXPECT_TRUE( json_get_obj( list, "limits", limits ) );
    EXPECT_EQ( json_get_int( limits, "hitsLimit" ), 2 );
    EXPECT_EQ( json_get_int( limits, "inactivityWindow" ), 8 );

    manager.addHit( Id2, TimeFrameSeconds );
    manager.addHit( Id3, TimeFrameSeconds );

    // No version and 3 items in the list:
    list = manager.list( JSON_EMPTY_OBJ );
    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 3 );
    list = manager.list( JSON( "version"
                               << "1" ) );

    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 3 );

    auto version = json_get_str( list, "version" );
    EXPECT_TRUE( !version.empty() );
    // The same version requested: empty result:
    params = JSON( "version" << version );
    list = manager.list( params );
    EXPECT_TRUE( list.empty() );
}

TEST_F( SwingerShutdownTestSuite, swingeShutdownReset ) {  // 15
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();
    bool deleted = false;

    SwingerShutdownDeviceStatistic::createDbRecord( DeviceId1, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( DeviceId1, JSON_EMPTY_OBJ, TempStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( DeviceId2, JSON_EMPTY_OBJ, PersistentStatisticDB, dbClient );
    SwingerShutdownDeviceStatistic::createDbRecord( DeviceId2, JSON_EMPTY_OBJ, TempStatisticDB, dbClient );

    updateStatisticRecord( DeviceId1, 5, 1, 2, deleted );
    updateStatisticRecord( DeviceId1, 2, 1, 2, deleted, TempStatisticDB );
    updateStatisticRecord( DeviceId1, 5, 1, 2, deleted );
    updateStatisticRecord( DeviceId2, 3, 1, 2, deleted, TempStatisticDB );

    SwingerShutdownTimeSeries::createDbRecord( Id1, JSON_EMPTY_OBJ, TempTimeSeriesDB, dbClient );
    SwingerShutdownTimeSeries::createDbRecord( Id2, JSON_EMPTY_OBJ, PersistentTimeSeriesDB, dbClient );
    updateTimeSeriesRecord( DeviceId1, 4, DeviceId1, 1, 2, deleted, TempTimeSeriesDB );
    updateTimeSeriesRecord( DeviceId1, 6, DeviceId1, 1, 2, deleted );
    updateTimeSeriesRecord( DeviceId1, 7, DeviceId1, 2, 3, deleted, TempTimeSeriesDB );
    updateTimeSeriesRecord( DeviceId1, 9, DeviceId1, 2, 3, deleted );
    updateTimeSeriesRecord( DeviceId2, 5, DeviceId1, 1, 2, deleted, TempTimeSeriesDB );
    updateTimeSeriesRecord( DeviceId2, 2, DeviceId1, 1, 2, deleted );

    manager.swingerShutdownReset( JSON( "deviceId" << DeviceId1 ) );
    EXPECT_EQ( getSummaryHits( DeviceId1, PersistentStatisticDB ), 0 );
    EXPECT_EQ( getSummaryHits( DeviceId1, TempStatisticDB ), 0 );
}

TEST_F( SwingerShutdownTestSuite, setDeviceLimits ) {  // 16
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();
    addSecurityDevice();
    manager.addHit( Id1, TimeFrameSeconds );
    const int DeviceHitsLimitDefault = 3;
    const int DeviceInactivityWindowDefault = 5;
    addLimitsForDevice( Id1, DeviceHitsLimitDefault, DeviceInactivityWindowDefault );

    const int DeviceHitsLimit = 10;
    const int DeviceInactivityWindow = 20;
    manager.swingerShutdownLimitsSet(
        JSON( "deviceId" << Id1 << "hitsLimit" << DeviceHitsLimit << "inactivityWindow" << DeviceInactivityWindow ) );

    auto list = manager.list( JSON_EMPTY_OBJ );
    JsonArray devices;
    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 1 );
    auto device = devices.front();
    EXPECT_EQ( json_get_str( device, "deviceId" ), Id1 );
    JsonObject deviceLimits;
    EXPECT_TRUE( json_get_obj( device, "limits", deviceLimits ) );
    EXPECT_EQ( json_get_int( deviceLimits, "hitsLimit" ), DeviceHitsLimit );
    EXPECT_EQ( json_get_int( deviceLimits, "inactivityWindow" ), DeviceInactivityWindow );
}

TEST_F( SwingerShutdownTestSuite, cleanerTestWithDevicesLimits ) {  // 17
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();
    addSecurityDevice();
    auto delay = []() { std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); };
    manager.addHit( Id1, TimeFrameSeconds );
    addLimitsForDevice( Id1, 2, 2 );
    delay();
    manager.syncTask();
    auto summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 1 );
    manager.addHit( Id1, TimeFrameSeconds );
    delay();
    manager.syncTask();
    summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 3 );
    cleanOldRecords( m_service, InactivityWindow, PeriodForRemoveAfterMarkAsDeleted );
    summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 1 );
    int deletedSeriesNumber = getDeletedSeries( Id1 );
    EXPECT_EQ( deletedSeriesNumber, 1 );
    delay();
    cleanOldRecords( m_service, InactivityWindow, PeriodForRemoveAfterMarkAsDeleted );
    summaryHits = getSummaryHits( Id1 );
    EXPECT_EQ( summaryHits, 0 );
    deletedSeriesNumber = getDeletedSeries( Id1 );
    EXPECT_EQ( deletedSeriesNumber, 2 );
    delay();
    cleanOldRecords( m_service, InactivityWindow, PeriodForRemoveAfterMarkAsDeleted );
    deletedSeriesNumber = getDeletedSeries( Id1 );
    EXPECT_EQ( deletedSeriesNumber, 1 );
    auto allTimeframeRecords = dbClient.findAll( PersistentTimeSeriesDB, JSON_EMPTY_OBJ, nullptr );
    EXPECT_EQ( allTimeframeRecords.size(), 1 );
}

TEST_F( SwingerShutdownTestSuite, resetDeviceLimits ) {  // 18
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();
    addSecurityDevice();
    manager.addHit( Id1, TimeFrameSeconds );
    const int DeviceHitsLimitDefault = 3;
    const int DeviceInactivityWindowDefault = 5;
    addLimitsForDevice( Id1, DeviceHitsLimitDefault, DeviceInactivityWindowDefault );

    const int DeviceHitsLimit = 10;
    const int DeviceInactivityWindow = 20;
    manager.swingerShutdownLimitsSet(
        JSON( "deviceId" << Id1 << "hitsLimit" << DeviceHitsLimit << "inactivityWindow" << DeviceInactivityWindow ) );

    auto list = manager.list( JSON_EMPTY_OBJ );
    JsonArray devices;
    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 1 );
    auto device = devices.front();
    EXPECT_EQ( json_get_str( device, "deviceId" ), Id1 );
    JsonObject deviceLimits;
    EXPECT_TRUE( json_get_obj( device, "limits", deviceLimits ) );
    EXPECT_EQ( json_get_int( deviceLimits, "hitsLimit" ), DeviceHitsLimit );
    EXPECT_EQ( json_get_int( deviceLimits, "inactivityWindow" ), DeviceInactivityWindow );

    manager.swingerShutdownLimitsReset( JSON( "deviceId" << Id1 ) );

    list = manager.list( JSON_EMPTY_OBJ );
    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 1 );
    device = devices.front();

    EXPECT_EQ( json_get_str( device, "deviceId" ), Id1 );
    EXPECT_TRUE( json_get_obj( device, "limits", deviceLimits ) );
    EXPECT_TRUE( deviceLimits.empty() );
}

TEST_F( SwingerShutdownTestSuite, protectUpdateItemWithWithDisabledDevice ) {  // 19
    getDBClient().clear( TempStatisticDB );
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );

    addSecurityDevice();
    const int DeviceHitsLimit = 3;
    const int DeviceInactivityWindow = 2;
    manager.swingerShutdownLimitsSet( JSON( "deviceId"
                                            << "id1"
                                            << "hitsLimit" << DeviceHitsLimit << "inactivityWindow"
                                            << DeviceInactivityWindow ) );
    addDisabledDevice( "id1" );
    auto message = updateSecurityItem( true );
    json_get_bool( message, "protectNotification", true );
}

TEST_F( SwingerShutdownTestSuite, protectUpdateItemWithWithFeatureOff ) {  // 20
    getDBClient().clear( TempStatisticDB );
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );

    addSecurityDevice();
    getFeatureManager().setFeatureStatus( "protect.swinger_shutdown", false );
    const int DeviceHitsLimit = 3;
    const int DeviceInactivityWindow = 2;
    manager.swingerShutdownLimitsSet( JSON( "deviceId"
                                            << "id1"
                                            << "hitsLimit" << DeviceHitsLimit << "inactivityWindow"
                                            << DeviceInactivityWindow ) );
    addDisabledDevice( "id1" );
    auto message = updateSecurityItem( true );
    json_get_bool( message, "protectNotification", true );
}

TEST_F( SwingerShutdownTestSuite, protectUpdateItemWithFirstHit ) {  // 21
    getDBClient().clear( TempStatisticDB );
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );

    addSecurityDevice();
    const int DeviceHitsLimit = 3;
    const int DeviceInactivityWindow = 2;
    manager.swingerShutdownLimitsSet( JSON( "deviceId"
                                            << "id1"
                                            << "hitsLimit" << DeviceHitsLimit << "inactivityWindow"
                                            << DeviceInactivityWindow ) );
    auto message = updateSecurityItem( true );
    JsonObject protect;
    json_get( message, "protect", protect );
    EXPECT_EQ( protect.dump(), "{\"bypass\":\"shutdown\",\"cmsCode\":575}" );
    json_get_bool( message, "protectNotification", false );
}

TEST_F( SwingerShutdownTestSuite, protectUpdateItemWithSwingerShutdownHitsLimitNotMet ) {  // 22
    getDBClient().clear( TempStatisticDB );
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );

    addSecurityDevice();
    const int DeviceHitsLimit = 3;
    const int DeviceInactivityWindow = 2;
    manager.swingerShutdownLimitsSet( JSON( "deviceId"
                                            << "id1"
                                            << "hitsLimit" << DeviceHitsLimit << "inactivityWindow"
                                            << DeviceInactivityWindow ) );

    JsonObject protect;
    auto message = updateSecurityItem( true );
    message = updateSecurityItem( false );
    EXPECT_FALSE( json_get_bool( message, "protectNotification" ) );
    json_get( message, "protect", protect );
    EXPECT_EQ( protect.dump(), "{\"bypass\":\"shutdown\"}" );
}

TEST_F( SwingerShutdownTestSuite, protectUpdateItemWithHitsLimitMet ) {  // 23
    getDBClient().clear( TempStatisticDB );
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );

    addSecurityDevice();
    const int DeviceHitsLimit = 3;
    const int DeviceInactivityWindow = 2;
    manager.swingerShutdownLimitsSet( JSON( "deviceId"
                                            << "id1"
                                            << "hitsLimit" << DeviceHitsLimit << "inactivityWindow"
                                            << DeviceInactivityWindow ) );

    JsonObject protect;
    auto message = updateSecurityItem( true );
    message = updateSecurityItem( false );
    message = updateSecurityItem( true );
    EXPECT_TRUE( json_get_bool( message, "protectNotification" ) );
}

TEST_F( SwingerShutdownTestSuite, setNonSummaryDeviceLimits ) {  // 24
    SwingerShutdown manager( *m_service, getConfig(), TestSyncPeriod );
    auto& dbClient = m_service->getDBClient();
    addSecurityDevice();
    const int DeviceHitsLimitDefault = 3;
    const int DeviceInactivityWindowDefault = 5;
    addLimitsForDevice( Id1, DeviceHitsLimitDefault, DeviceInactivityWindowDefault );

    const int DeviceHitsLimit = 10;
    const int DeviceInactivityWindow = 20;
    manager.swingerShutdownLimitsSet(
        JSON( "deviceId" << Id1 << "hitsLimit" << DeviceHitsLimit << "inactivityWindow" << DeviceInactivityWindow ) );

    auto list = manager.list( JSON_EMPTY_OBJ );
    JsonArray devices;
    EXPECT_TRUE( json_get_arr( list, "devices", devices ) );
    EXPECT_EQ( devices.size(), 1 );
    auto device = devices.front();
    EXPECT_EQ( json_get_str( device, "deviceId" ), Id1 );
    JsonObject deviceLimits;
    EXPECT_TRUE( json_get_obj( device, "limits", deviceLimits ) );
    EXPECT_EQ( json_get_int( deviceLimits, "hitsLimit" ), DeviceHitsLimit );
    EXPECT_EQ( json_get_int( deviceLimits, "inactivityWindow" ), DeviceInactivityWindow );
}

}  // namespace UT
}  // namespace HubPlatform
