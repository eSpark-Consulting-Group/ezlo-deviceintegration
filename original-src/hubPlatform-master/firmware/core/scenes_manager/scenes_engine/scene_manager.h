#pragma once

#include "scenes_manager/scenes_engine/scene.h"

#include "scenes_manager/scenes_engine/events/sun_state_event.h"
#include "scenes_manager/scenes_engine/events/interval_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_event.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/events/date_event.h"
#include "scenes_manager/scenes_engine/events/date_range_event.h"
#include "scenes_manager/scenes_engine/events/compare_numbers_event.h"
#include "scenes_manager/scenes_engine/events/button_event.h"
#include "scenes_manager/scenes_engine/events/battery_state_event.h"
#include "scenes_manager/scenes_engine/events/battery_level_event.h"
#include "scenes_manager/scenes_engine/events/user_lock_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_value_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_changed_event.h"
#include "scenes_manager/scenes_engine/events/hotzone_event.h"
#include "scenes_manager/scenes_engine/events/network_state_event.h"
#include "scenes_manager/scenes_engine/events/scene_state_event.h"
#include "scenes_manager/scenes_engine/events/cloud_state_event.h"
#include "scenes_manager/scenes_engine/events/group_state_event.h"
#include "scenes_manager/scenes_engine/events/device_state_event.h"
#include "scenes_manager/scenes_engine/events/firmware_update_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_alarm_phase_range_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_switch_to_range_state_event.h"
#include "scenes_manager/scenes_engine/actions/scene_context_manager.h"
#include "scenes_manager/scenes_engine/events/feature_state_event.h"

#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/date_scheduler/schedule_handler.h"
#include "scenes_manager/scenes_engine/environment.h"
#include "scenes_manager/scenes_engine/action_run_requests_container.h"
#include "scenes_manager/scenes_engine/scene_graph_cycles_detector.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"

#include "scenes_manager/scenes_engine/controller_interface/controller_factory_interface.h"
#include "scenes_manager/scenes_engine/actions/scene_action_execute_factory.h"
#include "scenes_manager/scenes_engine/actions/scene_context_info.h"
#include "service_interfaces/service.h"
#include "service_interfaces/task_loop_timer.h"
#include "cored/remote_interface/cored_interface.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/group_operations/device_item_group_manager.h"
#include "cored/model/cored_feature.h"

#include "json_utils/json_wrapper.h"
#include "scenes_manager/scenes_processor/actions/action_interface.h"
#include <vector>
#include <set>
#include <mutex>
#include <functional>
#include <tuple>

namespace SceneEngine {
namespace UT {
class SceneManagerTests;
}

struct TimeScene {
    Scene::SceneID id;
    std::string name;
    std::string method;
    std::string date;
    bool operator<( const TimeScene& a ) const {
        return date < a.date;
    }
};

class IClock;
class ItemCacheBase;
class StatesRegistry;
class EventContainer;

// TODO: remove the singleton idea. this object should be owned by service !!!
class SceneManager {
public:
    // predicate of iterating all scenes in pool
    using ControllerId = std::string;
    using BlockId = std::string;
    using SceneFuncProcT = std::function< bool( const SceneSharedPtr& sceneObj ) >;
    using TimeScenesList = std::multiset< TimeScene >;
    using BroadcastCallback = std::function< void( const JsonObject& message ) >;
    using DeviceId = HubPlatform::CoreDRemoteInterface::CoreDInterface::DeviceID;
    using ItemId = HubPlatform::CoreDRemoteInterface::CoreDInterface::ItemID;
    using SendFinishCallbackPtr = std::shared_ptr< HubPlatform::ScenesManager::ActionInterface::SendFinishCallback >;

    SceneManager( HubPlatform::Services::Service& hostService,
                  HubPlatform::CoreDRemoteInterface::CoreDInterface& coreInterface,
                  ControllerFactoryInterface& controllerFactoryInterface,
                  BroadcastCallback sendSceneProgressBroadcast,
                  Expressions::ExpressionManager& expressions,
                  ItemCacheBase& itemCache,
                  HubPlatform::CoreD::DeviceItemGroupManager& deviceItemGroupManager,
                  StatesRegistry& statesRegistry,
                  IClock& clock );

    ~SceneManager();

    void tearDown();

    bool addScene( SceneSharedPtr scenePtr,
                   ActionBlocks&& actions,
                   EventContainer&& triggers,
                   EventContainer&& exceptions,
                   EventContainer&& globalRestriction,
                   bool isEditing );

    bool setSceneEnabled( const Scene::SceneID& id, bool enabled );
    bool setSceneGroupEnabled( const std::string& group_id, bool enabled );
    void setSceneBlockEnabled( const Scene::SceneID& id, const BlockId& blockId, bool enabled );
    bool deleteScene( const Scene::SceneID& sceneId, bool isGlobal, bool isEditing = false );
    SceneSharedPtr findSceneById( const Scene::SceneID& id ) const;

    void setClock( IClock& clock );
    void onSystemTimeChanged();

    bool processDeviceArmed( const ValueSourceKey::ControllerId& controllerId,
                             const std::string& deviceId,
                             bool armed,
                             const JsonObject& message );

    void processItemEvent( const ValueSourceKey& itemSearchKey,
                           const ItemEventBase::ItemValue& itemValue,
                           const JsonObject& message );
    void processExtendedItemEvent( const ValueSourceKey& itemSearchKey,
                                   const Json& oldItemValue,
                                   const Json& newItemValue );
    void processItemDictionaryEvent( const ValueSourceKey& itemSearchKey,
                                     const DictionaryValueEvent::Operation& operation,
                                     const DictionaryValueEvent::ElementValue& element,
                                     const JsonObject& message );
    void processIsDictionaryChangedEvent( const ValueSourceKey& itemSearchKey,
                                          const DictionaryChangedEvent::Operation& operation,
                                          const DictionaryChangedEvent::KeyValue& keyValue,
                                          const JsonObject& message );
    void processDeviceItemGroupEvent( const Json& itemUpdatedMessage, const ItemEventBase::ItemValue& itemValue );
    void processIsDeviceItemGroupEvent( const Json& itemUpdatedMessage );
    void processIsItemStateGroupEvent( const Json& itemUpdatedMessage,
                                       const ItemEventBase::ItemValue& itemValue,
                                       ItemEvent::ArmedState armed );
    void processIsDetectedInHotzoneEvent( const ValueSourceKey& itemSearchKey,
                                          const HotzoneEvent::HotzonesItemValue& itemValue,
                                          const JsonObject& message );
    void processDeviceStateEvent( const ValueSourceKey& itemSearchKey,
                                  const JsonObject& deviceInfo,
                                  const JsonObject& broadcastMessage );
    void processDeviceGroupStateEvent( const ValueSourceKey& itemSearchKey,
                                       const JsonObject& deviceInfo,
                                       const std::string& deviceId,
                                       const std::string& category,
                                       const std::string& subcategory,
                                       const JsonObject& message );
    void processHouseModeChangedToEvent( const ControllerId& controllerId,
                                         const std::string& mode,
                                         const JsonObject& message );
    void processHouseModeChangedFromEvent( const ControllerId& controllerId,
                                           const std::string& mode,
                                           const JsonObject& message );
    void processHouseAlarmRangeEvent( const ControllerId& controllerId,
                                      const std::string& phase,
                                      const std::string& status,
                                      const JsonObject& message );
    void processHouseSwitchToRangeEvent( const ControllerId& controllerId,
                                         const std::string& phase,
                                         const std::string& status,
                                         const JsonObject& message );
    void processMainNetworkInterfaceChanged( const std::string& interfaceId, const JsonObject& message );
    void processBatteryStateEvent( const std::string& state, const JsonObject& message );
    void processBatteryLevelEvent( int level, const JsonObject& message );
    void processNetworkInterfaceStateChanged( const std::string& interfaceId,
                                              bool isConnected,
                                              const JsonObject& message );
    void processCloudConnectionState( bool isConnected, const JsonObject& message );
    void processFirmwareUpdateStateEvent( const std::string& state, const JsonObject& message );

    void resetSceneLatches( const Scene::SceneID& sceneId );
    void resetAllChildLatches( const Scene::SceneID& sceneId, const BlockId& blockId );
    void resetLatch( const Scene::SceneID& sceneId, const BlockId& blockId );

    void foreachScenes( SceneFuncProcT func ) const;

    TimeScenesList getTimeScenesList();
    Json buildSceneStatus( const Scene::SceneID& sceneId ) const;

    bool isSceneExist( const Scene::SceneID& id ) const;

    // Indicates whether a scene with a given ID has both "then" and "else" action blocks

    bool runScene( const Scene::SceneID& sceneId,
                   bool isGlobal,
                   RunSceneActionBlockType block = RunSceneActionBlockType::Empty,
                   const std::string& groupId = {} );

    bool stopScene( const Scene::SceneID& sceneId,
                    bool isGlobal,
                    RunSceneActionBlockType block = RunSceneActionBlockType::Empty );

    void processExpressionValueChanged( const std::string& expressionName,
                                        const Json& expressionValue,
                                        const JsonObject& message );
    bool checkActionGroupID( const Scene::SceneID& sceneId,
                             const RunSceneActionBlockType& block,
                             const std::string& groupId );
    bool checkActionGroupIDForAll( const RunSceneActionBlockType& block, const std::string& groupId );
    HubPlatform::Services::TaskLoop& getTaskLoop();
    ControllerInterface::Item getItem( const ValueSourceKey& searchKey );

    ItemEvent::ItemValue fetchSourceValue( const ValueSourceKey& key );
    TransactionalEventsUpdater makeEventsUpdater( const std::string& sceneContextId );
    void saveHubBackupRunCallbackPtr(
        const HubPlatform::ScenesManager::ActionInterface::Action& action,
        const HubPlatform::ScenesManager::ActionInterface::ActionExecutorPtr& actionExecutorPtr );
    void setHubBackupRunState( bool flag );
    void handleHubBackupCreateProgress( const JsonObject& message );
    void handleHubBackupRestoreProgress( const JsonObject& message );
    void processFeatureStateEvent( const std::string& controllerId,
                                   const std::string& name,
                                   bool status,
                                   const JsonObject& message );

    std::string createSceneContext( const JsonObject& message );

private:
    friend UT::SceneManagerTests;

private:  // types
    using Counter = int;
    using HouseModeEvents = std::multimap< ControllerId, HouseModeEventPtr >;
    using HouseModeAlarmPhaseRangeEvents = std::multimap< ControllerId, HouseModeAlarmPhaseRangeEventPtr >;
    using HouseModeSwitchToRangeEvents = std::multimap< ControllerId, HouseModeSwitchToRangeEventPtr >;
    using DictionaryValueEvents = std::multimap< ValueSourceKey, DictionaryValueEventPtr >;
    using DictionaryChangedEvents = std::multimap< ValueSourceKey, DictionaryChangedEventPtr >;
    using HotzoneEvents = std::multimap< ValueSourceKey, HotzoneEventPtr >;
    using AllItemsHotzoneEvents = std::multimap< ControllerId, HotzoneEventPtr >;
    using ItemArmedEvents = std::multimap< ValueSourceKey, ItemEventPtr >;
    using ItemEvents = std::multimap< ValueSourceKey, ItemEventBasePtr >;
    using DeviceItemGroupEvents = std::multimap< ValueSourceKey, ItemEventPtr >;
    using NetworkFailedStateEvents = std::vector< NetworkFailedStateEventPtr >;
    using NetworkMainStateEvents = std::vector< NetworkMainStateEventPtr >;
    using BatteryStateEvents = std::vector< BatteryStateEventPtr >;
    using BatteryLevelEvents = std::vector< BatteryLevelEventPtr >;
    using SceneStateEvents = std::vector< SceneStateEventPtr >;
    using GroupStateEvents = std::vector< GroupStateEventPtr >;
    using CloudStateEvents = std::vector< CloudStateEventPtr >;
    using FirmwareUpdateStateEvents = std::vector< FirmwareUpdateStateEventPtr >;
    using DeviceStateEvents = std::multimap< ValueSourceKey, DeviceStateEventPtr >;
    using FeatureStateEvents = std::vector< FeatureStateEventPtr >;

    using EventMap = std::map< BlockId, EventBasePtr >;
    using Operators = std::map< BlockId, AbstractOperator::SharedOperator >;

    SendFinishCallbackPtr m_sendFinishCallbackPtr;
    std::queue< SendFinishCallbackPtr > m_callbackPtrQueue;

    struct ItemToDeviceInfo {
        ItemEvent::DeviceID deviceId;
        Counter counter;

        ItemToDeviceInfo()
        : counter( 0 ) {}
    };

    // There can be records with non-empty scenePtr and all other fields empty
    struct SceneData {
        SceneSharedPtr scenePtr;
        SceneActionExecutePtr actionExecutorPtr;

        EventMap triggerEvents;
        Operators triggerOperators;
        EventMap exceptionEvents;
        Operators exceptionOperators;
        EventMap globalRestrictionEvents;
        Operators globalRestrictionOperators;

    public:
        bool hasError() const;
        void setError( Json value );
        Json getError() const;

        EventBasePtr findEvent( const BlockId& blockId ) const;
        AbstractOperator::SharedOperator findOperator( const BlockId& blockId ) const;

        using OperatorCallback = std::function< void( const AbstractOperator::SharedOperator& ) >;
        void forEachOperator( const OperatorCallback& callback ) const;
    };

    struct ItemArmedEventData {
        ItemEventPtr eventPtr;
        ItemEvent::ItemValue value;
    };

    using Scenes = std::map< Scene::SceneID, SceneData >;
    using ItemArmedEventDataList = std::vector< ItemArmedEventData >;

    using SceneStateEventHandler = void ( SceneManager::* )( const Scene::SceneID&, AbstractOperator::OperatorState );
    using OperatorStateEventHandler = void ( SceneManager::* )( const Scene::SceneID&,
                                                                const BlockId&,
                                                                AbstractOperator::OperatorState,
                                                                AbstractOperator::OperatorState );

private:  // method
    template < typename T >
    void processSceneStateEvents( const Scene::SceneID& sceneId, T&& state );
    void processGroupStateEvent( const Scene::SceneID& sceneId,
                                 const BlockId& blockId,
                                 AbstractOperator::OperatorState oldState,
                                 AbstractOperator::OperatorState newState );
    bool processItemArmedEvent( TransactionalEventsUpdater& updater,
                                const ValueSourceKey& itemSearchKey,
                                const ItemEvent::ItemValue& itemValue );

    AbstractOperator::SharedOperator findBlockOperator( const Scene::SceneID& sceneId, const BlockId& blockId ) const;

    DeviceId getDeviceId( const JsonObject& item );
    ItemEvent getItemEvent( const ItemEvent::ItemID& itemId, const JsonObject& item );
    JsonObject getItemValue( const JsonObject& item );
    bool hasDictionaryElement( const JsonObject& item, const JsonObject& elementValue );
    bool isItemEmpty( const JsonObject& item );

    void handleSceneProgressBroadcast( const JsonObject& message );

    void addValueDrivenEvents( EventContainer& events, bool initSceneSubtree );
    void addItemStateEvents( const EventContainer& events, bool initSceneSubtree );
    void addDeviceStateEvents( const EventContainer& events, bool initSceneSubtree );
    void addDictionaryValueEvents( EventContainer& events );
    void addHotzoneEvents( EventContainer& events );
    void addNonItemEvents( EventContainer& events, bool initSceneSubtree );

    void registerValueDrivenEvent( const ValueSourceKey& searchKey, ItemEventBasePtr itemEvent, bool initSceneSubtree );
    void initStateRegistryRecords( const Scene::SceneID& sceneId, const EventMap& events, const Operators& operators );

    void cleanSceneDependencies( const Scene::SceneID& sceneId );
    void deleteDictionaryValueEvent( const Scene::SceneID& sceneId );
    void deleteIsDictionaryChangedEvent( const Scene::SceneID& sceneId );
    void deleteHotzoneEvent( const Scene::SceneID& sceneId );
    void deleteItemArmedEvent( const Scene::SceneID& sceneId );
    void deleteDeviceItemGroupEvents( const Scene::SceneID& sceneId );
    void deleteValueDrivenEvent( const Scene::SceneID& sceneId );
    void deleteHouseModeEvent( HouseModeEvents& events, const Scene::SceneID& sceneId );
    void deleteHouseModeAlarmRangeEvent( HouseModeAlarmPhaseRangeEvents& events, const Scene::SceneID& sceneId );
    void deleteHouseModeSwitchToRangeEvent( HouseModeSwitchToRangeEvents& events, const Scene::SceneID& sceneId );
    void deleteNetworkMainStateEvent( const Scene::SceneID& sceneId );
    void deleteNetworkFailedStateEvent( const Scene::SceneID& sceneId );
    void deleteSceneStateEvent( const Scene::SceneID& sceneId );
    void deleteGroupStateEvent( const Scene::SceneID& sceneId );
    void deleteCloudStateEvent( const Scene::SceneID& sceneId );
    void deleteBatteryStateEvent( const Scene::SceneID& sceneId );
    void deleteBatteryLevelEvent( const Scene::SceneID& sceneId );
    void deleteFirmwareUpdateEvent( const Scene::SceneID& sceneId );
    void deleteDeviceStateEvent( const Scene::SceneID& sceneId );

    void onItemChange( const ControllerInterface::Item& item, ItemEventBasePtr itemEvent, bool updateSilently );
    void onExpressionChange( const ItemEvent::ItemValue& value, ItemEventBasePtr itemEvent );

    void checkRunSceneActionGroupID( const ActionBlocks& actions );
    void checkSceneIdActionArg( const Scene::SceneID& recursiveSceneId,
                                const ActionBlocks& actions,
                                const std::string& actionNameToCheck );

    void checkSceneStateEvents( const SceneSharedPtr& scenePtr, const EventContainer& events );
    void checkGroupStateEvents( const SceneSharedPtr& scenePtr, const GroupStateEvents& events ) const;
    void checkSceneGraphCycles( const Scene& scene,
                                const ISceneActionExecute& sceneActionExecutor,
                                const EventContainer& events,
                                bool isAddingNewScene ) const;
    SceneGraphCyclesDetector::RunSceneActions collectRunSceneThenAction(
        const ISceneActionExecute& newSceneExecutor ) const;
    static std::string dumpSceneGraphCyclePath( const SceneGraphCyclesDetector::ScenesConnections& connections,
                                                const Scene::SceneID& newSceneId,
                                                bool isAddingNewScene );

    void checkExpressionItems( const EventContainer& events ) const;
    void checkActionExceptions( const Scene::SceneID& sceneId,
                                const ActionBlocks& actionBlocks,
                                const EventContainer& sceneWhenBlocks,
                                const EventContainer& sceneExceptionBlocks ) const;
    void checkBlockReferenceException( const ActionExceptionData::BlockReference& blockReference,
                                       const Scene::SceneID& sceneId,
                                       const EventContainer& sceneWhenBlocks,
                                       const EventContainer& sceneExceptionBlocks ) const;
    void checkExpressionReferenceException( const ActionExceptionData::ExpressionReference& expressionReference ) const;

    void cacheExpressionItem( const ValueSourceKey& searchKey, const EventBase& event );

    void subscribeToTriggerEvents( const std::string& sceneId, Operators& operators, EventMap& events );
    void subscribeToExceptionEvents( const std::string& sceneId, Operators& operators, EventMap& events );
    void subscribeToOperatorEvents( const std::string& sceneId,
                                    Operators& operators,
                                    EventMap& events,
                                    SceneStateEventHandler sceneHandler,
                                    OperatorStateEventHandler operatorHandler );

    void onOperatorSceneStateEvent( const Scene::SceneID& sceneId, AbstractOperator::OperatorState state );
    void onTriggerOperatorStateEvent( const Scene::SceneID& sceneId,
                                      const BlockId& blockId,
                                      AbstractOperator::OperatorState oldState,
                                      AbstractOperator::OperatorState newState );
    void onExceptionOperatorStateEvent( const Scene::SceneID& sceneId,
                                        const BlockId& blockId,
                                        AbstractOperator::OperatorState oldState,
                                        AbstractOperator::OperatorState newState );

    void updateEventState( EventBase& event, bool state, bool updateParent );
    void updateBlockStateIfDifferent( const Scene::SceneID& sceneId,
                                      const BlockId& blockId,
                                      AbstractOperator::OperatorState state );

    bool findArmedStateForItem( const ValueSourceKey& key, ItemEvent::ArmedState& armedState ) const;
    ItemArmedEventDataList readItemsDataFromJson( const ControllerId& controllerId,
                                                  const std::vector< Json >& jsonItems ) const;

    static bool isWhenBlockExists( const SceneData& scene, const std::string& whenBlockId );
    HubPlatform::CoreD::Model::FeaturePtr getFeature( const std::string& name );

private:  // variables
    Scenes m_scenes;
    HubPlatform::Services::Service& m_hostService;
    ScheduleHandler m_schedulerHandler;
    HubPlatform::CoreDRemoteInterface::CoreDInterface& m_coreInterface;
    SceneActionExecuteFactory m_actionFactory;

    ActionRunRequestsContainer m_actionsRunRequests;

    std::map< ItemEvent::ItemID, ItemToDeviceInfo > m_itemToDevice;
    std::map< ItemEvent::DeviceID, std::set< ItemEvent::ItemID > > m_deviceToItem;

    HouseModeEvents m_houseModeChangedToEvents;
    HouseModeEvents m_houseModeChangedFromEvents;
    HouseModeAlarmPhaseRangeEvents m_houseModeAlarmPhaseRangeEvents;
    HouseModeSwitchToRangeEvents m_houseModeSwitchToRangeEvents;
    DictionaryValueEvents m_dictionaryValueEvents;
    DictionaryChangedEvents m_dictionaryChangedEvents;
    HotzoneEvents m_hotzoneEvents;
    ItemArmedEvents m_itemArmedEvents;
    ItemEvents m_valueDrivenEvents;
    DeviceItemGroupEvents m_valueDeviceItemGroupEvents;
    ItemEvents m_isItemStateChangedEvents;
    NetworkMainStateEvents m_networkMainStateEvents;
    BatteryStateEvents m_batteryStateEvents;
    BatteryLevelEvents m_batteryLevelEvents;
    NetworkFailedStateEvents m_networkFailedStateEvents;
    SceneStateEvents m_sceneStateEvents;
    GroupStateEvents m_groupStateEvents;
    CloudStateEvents m_cloudStateEvents;
    FirmwareUpdateStateEvents m_firmwareUpdateStateEvents;
    bool m_isCloudConnected = false;
    DeviceStateEvents m_devicesStateEvents;
    DeviceStateEvents m_devicesStateGroupEvents;
    FeatureStateEvents m_featuresStateEvents;

    ControllerFactoryInterface& m_controllerFactoryInterface;
    Expressions::ExpressionManager& m_expressions;
    ItemCacheBase& m_itemCache;
    HubPlatform::CoreD::DeviceItemGroupManager& m_deviceItemGroupManager;
    StatesRegistry& m_statesRegistry;
    std::reference_wrapper< IClock > m_clock;

    ActionBlocks::Actions m_runCustomScriptActions;

    std::string m_mainNetworkInterface;
    SceneContextManagerPtr m_sceneActionContextManager;
};
}  // namespace SceneEngine
