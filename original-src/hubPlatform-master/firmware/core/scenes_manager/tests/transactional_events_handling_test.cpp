#include "scenes_manager/tests/fakes/test_scenes_service.h"
#include "scenes_manager/tests/fakes/fake_controller_factory.h"
#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_engine/event_container.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache.h"
#include "cored/group_operations/device_item_group_manager.h"

#include "scenes_manager/scenes_engine/operators/operators.h"
#include "scenes_manager/scenes_engine/events/button_event.h"
#include "scenes_manager/scenes_engine/events/user_lock_event.h"
#include "scenes_manager/scenes_engine/events/compare_number_range_event.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"
#include "scenes_manager/scenes_engine/events/compare_strings_event.h"
#include "scenes_manager/scenes_engine/events/compare_values_event.h"
#include "scenes_manager/scenes_engine/events/string_operation_event.h"
#include "scenes_manager/scenes_engine/events/in_array_event.h"
#include "scenes_manager/scenes_engine/events/device_state_event.h"

#include "scenes_manager/scenes_engine/events/network_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_event.h"
#include "scenes_manager/scenes_engine/events/battery_level_event.h"
#include "scenes_manager/scenes_engine/events/battery_state_event.h"
#include "scenes_manager/scenes_engine/events/firmware_update_state_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_value_event.h"
#include "scenes_manager/scenes_engine/events/hotzone_event.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/events/cloud_state_event.h"
#include "scenes_manager/scenes_engine/events/scene_state_event.h"
#include "scenes_manager/scenes_engine/events/group_state_event.h"

#include "scenes_manager/scenes_engine/events/is_item_state_changed_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_changed_event.h"

#include "cored/remote_interface/cored_interface_factory.h"

#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/value_type.h"
#include "cored/model/values/value_factory.h"

#include "service_interfaces/tests/task_loop_test_helper.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include <gtest/gtest.h>

namespace SceneEngine {
namespace UT {

/*

TEST PLAN

DONE    1. Triggers with parent OR
DONE        1.1 compareNumbers
DONE        1.2 compareNumbersRange
DONE        1.3 compareStrings
DONE        1.3 compareValues
DONE        1.4 stringOperation
DONE        1.5 inArray
DONE        1.6 isButtonState
DONE        1.7 isUserLockOperation
DONE        1.8 isHouseModeChangedFrom
DONE        1.9 isHouseModeChangedTo
DONE        1.10 isNetworkState main
DONE        1.11 isNetworkState failed
DONE        1.12 isBatterState
DONE        1.13 isButteryLevel
DONE        1.14 isFirmwareUpdateState
DONE        1.15 isDetectedInHotzone
DONE        1.16 hasAtLeastOneDictionaryValue
DONE        1.17 isDeviceState
DONE        1.18 isItemState
DONE        1.19 isCloudState
DONE        1.20 isSceneState
DONE        1.21 isGroupState
DONE    2. Triggers with AND parent (triggers, which transit to TRUE and then instantly back to FALSE)
DONE        2.1 isItemStateChanged
DONE        2.2 isDictionaryChanged
DONE    3. Combination of different events of different types referencing same item
DONE        3.1 isItemState + isItemStateChanged
DONE        3.2 isItemState + compareNumbers
DONE        3.3 compareNumbers + isItemStateChanged

First part of the tests rely on OR operator.
The operator is used for making sure that updates are performed in transactional manner.
If these events updated one by one, then the scene will run one extra time:
+ setup: OR( event1, event2 ), where "event1" and "event2" depend on the same entity and expect the same value
+ when both events simultaneously switch to TRUE, the operator will be evaluated in following steps:
    + OR( false, false ) => OR( true, false ) - scene run 1
    + OR( true,  false ) => OR( true, true  ) - scene run 2
+ when both events simultaneously switch to FALSE:
    + OR( true,  true ) => OR( false, true  ) - scene run 3, THIS WON'T HAPPEN IF THE EVENTS UPDATED IN 1 TRANSACTION
    + OR( false, true ) => OR( false, false )

*/

namespace {

ItemValueWrapper makeItemValue( int value ) {
    return ItemValueWrapper::fromValueAndType( value, "int" );
}

ItemValueWrapper makeItemValue( const std::string& value ) {
    return ItemValueWrapper::fromValueAndType( value, "string" );
}

class FakeControllerInterface : public HubPlatform::ScenesManager::InternalController {
public:
    FakeControllerInterface( HubPlatform::Services::Service& hostService, std::string valueType )
    : HubPlatform::ScenesManager::InternalController( hostService )
    , m_valueType( std::move( valueType ) ) {}

    Response setDeviceArmed( const DeviceId& deviceId, bool armed ) override {
        throw std::runtime_error( "Unexpected method call" );
    }
    void setItemValue( const ItemData& data, SendResponseCallback sendResponseCallback ) override {
        throw std::runtime_error( "Unexpected method call" );
    }
    Response setHouseMode( const HouseModeId& houseModeId ) override {
        throw std::runtime_error( "Unexpected method call" );
    }

    Device getDevice( const DeviceId& deviceId ) override {
        return {};
    }
    Items getItems( const ItemIds& itemIds ) override {
        return Items( itemIds.size(), makeDefaultValue() );
    }
    Item getItem( const ItemId& itemId ) override {
        return makeDefaultValue();
    }

private:
    Json makeDefaultValue() const {
        using namespace HubPlatform::CoreD::Model;
        auto value = ValueFactory::createDefaultItemValue( ValueType::fromString( m_valueType ) );
        return value->toJson();
    }

    std::string m_valueType;
};

}  // anonymous namespace

class SceneEventsUpdateTest : public testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    template < typename T, typename... Args >
    ItemEventBasePtr makeItemBasedTrigger( Args&&... args );
    template < typename T, typename... Args >
    std::shared_ptr< T > makeTrigger( Args&&... args );
    template < typename T >
    std::shared_ptr< T > makeOperator();

    void addScene( Scene::SceneID sceneId,
                   ActionBlocks&& actionBlocks,
                   EventContainer&& triggers,
                   EventContainer&& exceptions,
                   EventContainer&& globalRestriction ) const;
    void setupScene( AbstractOperator::SharedOperator parentOperator, EventBasePtr event1, EventBasePtr event2 );
    void testItemEventsWithOrOperator( ItemEventBasePtr event1,
                                       ItemEventBasePtr event2,
                                       const ItemValueWrapper& expected,
                                       const ItemValueWrapper& other );
    void checkSceneRuns( int runs );

public:
    std::unique_ptr< HubPlatform::ScenesManager::TestService > m_service;  // any other service might be used instead
    std::future< void > m_taskLoopFuture;

    HubPlatform::CoreDRemoteInterface::CoreDInterfaceUPtr m_coreInterface;
    std::unique_ptr< ControllerFactoryInterface > m_controllerFactory;
    std::unique_ptr< Clock > m_clock;
    std::unique_ptr< ItemCacheBase > m_itemCache;
    std::unique_ptr< HubPlatform::CoreD::DeviceItemGroupManager > m_deviceItemGroupManager;
    std::unique_ptr< StatesRegistry > m_statesRegistry;
    std::unique_ptr< Expressions::ExpressionManager > m_expressions;
    std::unique_ptr< SceneManager > m_scenesManager;

    std::vector< Json > m_broadcasts;

    std::string m_testItemsValueType{ "int" };
    const Scene::SceneID m_sceneId{ "scene_id" };
    const SceneManager::ControllerId m_controllerId{ "controller" };
    const ValueSourceKey m_valueSourceKey{ "controller", "item" };

    EventContainer m_triggers;
    int m_idCounter{ 0 };
};

void SceneEventsUpdateTest::SetUp() {
    m_service = std::make_unique< HubPlatform::ScenesManager::TestService >();
    m_coreInterface = HubPlatform::CoreDRemoteInterface::makeInstance( *m_service );

    auto makeController = [this]( const FakeControllerFactory::ControllerId& ) {
        return std::make_shared< FakeControllerInterface >( *m_service, m_testItemsValueType );
    };
    m_controllerFactory = std::make_unique< FakeControllerFactory >( makeController );

    IDbClient& dbClient = m_service->getDBClient();
    const ValueSourceKey::ControllerId& controllerId = m_valueSourceKey.getControllerId();

    m_clock = std::make_unique< Clock >();
    m_itemCache = std::make_unique< ItemCache >( *m_controllerFactory, dbClient );
    m_itemCache->setUp();
    m_expressions =
        std::make_unique< Expressions::ExpressionManager >( *m_coreInterface, dbClient, *m_itemCache, controllerId );
    m_expressions->setUp();
    m_deviceItemGroupManager = std::make_unique< HubPlatform::CoreD::DeviceItemGroupManager >();
    m_statesRegistry = std::make_unique< StatesRegistry >( m_service->getDBClient(),
                                                           [this]() { return m_clock->greenwichMeanTimePoint(); } );

    auto sendBroadcast = [this]( const JsonObject& message ) { m_broadcasts.push_back( message ); };
    m_scenesManager = std::make_unique< SceneManager >( *m_service,
                                                        *m_coreInterface,
                                                        *m_controllerFactory,
                                                        sendBroadcast,
                                                        *m_expressions,
                                                        *m_itemCache,
                                                        *m_deviceItemGroupManager,
                                                        *m_statesRegistry,
                                                        *m_clock );

    m_taskLoopFuture = HubPlatform::Services::UT::TaskLoopTestHelper{}.runInThread( *m_service );
}

void SceneEventsUpdateTest::TearDown() {
    m_service->stop();
    const std::future_status status = m_taskLoopFuture.wait_for( std::chrono::seconds( 1 ) );
    EXPECT_EQ( status, std::future_status::ready );
}

template < typename T, typename... Args >
ItemEventBasePtr SceneEventsUpdateTest::makeItemBasedTrigger( Args&&... args ) {
    auto event = makeTrigger< T >( std::forward< Args >( args )... );
    // intended usage of addCompareNumbersEvent, in SceneManager all item events will be placed in same collection
    m_triggers.addCompareNumbersEvent( m_valueSourceKey, event );
    return event;
}

template < typename T, typename... Args >
std::shared_ptr< T > SceneEventsUpdateTest::makeTrigger( Args&&... args ) {
    auto event = std::make_shared< T >( std::forward< Args >( args )... );
    event->setEventId( std::to_string( m_idCounter++ ) );
    m_triggers.events.emplace( event->getEventId(), event );
    return event;
}

template < typename T >
std::shared_ptr< T > SceneEventsUpdateTest::makeOperator() {
    auto op = std::make_shared< T >();
    op->setBlockID( std::to_string( m_idCounter++ ) );
    m_triggers.operators.emplace( op->getBlockID(), op );
    return op;
}

void SceneEventsUpdateTest::addScene( Scene::SceneID sceneId,
                                      ActionBlocks&& actionBlocks,
                                      EventContainer&& triggers,
                                      EventContainer&& exceptions,
                                      EventContainer&& globalRestriction ) const {
    auto scenePtr = std::make_shared< Scene >( std::move( sceneId ) );
    const bool isEditing = false;
    const bool added = m_scenesManager->addScene( std::move( scenePtr ),
                                                  std::move( actionBlocks ),
                                                  std::move( triggers ),
                                                  std::move( exceptions ),
                                                  std::move( globalRestriction ),
                                                  isEditing );
    if( !added )
        throw std::runtime_error( "Didn't add the scene, test logic is invalid" );
}

void SceneEventsUpdateTest::setupScene( AbstractOperator::SharedOperator parentOperator,
                                        EventBasePtr event1,
                                        EventBasePtr event2 ) {
    event1->setOperator( parentOperator );
    event2->setOperator( parentOperator );

    m_triggers.events.emplace( event1->getEventId(), event1 );
    m_triggers.events.emplace( event2->getEventId(), event2 );
    m_triggers.operators.emplace( parentOperator->getBlockID(), parentOperator );

    addScene( m_sceneId, ActionBlocks{}, std::move( m_triggers ), EventContainer{}, EventContainer{} );
    HubPlatform::Services::UT::TaskLoopTestHelper{}.waitForTasks( *m_service );  // needed for items events to init
}

void SceneEventsUpdateTest::testItemEventsWithOrOperator( ItemEventBasePtr event1,
                                                          ItemEventBasePtr event2,
                                                          const ItemValueWrapper& expected,
                                                          const ItemValueWrapper& other ) {
    setupScene( makeOperator< OrOperator >(), std::move( event1 ), std::move( event2 ) );

    m_scenesManager->processItemEvent( m_valueSourceKey, expected, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemEvent( m_valueSourceKey, other, JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

void SceneEventsUpdateTest::checkSceneRuns( int runs ) {
    ASSERT_EQ( runs * 2, static_cast< int >( m_broadcasts.size() ) );
    for( int i = 0; i < runs; ++i ) {
        const Json& broadcastStarted = m_broadcasts[ i * 2 ];
        std::cout << broadcastStarted.dump() << std::endl;
        ASSERT_EQ( broadcastStarted[ "scene_id" ], Json( m_sceneId ) );
        ASSERT_EQ( broadcastStarted[ "status" ], Json( "started" ) );

        const Json& broadcastFinished = m_broadcasts[ i * 2 + 1 ];
        std::cout << broadcastFinished.dump() << std::endl;
        ASSERT_EQ( broadcastFinished[ "scene_id" ], Json( m_sceneId ) );
        ASSERT_EQ( broadcastFinished[ "status" ], Json( "finished" ) );
    }

    m_broadcasts.clear();
}

TEST_F( SceneEventsUpdateTest, compareNumbersUnderOr ) {  // 1.1
    m_testItemsValueType = "int";
    const int expected = 10;
    const auto comparator = std::greater< ItemValueWrapper >{};
    auto event1 = makeItemBasedTrigger< CompareNumbersEvent >( makeItemValue( expected ), comparator );
    auto event2 = makeItemBasedTrigger< CompareNumbersEvent >( makeItemValue( expected ), comparator );
    testItemEventsWithOrOperator( event1, event2, makeItemValue( expected + 1 ), makeItemValue( expected - 1 ) );
}

TEST_F( SceneEventsUpdateTest, compareNumberRangeUnderOr ) {  // 1.2
    m_testItemsValueType = "int";
    const auto startValue = makeItemValue( 1 );
    const auto endValue = makeItemValue( 10 );
    const auto comparator = Model::Resources::ValueComparators::Between;
    auto event1 = makeItemBasedTrigger< CompareNumberRangeEvent >( startValue, endValue, comparator );
    auto event2 = makeItemBasedTrigger< CompareNumberRangeEvent >( startValue, endValue, comparator );
    testItemEventsWithOrOperator( event1, event2, startValue, makeItemValue( 100 ) );
}

TEST_F( SceneEventsUpdateTest, compareStringsUnderOr ) {  // 1.3
    m_testItemsValueType = "string";
    const auto expected = makeItemValue( "aa" );
    auto event1 = makeItemBasedTrigger< CompareStringsEvent >( expected, std::equal_to< ItemValueWrapper >{} );
    auto event2 = makeItemBasedTrigger< CompareStringsEvent >( expected, std::equal_to< ItemValueWrapper >{} );
    testItemEventsWithOrOperator( event1, event2, expected, makeItemValue( "bb" ) );
}

TEST_F( SceneEventsUpdateTest, compareValuesUnderOr ) {  // 1.4
    m_testItemsValueType = "string";
    const auto expected = makeItemValue( "aa" );
    const auto comparator = Model::Resources::ValueComparators::Equal;
    auto event1 = makeItemBasedTrigger< CompareValuesEvent >( expected, comparator, expected.getValueType() );
    auto event2 = makeItemBasedTrigger< CompareValuesEvent >( expected, comparator, expected.getValueType() );
    testItemEventsWithOrOperator( event1, event2, expected, makeItemValue( "bb" ) );
}

TEST_F( SceneEventsUpdateTest, stringOperaionUnderOr ) {  // 1.5
    m_testItemsValueType = "string";
    const auto expected = makeItemValue( "aa" );
    auto event1 = makeItemBasedTrigger< StringOperationEvent >( expected, Model::Resources::StringOperations::Begin );
    auto event2 = makeItemBasedTrigger< StringOperationEvent >( expected, Model::Resources::StringOperations::Begin );
    testItemEventsWithOrOperator( event1, event2, expected, makeItemValue( "bb" ) );
}

TEST_F( SceneEventsUpdateTest, inArrayUnderOr ) {  // 1.6
    m_testItemsValueType = "int";
    const std::vector< ItemValueWrapper > expected{ makeItemValue( 10 ), makeItemValue( 20 ), makeItemValue( 3 ) };
    auto event1 = makeItemBasedTrigger< InArrayEvent >( expected, Model::Resources::ArrayOperations::In );
    auto event2 = makeItemBasedTrigger< InArrayEvent >( expected, Model::Resources::ArrayOperations::In );
    testItemEventsWithOrOperator( event1, event2, expected.front(), makeItemValue( 40 ) );
}

TEST_F( SceneEventsUpdateTest, buttonStateUnderOr ) {  // 1.7
    using namespace HubPlatform::CoreD::Model;
    using namespace HubPlatform::CoreD::Model::Resources::ButtonStateValueFields;

    m_testItemsValueType = ValueType( ValueType::Enum::CameraHotzone ).toString();

    const std::string expected = "released";
    auto event1 = makeItemBasedTrigger< ButtonStateEvent >( expected );
    auto event2 = makeItemBasedTrigger< ButtonStateEvent >( expected );

    const auto expectedValue =
        ItemValueWrapper::fromValueAndType( Json{ { ButtonStateFieldName, expected } }, m_testItemsValueType );
    const auto otherValue =
        ItemValueWrapper::fromValueAndType( Json{ { ButtonStateFieldName, "idle" } }, m_testItemsValueType );
    testItemEventsWithOrOperator( event1, event2, expectedValue, otherValue );
}

TEST_F( SceneEventsUpdateTest, userLockUnderOr ) {  // 1.8
    using namespace HubPlatform::CoreD::Model;
    using namespace HubPlatform::CoreD::Model::Resources::UserLockOperation;

    m_testItemsValueType = ValueType( ValueType::Enum::UserLockOperation ).toString();

    const std::string expectedAction = "action";
    const int expectedUserId = 7;
    auto event1 = makeItemBasedTrigger< UserLockEvent >( "", expectedAction, expectedUserId );
    auto event2 = makeItemBasedTrigger< UserLockEvent >( "", expectedAction, expectedUserId );

    const auto expectedValue = Json{ { Fields::Action, expectedAction }, { Fields::UserId, expectedUserId } };
    const auto otherValue = Json{ { Fields::Action, expectedAction }, { Fields::UserId, 10 } };
    testItemEventsWithOrOperator( event1,
                                  event2,
                                  ItemValueWrapper::fromValueAndType( expectedValue, m_testItemsValueType ),
                                  ItemValueWrapper::fromValueAndType( otherValue, m_testItemsValueType ) );
}

TEST_F( SceneEventsUpdateTest, houseModeChangedFromUnderOr ) {  // 1.9
    const HouseModeEvent::Modes expectedModes{ "1" };
    auto event1 = makeTrigger< HouseModeEvent >( expectedModes );
    auto event2 = makeTrigger< HouseModeEvent >( expectedModes );
    m_triggers.houseModeChangedFromEvents.emplace_back( m_controllerId, event1 );
    m_triggers.houseModeChangedFromEvents.emplace_back( m_controllerId, event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processHouseModeChangedFromEvent( m_controllerId, *expectedModes.begin(), JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processHouseModeChangedFromEvent( m_controllerId, "2", JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, houseModeChangedToUnderOr ) {  // 1.10
    const HouseModeEvent::Modes expectedModes{ "2" };
    auto event1 = makeTrigger< HouseModeEvent >( expectedModes );
    auto event2 = makeTrigger< HouseModeEvent >( expectedModes );
    m_triggers.houseModeChangedToEvents.emplace_back( m_controllerId, event1 );
    m_triggers.houseModeChangedToEvents.emplace_back( m_controllerId, event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processHouseModeChangedToEvent( m_controllerId, *expectedModes.begin(), JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processHouseModeChangedToEvent( m_controllerId, "1", JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, mainNetworkInterfaceChangedUnderOr ) {  // 1.11
    const std::string expected = "interface";
    auto event1 = makeTrigger< NetworkMainStateEvent >( expected );
    auto event2 = makeTrigger< NetworkMainStateEvent >( expected );
    m_triggers.networkMainStateEvents.emplace_back( event1 );
    m_triggers.networkMainStateEvents.emplace_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processMainNetworkInterfaceChanged( expected, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processMainNetworkInterfaceChanged( "other", JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, networkInterfaceStateChangedUnderOr ) {  // 1.12
    const std::string interfaceId = "interface";
    auto event1 = makeTrigger< NetworkFailedStateEvent >( interfaceId );
    auto event2 = makeTrigger< NetworkFailedStateEvent >( interfaceId );
    m_triggers.networkFailedStateEvents.emplace_back( event1 );
    m_triggers.networkFailedStateEvents.emplace_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processNetworkInterfaceStateChanged( interfaceId, false, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processNetworkInterfaceStateChanged( interfaceId, true, JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, batteryStateUnderOr ) {
    const std::string expected = "state";
    auto event1 = makeTrigger< BatteryStateEvent >( expected );
    auto event2 = makeTrigger< BatteryStateEvent >( expected );
    m_triggers.batteryStateEvents.push_back( event1 );
    m_triggers.batteryStateEvents.push_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processBatteryStateEvent( expected, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processBatteryStateEvent( "other", JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, batteryLevelUnderOr ) {  // 1.13
    const int expected = 50;
    auto event1 = makeTrigger< BatteryLevelEvent >( std::greater< ItemValueWrapper >{}, makeItemValue( expected ) );
    auto event2 = makeTrigger< BatteryLevelEvent >( std::greater< ItemValueWrapper >{}, makeItemValue( expected ) );
    m_triggers.batteryLevelEvents.push_back( event1 );
    m_triggers.batteryLevelEvents.push_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    JsonObject message = JSON_EMPTY_OBJ;
    m_scenesManager->processBatteryLevelEvent( expected + 1, message );
    checkSceneRuns( 1 );
    m_scenesManager->processBatteryLevelEvent( expected - 1, message );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, firmwareUpdateStateUnderOr ) {  // 1.14
    const std::string expected = "started";
    auto event1 = makeTrigger< FirmwareUpdateStateEvent >( expected );
    auto event2 = makeTrigger< FirmwareUpdateStateEvent >( expected );
    m_triggers.firmwareUpdateStateEvents.push_back( event1 );
    m_triggers.firmwareUpdateStateEvents.push_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processFirmwareUpdateStateEvent( expected, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processFirmwareUpdateStateEvent( "done", JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, hotzoneEventsUnderOr ) {  // 1.15
    const auto hotzone = HotzoneEvent::Hotzone( "hotzone" );
    const auto policy = HotzoneEvent::MatchPolicy::Any;
    auto event1 = makeTrigger< HotzoneEvent >( HotzoneEvent::Hotzones{ hotzone }, policy, HotzoneEvent::Filters{} );
    auto event2 = makeTrigger< HotzoneEvent >( HotzoneEvent::Hotzones{ hotzone }, policy, HotzoneEvent::Filters{} );
    m_triggers.hotzoneEvents.emplace_back( m_valueSourceKey, event1 );
    m_triggers.hotzoneEvents.emplace_back( m_valueSourceKey, event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    using namespace HubPlatform::CoreD::Model;

    auto value = ValueFactory::createDefaultItemValue( ValueType::Enum::CameraHotzone );
    Json hotzoneItemValue;
    hotzoneItemValue[ hotzone ] = value->toJson();
    hotzoneItemValue[ hotzone ][ HotzoneFields::Detected ][ HotzoneFields::Objects ] = JSON_ARRAY_( "obj" );
    m_scenesManager->processIsDetectedInHotzoneEvent( m_valueSourceKey, hotzoneItemValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );

    hotzoneItemValue[ hotzone ][ HotzoneFields::Detected ][ HotzoneFields::Objects ] = JSON_EMPTY_ARR;
    m_scenesManager->processIsDetectedInHotzoneEvent( m_valueSourceKey, hotzoneItemValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, dictionaryValueEventsUnderOr ) {  // 1.16
    const auto expected = DictionaryValueEvent::ElementValue( true );
    auto event1 = makeTrigger< DictionaryValueEvent >( expected );
    auto event2 = makeTrigger< DictionaryValueEvent >( expected );
    m_triggers.dictionaryValueEvents.emplace_back( m_valueSourceKey, event1 );
    m_triggers.dictionaryValueEvents.emplace_back( m_valueSourceKey, event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    auto operation = HubPlatform::CoreD::Model::Resources::DictionaryOperations::Added;
    m_scenesManager->processItemDictionaryEvent( m_valueSourceKey, operation, expected, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemDictionaryEvent(
        m_valueSourceKey, operation, DictionaryValueEvent::ElementValue( false ), JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isDeviceStateUnderOr ) {  // 1.17
    const std::string expectedParam = "param";
    const std::string expectedValue = "value";
    const DeviceStateEvent::DeviceStateParams expectedParams{ { expectedParam, Json( expectedValue ) } };
    auto event1 = makeTrigger< DeviceStateEvent >( expectedParams );
    auto event2 = makeTrigger< DeviceStateEvent >( expectedParams );
    m_triggers.deviceStateEvents.emplace_back( m_valueSourceKey, event1 );
    m_triggers.deviceStateEvents.emplace_back( m_valueSourceKey, event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processDeviceStateEvent(
        m_valueSourceKey, Json{ { expectedParam, expectedValue } }, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processDeviceStateEvent( m_valueSourceKey, Json{ { expectedParam, "other" } }, JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isItemStateUnderOr ) {  // 1.18
    m_testItemsValueType = "int";
    const auto expectedValue = makeItemValue( 1 );
    const auto& itemId = m_valueSourceKey.getSourceId();
    auto event1 = makeTrigger< ItemEvent >( itemId, std::make_shared< ItemValueProvider >( expectedValue ) );
    auto event2 = makeTrigger< ItemEvent >( itemId, std::make_shared< ItemValueProvider >( expectedValue ) );
    m_triggers.addItemEvent( m_valueSourceKey, event1 );
    m_triggers.addItemEvent( m_valueSourceKey, event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processItemEvent( m_valueSourceKey, expectedValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemEvent( m_valueSourceKey, makeItemValue( 2 ), JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isCloudStateUnderOr ) {  // 1.19
    auto event1 = makeTrigger< CloudStateEvent >( CloudStateEvent::State::Connected );
    auto event2 = makeTrigger< CloudStateEvent >( CloudStateEvent::State::Connected );
    m_triggers.cloudStateEvents.push_back( event1 );
    m_triggers.cloudStateEvents.push_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processCloudConnectionState( true, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processCloudConnectionState( false, JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isSceneStateUnderOr ) {  // 1.20
    const Scene::SceneID sceneId = "a";
    addScene( sceneId, ActionBlocks{}, EventContainer{}, EventContainer{}, EventContainer{} );

    auto event1 = makeTrigger< SceneStateEvent >( sceneId, SceneStateEvent::Action::SceneDisabled );
    auto event2 = makeTrigger< SceneStateEvent >( sceneId, SceneStateEvent::Action::SceneDisabled );
    m_triggers.sceneStateEvents.push_back( event1 );
    m_triggers.sceneStateEvents.push_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->setSceneEnabled( sceneId, false );
    checkSceneRuns( 1 );
    m_scenesManager->setSceneEnabled( sceneId, true );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isGroupStateUnderOr ) {  // 1.21
    const Scene::SceneID sceneId = "a";
    std::string groupId;

    const auto expectedValue = makeItemValue( 1 );

    {
        const auto& itemId = m_valueSourceKey.getSourceId();
        auto referencedEvent =
            makeTrigger< ItemEvent >( itemId, std::make_shared< ItemValueProvider >( expectedValue ) );
        groupId = referencedEvent->getEventId();

        EventContainer triggers;
        triggers.addItemEvent( m_valueSourceKey, referencedEvent );
        triggers.events.emplace( groupId, referencedEvent );
        addScene( sceneId, ActionBlocks{}, std::move( triggers ), EventContainer{}, EventContainer{} );
    }

    auto event1 = makeTrigger< GroupStateEvent >( sceneId, groupId, GroupStateEvent::State::True );
    auto event2 = makeTrigger< GroupStateEvent >( sceneId, groupId, GroupStateEvent::State::True );
    m_triggers.groupStateEvents.push_back( event1 );
    m_triggers.groupStateEvents.push_back( event2 );
    setupScene( makeOperator< OrOperator >(), event1, event2 );

    m_scenesManager->processItemEvent( m_valueSourceKey, expectedValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemEvent( m_valueSourceKey, makeItemValue( 2 ), JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isItemStateChangedUnderAnd ) {  // 2.1
    m_testItemsValueType = "int";
    const auto expectedEndValue = makeItemValue( 10 );
    auto event1 = makeTrigger< ItemStateChangedEvent >( nullptr, makeItemValueProvider( expectedEndValue ) );
    auto event2 = makeTrigger< ItemStateChangedEvent >( nullptr, makeItemValueProvider( expectedEndValue ) );
    m_triggers.addItemStateChangedEvent( m_valueSourceKey, event1 );
    m_triggers.addItemStateChangedEvent( m_valueSourceKey, event2 );
    setupScene( makeOperator< AndOperator >(), event1, event2 );

    m_scenesManager->processItemEvent( m_valueSourceKey, expectedEndValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
}

TEST_F( SceneEventsUpdateTest, dictionaryChangedEventUnderAnd ) {  // 2.2
    const std::string key = "key";
    const std::string operation = "op";
    auto event1 = makeTrigger< DictionaryChangedEvent >( "", key, operation );
    auto event2 = makeTrigger< DictionaryChangedEvent >( "", key, operation );
    m_triggers.dictionaryChangedEvents.emplace_back( m_valueSourceKey, event1 );
    m_triggers.dictionaryChangedEvents.emplace_back( m_valueSourceKey, event2 );
    setupScene( makeOperator< AndOperator >(), event1, event2 );

    m_scenesManager->processIsDictionaryChangedEvent( m_valueSourceKey, operation, key, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
}

TEST_F( SceneEventsUpdateTest, isItemStateAndIsItemStateChanged ) {  // 3.1
    m_testItemsValueType = "int";
    const auto expectedValue = makeItemValue( 10 );
    const auto& itemId = m_valueSourceKey.getSourceId();
    auto event1 = makeTrigger< ItemStateChangedEvent >( nullptr, makeItemValueProvider( expectedValue ) );
    auto event2 = makeTrigger< ItemEvent >( itemId, std::make_shared< ItemValueProvider >( expectedValue ) );
    m_triggers.addItemStateChangedEvent( m_valueSourceKey, event1 );
    m_triggers.addItemEvent( m_valueSourceKey, event2 );
    setupScene( makeOperator< AndOperator >(), event1, event2 );

    m_scenesManager->processItemEvent( m_valueSourceKey, expectedValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemEvent( m_valueSourceKey, makeItemValue( 2 ), JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, isItemStateAndCompareNumbers ) {  // 3.2
    m_testItemsValueType = "int";
    const auto expectedValue = makeItemValue( 10 );
    auto event1 = makeItemBasedTrigger< CompareNumbersEvent >( expectedValue, std::equal_to< ItemValueWrapper >{} );
    auto event2 = makeTrigger< ItemEvent >( m_valueSourceKey.getSourceId(),
                                            std::make_shared< ItemValueProvider >( expectedValue ) );
    m_triggers.addItemEvent( m_valueSourceKey, event2 );
    setupScene( makeOperator< AndOperator >(), event1, event2 );

    m_scenesManager->processItemEvent( m_valueSourceKey, expectedValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemEvent( m_valueSourceKey, makeItemValue( 2 ), JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

TEST_F( SceneEventsUpdateTest, compareNumbersAndIsItemState ) {  // 3.3
    m_testItemsValueType = "int";
    const auto expectedValue = makeItemValue( 10 );
    auto event1 = makeItemBasedTrigger< CompareNumbersEvent >( expectedValue, std::equal_to< ItemValueWrapper >{} );
    auto event2 = makeTrigger< ItemStateChangedEvent >( nullptr, makeItemValueProvider( expectedValue ) );
    m_triggers.addItemStateChangedEvent( m_valueSourceKey, event2 );
    setupScene( makeOperator< AndOperator >(), event1, event2 );

    m_scenesManager->processItemEvent( m_valueSourceKey, expectedValue, JSON_EMPTY_OBJ );
    checkSceneRuns( 1 );
    m_scenesManager->processItemEvent( m_valueSourceKey, makeItemValue( 2 ), JSON_EMPTY_OBJ );
    checkSceneRuns( 0 );
}

}  // namespace UT
}  // namespace SceneEngine
