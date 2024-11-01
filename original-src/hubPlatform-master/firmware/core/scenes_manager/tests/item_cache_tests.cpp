#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache.h"
#include "database_client/fake_db/fake_db_client.h"

#include "common_utils/cpp14.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "scenes_manager/scenes_engine/controller_interface/controller_interface.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace SceneEngine {
namespace UT {

class MockControllerInterface : public ControllerInterface {
public:
    void subscribeOnItemChanges( const ItemId& itemId ) override {}
    void subscribeOnItemChanges( const ItemId& itemId, SubscriptionCallbackSelector ) override {}
    void unsubscribeFromItemChanges( const ItemId& itemId ) override {}

    void subscribeOnDevicesChanges( const DeviceId& deviceId ) override {}
    void subscribeOnDevicesChanges( const DeviceId& deviceId, SubscriptionCallbackSelector ) override {}
    void unsubscribeFromDevicesChanges( const DeviceId& deviceId ) override {}

    BoolResult isDeviceArmed( const DeviceId& deviceId ) override {
        return {};
    }
    Device getDevice( const DeviceId& deviceId ) override {
        return {};
    }
    Response setDeviceArmed( const DeviceId& deviceId, bool armed ) override {
        return {};
    }

    void setItemValue( const ItemData& data,
                       SendResponseCallback sendResponseCallback = []( const JsonObject& ) {} ) override{};

    RawItems getItemsBy( const JsonObject& condition ) override {
        return RawItems{};
    }
    RawDevices getDevicesBy( const JsonObject& condition ) override {
        return RawDevices{};
    }

    Items getItems( const ItemIds& itemIds ) override {
        return Items{};
    }
    Item getItem( const ItemId& itemId ) override {
        return { { "value", 1 } };
    }

    Response setHouseMode( const HouseModeId& houseModeId ) override {
        return Response{};
    }
};

class MockControllerFactoryInterface final : public SceneEngine::ControllerFactoryInterface {
public:
    using ControllerInterfacePtr = std::shared_ptr< SceneEngine::ControllerInterface >;
    using ControllerId = SceneEngine::ControllerInterface::ControllerId;
    using ControllerInterface = SceneEngine::ControllerInterface;

    ControllerInterfacePtr getControllerInterface( const ControllerId& controllerId ) override {
        return ControllerInterfacePtr( new MockControllerInterface() );
    }
    MOCK_METHOD1( itemAdded, void( const ControllerInterface::Item& item ) );
    MOCK_METHOD1( deviceAdded, void( const ControllerInterface::Device& device ) );
};

// TODO: mock ControllerFactoryInterface
class ItemCacheTests : public ::testing::Test {
public:
    void SetUp() override {
        m_controllerFactoryPtr = std::make_unique< MockControllerFactoryInterface >();
        m_dbClientPtr = std::make_unique< HubPlatform::UT::FakeDbClient >();
        m_itemCachePtr = std::make_unique< SceneEngine::ItemCache >( *m_controllerFactoryPtr, *m_dbClientPtr );
    }

protected:
    std::unique_ptr< SceneEngine::ControllerFactoryInterface > m_controllerFactoryPtr;
    std::unique_ptr< IDbClient > m_dbClientPtr;
    std::unique_ptr< SceneEngine::ItemCache > m_itemCachePtr;
};

TEST_F( ItemCacheTests, hasKey ) {
    ASSERT_TRUE( m_itemCachePtr != nullptr );

    ValueSourceKey key( "controller1", "item1" );
    m_dbClientPtr->insert( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ) );
    m_itemCachePtr->setUp();

    ASSERT_TRUE( m_itemCachePtr->hasKey( key ) );
}

TEST_F( ItemCacheTests, addItem ) {
    ASSERT_TRUE( m_itemCachePtr != nullptr );

    ValueSourceKey key( "controller1", "item1" );
    //    m_dbClientPtr->insert( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ) );
    m_itemCachePtr->setUp();

    ASSERT_TRUE( m_itemCachePtr->addItem( key, Json{ { "key", "value" } } ) );
    ASSERT_TRUE( m_itemCachePtr->hasKey( key ) );
    auto found =
        m_dbClientPtr->findOne( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ), nullptr );
    ASSERT_FALSE( found.empty() ) << "should have a record stored in db";
}

TEST_F( ItemCacheTests, removeItem ) {
    ASSERT_TRUE( m_itemCachePtr != nullptr );

    auto& itemCache = *m_itemCachePtr;
    ValueSourceKey key( "controller1", test_info_->name() );
    ASSERT_TRUE( itemCache.addItem( key, Json{ { "key", "value" } } ) );
    ASSERT_TRUE( itemCache.hasKey( key ) );
    auto found =
        m_dbClientPtr->findOne( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ), nullptr );
    ASSERT_FALSE( found.empty() ) << "should have a record stored in db";

    ASSERT_TRUE( itemCache.removeItem( key ) );
    ASSERT_FALSE( itemCache.hasKey( key ) );

    found = m_dbClientPtr->findOne( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ), nullptr );
    ASSERT_TRUE( found.empty() ) << "should have a record stored in db";
}

TEST_F( ItemCacheTests, updateItem ) {
    ASSERT_TRUE( m_itemCachePtr != nullptr );

    auto& itemCache = *m_itemCachePtr;
    ValueSourceKey key( "controller1", test_info_->name() );
    Json item = Json{ { "key", "value" } };
    ASSERT_TRUE( itemCache.addItem( key, item ) );
    ASSERT_TRUE( itemCache.hasKey( key ) );
    auto recordsFound = m_dbClientPtr->count( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ) );
    ASSERT_TRUE( recordsFound > 0 ) << "should have a record stored in db";

    item[ "key" ] = "updatedValue";
    ASSERT_TRUE( itemCache.updateItem( key, item ) );
    ASSERT_TRUE( itemCache.hasKey( key ) );
}

TEST_F( ItemCacheTests, fetchItem ) {
    ASSERT_TRUE( m_itemCachePtr != nullptr );

    auto& itemCache = *m_itemCachePtr;
    ValueSourceKey key( "controller1", test_info_->name() );
    Json item = Json{ { "key", "value" } };
    ASSERT_TRUE( itemCache.addItem( key, item ) );
    ASSERT_TRUE( itemCache.hasKey( key ) );
    auto recordsFound = m_dbClientPtr->count( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ) );
    ASSERT_TRUE( recordsFound > 0 ) << "should have a record stored in db";

    Json fetchedItem;
    ASSERT_TRUE( itemCache.fetchItem( key, fetchedItem ) );
    ASSERT_EQ( fetchedItem, item );
}

TEST_F( ItemCacheTests, fetchItemShouldAddToStorage ) {
    ASSERT_TRUE( m_itemCachePtr != nullptr );

    auto& itemCache = *m_itemCachePtr;
    ValueSourceKey key( "controller1", test_info_->name() );

    Json fetchedItem;
    ASSERT_TRUE( itemCache.fetchItem( key, fetchedItem ) );
    auto recordsFound = m_dbClientPtr->count( Resources::DbNames::ItemCacheDbName, JSON( "_id" << toDbKey( key ) ) );
    ASSERT_TRUE( recordsFound > 0 ) << "should have a record stored in db";
    Json expectedItem = { { "value", 1 } };
    ASSERT_EQ( fetchedItem, expectedItem );
}

}  // namespace UT
}  // namespace SceneEngine