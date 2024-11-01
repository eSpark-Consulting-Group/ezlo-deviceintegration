#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_engine/environment.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/scene_graph_cycles_detector.h"
#include "scenes_manager/scenes_engine/scene_status_builder.h"
#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache_base.h"
#include "scenes_manager/scenes_engine/operators/event_operator.h"
#include "scenes_manager/scenes_engine/operators/multiple_children_operator.h"
#include "scenes_manager/scenes_engine/operators/visitors/operator_name_visitor.h"
#include "scenes_manager/scenes_engine/operators/visitors/latch_visitor.h"
#include "scenes_manager/scenes_engine/operators/operator_exceptions.h"
#include "scenes_manager/scenes_engine/events/transactional_events_updater.h"
#include "scenes_manager/scenes_engine/actions/action_exception.h"
#include "scenes_manager/scenes_engine/expressions/expression_value_provider.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "scenes_manager/scenes_engine/event_container.h"
#include "cored/group_operations/device_item_group_manager.h"
#include "cored/model/resources/cored_resources.h"
#include "firmware/core/scenes_manager/scenes_engine/actions/scene_context_info.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/remote_interface/cored_remote_interface.h"

#include "database_client/dbid.h"

#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/exception_utils.h"
#include "base_utils/timeutils.h"

#include <tiny/tiny_dispatch.h>
#include <functional>
#include <utility>

namespace SceneEngine {

namespace {

std::string whatEvent( const EventBase& event ) {
    return event.getMethodName() + " : event id = " + event.getEventId();
}

std::string eventProcessError( const EventBase& event, const std::exception& e ) {
    return "Unable to process: " + whatEvent( event ) + " : " + HubPlatform::CommonUtils::whatError( e );
}

template < typename EventPtr, typename... Values >
void requestUpdateGeneric( TransactionalEventsUpdater& updater, const EventPtr& event, const Values&... values ) {
    const bool state = event->compare( values... );
    if( state )
        updater.requestTrigger( event );
    else
        updater.requestReset( event );
}

template < typename EventPtr, typename... Values >
void processEventGenericCompare( SceneManager& sceneManager,
                                 const std::string& sceneContextId,
                                 const std::vector< EventPtr >& events,
                                 const Values&... values ) {
    TransactionalEventsUpdater updater = sceneManager.makeEventsUpdater( sceneContextId );

    for( auto& event : events ) {
        try {
            requestUpdateGeneric( updater, event, values... );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( *event, e ) ) );
        }
    }

    updater.flush();
}

template < typename Key, typename EventPtr, typename... Values >
void processEventGenericCompare( SceneManager& sceneManager,
                                 const std::string& contextId,
                                 const std::multimap< Key, EventPtr >& map,
                                 const Key& key,
                                 const Values&... values ) {
    TransactionalEventsUpdater updater = sceneManager.makeEventsUpdater( contextId );

    auto range = map.equal_range( key );
    for( auto it = range.first; it != range.second; ++it ) {
        const EventPtr& event = it->second;
        try {
            requestUpdateGeneric( updater, event, values... );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( *event, e ) ) );
        }
    }

    updater.flush();
}

bool isEventUseValue( const EventBase& event,
                      ValueSourceKey::SourceType type,
                      const ValueSourceKey::SourceId& sourceId ) {
    for( const ValueSourceKey& eventValueKey : event.getValueKeys() ) {
        if( eventValueKey.getType() == type && eventValueKey.getSourceId() == sourceId )
            return true;
    }
    return false;
}

template < class EventPtr, class UpdateEventCallback >
bool handleExpressionEvent( SceneManager& sceneManager,
                            const ValueSourceKey& eventKey,
                            const EventPtr& eventPtr,
                            const std::string& expressionName,
                            const Json& expressionValue,
                            UpdateEventCallback&& callback ) {
    if( !eventPtr )
        return false;

    try {
        if( eventKey.isExpression() && eventKey.getSourceId() == expressionName ) {
            callback( eventPtr, expressionValue );
            return true;
        }
        else if( isEventUseValue( *eventPtr, ValueSourceKey::SourceType::Expression, expressionName ) ) {
            callback( eventPtr, sceneManager.fetchSourceValue( eventKey ) );
            return true;
        }
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to handleExpressionEvent: " + HubPlatform::CommonUtils::whatError( e ) ) );
    }
    return false;
}

using UpdateItemEventCallback = std::function< void( const ItemEventBasePtr&, const ItemEventBase::ItemValue& ) >;
bool handleItemEvent( SceneManager& sceneManager,
                      const ValueSourceKey& eventKey,
                      const ItemEventBasePtr& eventPtr,
                      const ValueSourceKey& itemKey,
                      const ItemEventBase::ItemValue& itemValue,
                      const UpdateItemEventCallback& callback ) {
    if( !eventPtr )
        return false;

    try {
        if( eventKey == itemKey ) {
            callback( eventPtr, itemValue );
            return true;
        }
        else if( isEventUseValue( *eventPtr, ValueSourceKey::SourceType::Item, itemKey.getSourceId() ) ) {
            callback( eventPtr, sceneManager.fetchSourceValue( eventKey ) );
            return true;
        }
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to handleItemEvent: " + HubPlatform::CommonUtils::whatError( e ) ) );
    }
    return false;
}

}  // namespace

bool SceneManager::processItemArmedEvent( TransactionalEventsUpdater& updater,
                                          const ValueSourceKey& itemSearchKey,
                                          const ItemEvent::ItemValue& itemValue ) {
    ItemEvent::ArmedState armedState;
    if( !findArmedStateForItem( itemSearchKey, armedState ) ) {
        LOGGER_DEBUG( cc::debug( "cannot read armed state for item: " + itemSearchKey.getSourceId() ) );
        return false;
    }

    auto range = m_itemArmedEvents.equal_range( itemSearchKey );
    for( auto it = range.first; it != range.second; ++it ) {
        const ItemEventPtr& event = it->second;
        try {
            requestUpdateGeneric( updater, event, itemValue, armedState, ItemEvent::EventType::ITEM );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( *event, e ) ) );
        }
    }

    return true;
}

bool SceneManager::processDeviceArmed( const ValueSourceKey::ControllerId& controllerId,
                                       const std::string& deviceId,
                                       bool armed,
                                       const JsonObject& message ) {
    LOGGER_TRACE( cc::trace( "device state: (" + deviceId + ", " + std::to_string( armed ) + ")" ) );
    auto deviceIt = m_deviceToItem.find( deviceId );
    if( deviceIt == m_deviceToItem.end() ) {
        return false;
    }

    const auto& itemIds = deviceIt->second;
    auto controllerInterface = m_controllerFactoryInterface.getControllerInterface( controllerId );
    const auto items = controllerInterface->getItems( ControllerInterface::ItemIds( itemIds.begin(), itemIds.end() ) );
    if( items.empty() ) {
        LOGGER_DEBUG( cc::debug( "empty items from core" ) );
        return false;
    }

    ItemArmedEventDataList itemArmedEventDataList = readItemsDataFromJson( controllerId, items );
    if( itemArmedEventDataList.empty() ) {
        return false;
    }

    LOGGER_ERROR( "TransactionalEventsUpdater updater = makeEventsUpdater( message, true )" );

    auto sceneContextId = createSceneContext( message );

    TransactionalEventsUpdater updater = makeEventsUpdater( sceneContextId );
    for( const auto& data : itemArmedEventDataList ) {
        const ItemEventPtr& event = data.eventPtr;
        LOGGER_TRACE( cc::trace( event->getId() + " " + event->getValue().dump() ) );
        try {
            auto eventType = ItemEvent::EventType::DEVICE;
            requestUpdateGeneric( updater, event, data.value, ItemEvent::toArmedState( armed ), eventType );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( *event, e ) ) );
        }
    }
    updater.flush();

    return true;
}

void SceneManager::processItemEvent( const ValueSourceKey& itemSearchKey,
                                     const ItemEventBase::ItemValue& itemValue,
                                     const JsonObject& message ) {
    LOGGER_TRACE_RAW( cc::trace( "item state: (" + itemSearchKey.getControllerId() + ", " +
                                 itemSearchKey.getSourceId() + "), value:" ) +
                      cc::j( itemValue.getPayload() ) );
    auto sceneContextId = createSceneContext( message );
    TransactionalEventsUpdater updater = makeEventsUpdater( sceneContextId );
    TransactionalEventsUpdater::Events itemStateChangedEventsToUpdate;

    processItemArmedEvent( updater, itemSearchKey, itemValue );

    {
        UpdateItemEventCallback callback(
            [&updater]( const ItemEventBasePtr& event, const ItemEventBase::ItemValue& comparable ) {
                requestUpdateGeneric( updater, event, comparable );
            } );

        for( auto& eventData : m_valueDrivenEvents )
            handleItemEvent( *this, eventData.first, eventData.second, itemSearchKey, itemValue, callback );
    }
    {
        UpdateItemEventCallback callback(
            [&itemStateChangedEventsToUpdate]( const ItemEventBasePtr& event,
                                               const ItemEventBase::ItemValue& comparable ) {
                const bool state = event->compare( comparable );
                if( state )
                    itemStateChangedEventsToUpdate.push_back( event );
            } );

        for( auto& eventData : m_isItemStateChangedEvents )
            handleItemEvent( *this, eventData.first, eventData.second, itemSearchKey, itemValue, callback );
    }

    updater.requestTrigger( itemStateChangedEventsToUpdate );
    updater.flush();
    updater.requestReset( itemStateChangedEventsToUpdate );
    updater.flush();
}

void SceneManager::processExtendedItemEvent( const ValueSourceKey& itemSearchKey,
                                             const Json& oldItemValue,
                                             const Json& newItemValue ) {
    LOGGER_TRACE_RAW(
        cc::trace( "item state extended: key: (" + itemSearchKey.getControllerId() + ", " +
                   itemSearchKey.getSourceId() ) +
        cc::trace( "), old value: (" + oldItemValue.dump() + "), new value: (" + newItemValue.dump() + ")" ) );
}

void SceneManager::processItemDictionaryEvent( const ValueSourceKey& itemSearchKey,
                                               const DictionaryValueEvent::Operation& operation,
                                               const DictionaryValueEvent::ElementValue& element,
                                               const JsonObject& message ) {
    LOGGER_TRACE_RAW( cc::trace( "item state: (" + itemSearchKey.getControllerId() + ", " +
                                 itemSearchKey.getSourceId() + ", " + operation + ", " + element.dump() + ")" ) );
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_dictionaryValueEvents, itemSearchKey, operation, element );
}

void SceneManager::processIsDictionaryChangedEvent( const ValueSourceKey& itemSearchKey,
                                                    const DictionaryChangedEvent::Operation& operation,
                                                    const DictionaryChangedEvent::KeyValue& keyValue,
                                                    const JsonObject& message ) {
    TransactionalEventsUpdater::Events triggeredEvents;
    auto range = m_dictionaryChangedEvents.equal_range( itemSearchKey );
    for( auto it = range.first; it != range.second; ++it ) {
        const DictionaryChangedEventPtr& event = it->second;
        try {
            if( event->compare( keyValue, operation ) )
                triggeredEvents.push_back( event );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( *event, e ) ) );
        }
    }
    LOGGER_ERROR( "SceneManager::processIsDictionaryChangedEvent" );
    auto sceneContextId = createSceneContext( message );
    TransactionalEventsUpdater updater = makeEventsUpdater( sceneContextId );
    updater.requestTrigger( triggeredEvents );
    updater.flush();
    updater.requestResetSilent( triggeredEvents );
    updater.flush();
}

void SceneManager::processIsItemStateGroupEvent( const Json& itemUpdatedMessage,
                                                 const ItemEventBase::ItemValue& itemValue,
                                                 ItemEvent::ArmedState armed ) {
    bool isDeviceGroupsFetched = false;
    std::set< std::string > matchedDeviceGroups;

    bool isItemGroupsFetched = false;
    std::set< std::string > matchedItemGroups;
    for( auto itemEventIt = m_valueDeviceItemGroupEvents.cbegin(); itemEventIt != m_valueDeviceItemGroupEvents.cend();
         ++itemEventIt ) {
        const ValueSourceKey& eventKey = itemEventIt->first;
        if( !eventKey.isDeviceItemGroups() ) {
            continue;
        }

        ItemEvent& event = *itemEventIt->second;
        auto deviceItemGroupIds = ValueSourceKey::unpackDeviceItemGroupIds( eventKey.getSourceId() );
        auto& deviceGroupId = deviceItemGroupIds.first;
        auto& itemGroupId = deviceItemGroupIds.second;

        if( deviceGroupId.empty() && itemGroupId.empty() ) {
            LOGGER_ERROR_RAW( cc::error( "Invalid event key for device and item group" ) );
            continue;
        }

        if( !deviceGroupId.empty() ) {
            if( !isDeviceGroupsFetched ) {
                matchedDeviceGroups =
                    m_deviceItemGroupManager.matchDeviceGroupsForItemUpdatedEvent( itemUpdatedMessage );
                isDeviceGroupsFetched = true;
            }
        }

        if( !itemGroupId.empty() ) {
            if( !isItemGroupsFetched ) {
                matchedItemGroups = m_deviceItemGroupManager.matchItemGroupsForItemUpdatedEvent( itemUpdatedMessage );
                isItemGroupsFetched = true;
            }
            if( !matchedItemGroups.count( itemGroupId ) )
                continue;
        }

        try {
            using namespace HubPlatform::CoreD::Model::Resources;
            const auto itemId = json_get_str( itemUpdatedMessage, ItemFields::IdFieldName );
            const bool isGroupOperation = true;
            bool state = event.compare( itemId, itemValue, armed, isGroupOperation );
            const bool updateParent = true;
            updateEventState( event, state, updateParent );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( event, e ) ) );
        }
    }
}

void SceneManager::processDeviceItemGroupEvent( const Json& itemUpdatedMessage,
                                                const ItemEventBase::ItemValue& itemValue ) {
    bool isDeviceGroupsFetched = false;
    std::set< std::string > matchedDeviceGroups;

    bool isItemGroupsFetched = false;
    std::set< std::string > matchedItemGroups;

    for( auto itemEventIt = m_valueDrivenEvents.cbegin(); itemEventIt != m_valueDrivenEvents.cend(); ++itemEventIt ) {
        const ValueSourceKey& eventKey = itemEventIt->first;

        ItemEventBase& event = *itemEventIt->second;
        if( !eventKey.isDeviceItemGroups() ||
            event.getMethodName() == Model::Resources::MethodNames::isDeviceItemGroup )
            continue;
        auto deviceItemGroupIds = ValueSourceKey::unpackDeviceItemGroupIds( eventKey.getSourceId() );
        auto& deviceGroupId = deviceItemGroupIds.first;
        auto& itemGroupId = deviceItemGroupIds.second;

        if( deviceGroupId.empty() && itemGroupId.empty() ) {
            LOGGER_ERROR_RAW( cc::error( "Invalid event key for device and item group" ) );
            continue;
        }

        if( !deviceGroupId.empty() ) {
            if( !isDeviceGroupsFetched ) {
                matchedDeviceGroups =
                    m_deviceItemGroupManager.matchDeviceGroupsForItemUpdatedEvent( itemUpdatedMessage );
                isDeviceGroupsFetched = true;
            }
        }

        if( !itemGroupId.empty() ) {
            if( !isItemGroupsFetched ) {
                matchedItemGroups = m_deviceItemGroupManager.matchItemGroupsForItemUpdatedEvent( itemUpdatedMessage );
                isItemGroupsFetched = true;
            }
            if( !matchedItemGroups.count( itemGroupId ) )
                continue;
        }

        try {
            bool state = event.compare( itemValue );
            const bool updateParent = true;
            updateEventState( event, state, updateParent );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( event, e ) ) );
        }
    }
}

void SceneManager::processIsDetectedInHotzoneEvent( const ValueSourceKey& itemSearchKey,
                                                    const HotzoneEvent::HotzonesItemValue& itemValue,
                                                    const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_hotzoneEvents, itemSearchKey, itemValue );
}

void SceneManager::processDeviceStateEvent( const ValueSourceKey& itemSearchKey,
                                            const JsonObject& message,
                                            const JsonObject& broadcastMessage ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_devicesStateEvents, itemSearchKey, message );
}

void SceneManager::processDeviceGroupStateEvent( const ValueSourceKey& itemSearchKey,
                                                 const JsonObject& deviceInfo,
                                                 const std::string& deviceId,
                                                 const std::string& category,
                                                 const std::string& subcategory,
                                                 const JsonObject& message ) {
    LOGGER_ERROR( "SceneManager::processDeviceGroupStateEvent" );
    auto sceneContextId = createSceneContext( message );
    TransactionalEventsUpdater updater = makeEventsUpdater( sceneContextId );
    bool isDeviceGroupsFetched = false;
    std::set< std::string > matchedDeviceGroups;

    bool isItemGroupsFetched = false;
    std::set< std::string > matchedItemGroups;

    for( auto eventIt = m_devicesStateEvents.cbegin(); eventIt != m_devicesStateEvents.cend(); ++eventIt ) {
        const ValueSourceKey& eventKey = eventIt->first;
        if( !eventKey.isDeviceItemGroups() ) {
            continue;
        }

        auto event = eventIt->second;
        auto deviceItemGroupIds = ValueSourceKey::unpackDeviceItemGroupIds( eventKey.getSourceId() );
        auto& deviceGroupId = deviceItemGroupIds.first;
        auto& itemGroupId = deviceItemGroupIds.second;

        if( deviceGroupId.empty() && itemGroupId.empty() ) {
            LOGGER_ERROR_RAW( cc::error( "Invalid event key for device and item group" ) );
            continue;
        }

        if( !deviceGroupId.empty() ) {
            if( !isDeviceGroupsFetched ) {
                matchedDeviceGroups =
                    m_deviceItemGroupManager.matchDeviceGroupsForDeviceUpdatedEvent( deviceId, category, subcategory );
                isDeviceGroupsFetched = true;
            }
            if( !matchedDeviceGroups.count( deviceGroupId ) ) {
                continue;
            }
        }

        if( event->compare( deviceInfo ) ) {
            updater.requestTrigger( event );
        }
        else {
            updater.requestReset( event );
        }
    }

    updater.flush();
}

void SceneManager::processHouseModeChangedToEvent( const ControllerId& controllerId,
                                                   const std::string& mode,
                                                   const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_houseModeChangedToEvents, controllerId, mode );
}

void SceneManager::processHouseModeChangedFromEvent( const ControllerId& controllerId,
                                                     const std::string& mode,
                                                     const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_houseModeChangedFromEvents, controllerId, mode );
}

void SceneManager::processHouseAlarmRangeEvent( const ControllerId& controllerId,
                                                const std::string& phase,
                                                const std::string& status,
                                                const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_houseModeAlarmPhaseRangeEvents, controllerId, phase, status );
}

void SceneManager::processHouseSwitchToRangeEvent( const ControllerId& controllerId,
                                                   const std::string& phase,
                                                   const std::string& status,
                                                   const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_houseModeSwitchToRangeEvents, controllerId, status );
}

void SceneManager::processBatteryStateEvent( const std::string& state, const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_batteryStateEvents, state );
}

void SceneManager::processBatteryLevelEvent( int level, const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_batteryLevelEvents, level );
}

void SceneManager::processFirmwareUpdateStateEvent( const std::string& status, const JsonObject& message ) {
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_firmwareUpdateStateEvents, status );
}

void SceneManager::processMainNetworkInterfaceChanged( const std::string& interfaceId, const JsonObject& message ) {
    LOGGER_TRACE( cc::trace( "new main interface: " ) + cc::note( interfaceId ) );
    m_mainNetworkInterface = interfaceId;
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_networkMainStateEvents, interfaceId );
}

void SceneManager::processNetworkInterfaceStateChanged( const std::string& interfaceId,
                                                        bool isConnected,
                                                        const JsonObject& message ) {
    LOGGER_TRACE( cc::trace( "interface " ) + cc::note( interfaceId ) + cc::trace( " state: " ) +
                  cc::flag( isConnected ) );
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_networkFailedStateEvents, interfaceId, isConnected );
}

void SceneManager::processCloudConnectionState( bool isConnected, const JsonObject& message ) {
    LOGGER_TRACE( cc::trace( "cloud connection state: " + cc::flag( isConnected ) ) );
    if( m_isCloudConnected != isConnected ) {
        m_isCloudConnected = isConnected;
        auto sceneContextId = createSceneContext( message );
        processEventGenericCompare( *this, sceneContextId, m_cloudStateEvents, isConnected );
    }
}

template < typename T >
void SceneManager::processSceneStateEvents( const Scene::SceneID& sceneId, T&& state ) {
    LOGGER_ERROR( "SceneManager::processSceneStateEvents" );
    auto sceneContextId = createSceneContext( JSON_EMPTY_OBJ );
    TransactionalEventsUpdater updater = makeEventsUpdater( sceneContextId );
    for( auto& event : m_sceneStateEvents ) {
        const SceneStateEvent::CompareResult compareResult = event->compare( sceneId, std::forward< T >( state ) );
        if( compareResult == SceneStateEvent::CompareResult::True )
            updater.requestTrigger( event );
        else if( compareResult == SceneStateEvent::CompareResult::False )
            updater.requestReset( event );
    }
    updater.flush();
}

void SceneManager::processGroupStateEvent( const Scene::SceneID& sceneId,
                                           const BlockId& blockId,
                                           AbstractOperator::OperatorState oldState,
                                           AbstractOperator::OperatorState newState ) {
    TransactionalEventsUpdater updater( nullptr );
    for( auto& event : m_groupStateEvents ) {
        if( event->isReferencedBlock( sceneId, blockId ) ) {
            requestUpdateGeneric( updater, event, oldState, newState );
        }
    }
    updater.flush();
}

void SceneManager::onOperatorSceneStateEvent( const Scene::SceneID& sceneId, AbstractOperator::OperatorState state ) {
    auto sceneIter = m_scenes.find( sceneId );
    if( sceneIter != m_scenes.end() ) {
        m_statesRegistry.storeSceneState( sceneId, state );
        m_actionsRunRequests.push( sceneIter->second.actionExecutorPtr, state );
    }
}

void SceneManager::onTriggerOperatorStateEvent( const Scene::SceneID& sceneId,
                                                const BlockId& blockId,
                                                AbstractOperator::OperatorState oldState,
                                                AbstractOperator::OperatorState newState ) {
    auto sceneIter = m_scenes.find( sceneId );
    if( sceneIter != m_scenes.end() ) {
        updateBlockStateIfDifferent( sceneId, blockId, newState );
        m_actionsRunRequests.push( sceneIter->second.actionExecutorPtr, blockId, newState );
    }

    processGroupStateEvent( sceneId, blockId, oldState, newState );
}

void SceneManager::onExceptionOperatorStateEvent( const Scene::SceneID& sceneId,
                                                  const BlockId& blockId,
                                                  AbstractOperator::OperatorState oldState,
                                                  AbstractOperator::OperatorState newState ) {
    updateBlockStateIfDifferent( sceneId, blockId, newState );
}

void SceneManager::resetSceneLatches( const Scene::SceneID& sceneId ) {
    auto sceneIt = m_scenes.find( sceneId );
    if( sceneIt == m_scenes.end() )
        throw SceneEngine::SceneException( Errors::SCENE_DOES_NOT_EXIST, "Scene does not exist: " + sceneId );

    SceneData& sceneData = sceneIt->second;

    const bool resetLatch = true;
    LatchVisitor visitor( resetLatch );
    sceneData.forEachOperator( [&visitor]( const AbstractOperator::SharedOperator& op ) { op->accept( visitor ); } );
}

void SceneManager::resetAllChildLatches( const Scene::SceneID& sceneId, const BlockId& blockId ) {
    auto sceneIter = m_scenes.find( sceneId );
    if( sceneIter == m_scenes.end() )
        return;

    SceneData& sceneData = sceneIter->second;
    AbstractOperator::SharedOperator parentOp = sceneData.findOperator( blockId );
    if( !parentOp )
        return;

    // iterate all children on the block, select latches, reset closed ones
    if( auto* multipleChildren = dynamic_cast< MultipleChildrenOperator* >( parentOp.get() ) ) {
        multipleChildren->forEach( []( AbstractOperator& op ) {
            LOGGER_TRACE_RAW( cc::trace( "iterating to reset latches: blockId = " ) + op.getBlockID() );

            const bool resetLatch = true;
            LatchVisitor visitor( resetLatch );
            op.accept( visitor );
        } );
    }
}

void SceneManager::resetLatch( const Scene::SceneID& sceneId, const SceneManager::BlockId& blockId ) {
    auto sceneIter = m_scenes.find( sceneId );
    if( sceneIter == m_scenes.end() )
        throw SceneEngine::SceneException( Errors::SCENE_DOES_NOT_EXIST, "Scene does not exist: " + sceneId );

    SceneData& sceneData = sceneIter->second;
    auto foundOp = sceneData.findOperator( blockId );
    if( !foundOp )
        throw SceneEngine::SceneException( Errors::SCENE_BLOCK_DOES_NOT_EXIST, "blockId: " + blockId );

    const bool resetLatch = true;
    LatchVisitor visitor( resetLatch );
    foundOp->accept( visitor );

    if( visitor.isLatch() ) {
        if( visitor.isCompleted() ) {
            LOGGER_INFO( cc::info( "Latch have been opened, sceneId: " ) + sceneId + cc::info( ", blockId: " ) +
                         blockId );
        }
        else {
            LOGGER_DEBUG( cc::debug( "Latch wasn't reset, sceneId: " + sceneId + ", blockId: " + blockId ) );
        }
    }
    else {
        LOGGER_TRACE( cc::trace( "Not a latch block, blockId: " ) + blockId );
        throw LatchException( "Block is not a latch" );
    }
}

SceneSharedPtr SceneManager::findSceneById( const Scene::SceneID& id ) const {
    auto it = m_scenes.find( id );
    return it != m_scenes.end() ? it->second.scenePtr : nullptr;
}

bool SceneManager::setSceneGroupEnabled( const std::string& group_id, bool enabled ) {
    bool ret = false;
    for( auto& sceneIt : m_scenes ) {
        SceneSharedPtr& scenePtr = sceneIt.second.scenePtr;
        if( scenePtr->getGroupId() == group_id ) {
            processSceneStateEvents( scenePtr->getId(), enabled );
            scenePtr->setEnabled( enabled );
            ret = true;
        }
    }
    return ret;
}

void SceneManager::setSceneBlockEnabled( const Scene::SceneID& id, const BlockId& blockId, bool enabled ) {
    auto sceneIt = m_scenes.find( id );
    if( sceneIt == m_scenes.end() ) {
        throw SceneEngine::SceneException( Errors::SCENE_DOES_NOT_EXIST );
    }

    SceneData& sceneData = sceneIt->second;
    if( auto event = sceneData.findEvent( blockId ) ) {
        event->setEnabled( enabled );
    }
    else if( auto op = sceneData.findOperator( blockId ) ) {
        op->setEnabled( enabled );
    }
    else {
        throw SceneEngine::SceneException( Errors::SCENE_BLOCK_DOES_NOT_EXIST );
    }
}

SceneManager::SceneManager( HubPlatform::Services::Service& hostService,
                            HubPlatform::CoreDRemoteInterface::CoreDInterface& coreInterface,
                            ControllerFactoryInterface& controllerFactoryInterface,
                            BroadcastCallback sendSceneProgressBroadcast,
                            Expressions::ExpressionManager& expressions,
                            ItemCacheBase& itemCache,
                            HubPlatform::CoreD::DeviceItemGroupManager& deviceItemGroupManager,
                            StatesRegistry& statesRegistry,
                            IClock& clock )
: m_hostService( hostService )
, m_schedulerHandler( *this, clock )
, m_coreInterface( coreInterface )
, m_actionFactory( hostService,
                   coreInterface,
                   controllerFactoryInterface,
                   [this, sendSceneProgressBroadcast]( const JsonObject& message ) {
                       sendSceneProgressBroadcast( message );
                       handleSceneProgressBroadcast( message );
                   },
                   statesRegistry )
, m_controllerFactoryInterface( controllerFactoryInterface )
, m_expressions( expressions )
, m_itemCache( itemCache )
, m_deviceItemGroupManager( deviceItemGroupManager )
, m_statesRegistry( statesRegistry )
, m_clock( clock ) {
    m_sceneActionContextManager = std::make_shared< SceneContextManager >( m_hostService.getDBClient() );
    m_schedulerHandler.start();
}

SceneManager::~SceneManager() {
    m_sceneActionContextManager.reset();
}

void SceneManager::tearDown() {
    LOGGER_TRACE_RAW( cc::trace( "SceneManager::tearDown: drop scheduler" ) );
    m_schedulerHandler.tearDown();

    LOGGER_TRACE( "drop scenes and events" );
    for( const auto& sceneData : m_scenes )
        cleanSceneDependencies( sceneData.first );

    m_scenes.clear();
    LOGGER_TRACE_RAW( cc::trace( "SceneManager::tearDown: finished" ) );
}

void SceneManager::checkRunSceneActionGroupID( const ActionBlocks& actions ) {
    actions.forEach( [&]( const ActionSharedPtr& actionPtr ) {
        using namespace Model::Resources;
        if( actionPtr->getName() == ActionNames::RunSceneAction ) {
            auto& args = actionPtr->getArgs();
            auto block = json_get_str( args, "block" );
            if( block == "thenGroups" || block == "elseGroups" ) {
                auto linkedSceneIter = m_scenes.find( json_get_str( args, "sceneId" ) );
                if( linkedSceneIter == m_scenes.end() ) {
                    throw SceneException( Errors::SCENE_DOES_NOT_EXIST );
                }
                auto linkedGroupId = json_get_str( args, "group" );
                if( !isWhenBlockExists( linkedSceneIter->second, linkedGroupId ) ) {
                    throw SceneException( Errors::SCENE_ACTION_GROUP_NOT_EXIST );
                }
            }
        }
    } );
}

void SceneManager::checkSceneIdActionArg( const Scene::SceneID& recursiveSceneId,
                                          const ActionBlocks& actions,
                                          const std::string& actionNameToCheck ) {
    actions.forEach( [&]( const ActionSharedPtr& actionPtr ) {
        if( actionPtr->getName() == actionNameToCheck ) {
            auto args = actionPtr->getArgs();
            auto linkedSceneId = args[ "sceneId" ];
            if( linkedSceneId != recursiveSceneId ) {
                auto linkedSceneIter = m_scenes.find( linkedSceneId );
                if( linkedSceneIter == m_scenes.end() ) {
                    throw SceneException( Errors::SCENE_DOES_NOT_EXIST );
                }
                args[ "global" ] = linkedSceneIter->second.scenePtr->isGlobal();
                actionPtr->setArgs( std::move( args ) );
            }
            else {
                if( actionNameToCheck == Model::Resources::ActionNames::SetSceneStateAction ) {
                    args[ "global" ] = false;
                    actionPtr->setArgs( std::move( args ) );
                }

                else {
                    throw SceneException( Errors::SCENE_ACTION_RECURSIVE_EXECUTE );
                }
            }
        }
    } );
}

void SceneManager::checkSceneStateEvents( const SceneSharedPtr& scenePtr, const EventContainer& events ) {
    const Scene::SceneID& eventSceneId = scenePtr->getId();

    for( const auto& event : events.sceneStateEvents ) {
        const Scene::SceneID& activatingSceneId = event->getActivatingSceneId();
        if( activatingSceneId == eventSceneId ) {
            throw SceneException( Errors::SCENE_WHEN_RECURSIVE_EXECUTE );
        }
        if( m_scenes.find( activatingSceneId ) == m_scenes.end() ) {
            throw SceneException( Errors::SCENE_DOES_NOT_EXIST );
        }
    }
}

void SceneManager::checkGroupStateEvents( const SceneSharedPtr& scenePtr,
                                          const EventContainer::GroupStateEvents& events ) const {
    for( auto& event : events ) {
        if( event->getSceneIdArg() == scenePtr->getId() ) {
            throw SceneException( Errors::SCENE_WHEN_RECURSIVE_EXECUTE );
        }
        auto linkedSceneIter = m_scenes.find( event->getSceneIdArg() );
        if( linkedSceneIter == m_scenes.end() ) {
            throw SceneException( Errors::SCENE_DOES_NOT_EXIST );
        }
        if( !isWhenBlockExists( linkedSceneIter->second, event->getGroupIdArg() ) ) {
            throw SceneException( Errors::SCENE_ACTION_GROUP_NOT_EXIST );
        }
    }
}

void SceneManager::checkSceneGraphCycles( const Scene& scene,
                                          const ISceneActionExecute& sceneActionExecutor,
                                          const EventContainer& events,
                                          bool isAddingNewScene ) const {
    auto isTrigger = [this]( const EventBasePtr& event ) {
        const Scene::SceneID& sceneId = event->getSceneId();
        auto it = m_scenes.find( sceneId );
        if( it != m_scenes.end() ) {
            const SceneData& sceneData = it->second;
            return sceneData.triggerEvents.count( event->getEventId() ) > 0;
        }

        return false;
    };
    SceneGraphCyclesDetector::SceneStateEvents sceneStateTriggers;
    std::copy_if(
        m_sceneStateEvents.begin(), m_sceneStateEvents.end(), std::back_inserter( sceneStateTriggers ), isTrigger );
    SceneGraphCyclesDetector::GroupStateEvents groupStateTriggers;
    std::copy_if(
        m_groupStateEvents.begin(), m_groupStateEvents.end(), std::back_inserter( groupStateTriggers ), isTrigger );

    SceneGraphCyclesDetector::RunSceneActions runSceneActions = collectRunSceneThenAction( sceneActionExecutor );

    SceneGraphCyclesDetector cyclesDetector(
        std::move( sceneStateTriggers ), std::move( groupStateTriggers ), std::move( runSceneActions ), scene, events );
    SceneGraphCyclesDetector::RunDetectionResult detectionResult = cyclesDetector.runDetection();
    const bool cycleDetected = detectionResult.first;
    if( cycleDetected ) {
        const std::string sceneCyclePath =
            dumpSceneGraphCyclePath( detectionResult.second, scene.getId(), isAddingNewScene );
        LOGGER_ERROR_RAW( cc::error( sceneCyclePath ) );
        throw SceneException( Errors::SCENE_CYCLE_DETECTED, sceneCyclePath.c_str() );
    }
}

SceneGraphCyclesDetector::RunSceneActions SceneManager::collectRunSceneThenAction(
    const ISceneActionExecute& newSceneExecutor ) const {
    SceneGraphCyclesDetector::RunSceneActions runSceneActions;
    auto collectRunSceneActions = [&runSceneActions]( const Scene::SceneID& sceneId,
                                                      const ISceneActionExecute& actionExecutor ) {
        using namespace Model::Resources;
        const ActionBlocks::Actions actions = actionExecutor.getThenActions( ActionNames::RunSceneAction );
        for( const ActionSharedPtr& action : actions ) {
            Scene::SceneID executedSceneId = json_get_str( action->getArgs(), "sceneId" );
            const std::string blockId = json_get_str( action->getArgs(), "block" );
            if( blockId == "then" || blockId.empty() && actionExecutor.hasThenActions() ) {
                runSceneActions.emplace( sceneId, std::move( executedSceneId ) );
            }
        }
    };

    for( const auto& sceneNameDataPair : m_scenes ) {
        const SceneActionExecutePtr& actionExecutor = sceneNameDataPair.second.actionExecutorPtr;
        collectRunSceneActions( sceneNameDataPair.first, *actionExecutor );
    }
    collectRunSceneActions( newSceneExecutor.getSceneId(), newSceneExecutor );

    return runSceneActions;
}

std::string SceneManager::dumpSceneGraphCyclePath( const SceneGraphCyclesDetector::ScenesConnections& connections,
                                                   const Scene::SceneID& newSceneId,
                                                   bool isAddingNewScene ) {
    const std::string newScenePrintId = "*new scene*";
    auto getSceneIdError =
        [&newSceneId, &newScenePrintId, isAddingNewScene]( const Scene::SceneID& sceneId ) -> const std::string& {
        if( isAddingNewScene && sceneId == newSceneId )
            return newScenePrintId;

        return sceneId;
    };

    std::stringstream errorMessage;
    errorMessage << "Detected cycle with following scenes: ";
    errorMessage << getSceneIdError( newSceneId );
    for( const auto& connection : connections ) {
        errorMessage << " --" << connection.m_connectionMethod << "-> " << getSceneIdError( connection.m_sceneId );
    }

    return errorMessage.str();
}

void SceneManager::checkActionExceptions( const Scene::SceneID& sceneId,
                                          const ActionBlocks& actionBlocks,
                                          const EventContainer& sceneWhenBlocks,
                                          const EventContainer& sceneExceptionBlocks ) const {
    actionBlocks.forEach( [this, &sceneId, &sceneWhenBlocks, &sceneExceptionBlocks]( const ActionSharedPtr& action ) {
        const ActionExceptionData& exceptionData = action->getExceptionData();
        if( mpark::holds_alternative< ActionExceptionData::BlockReference >( exceptionData.storage ) ) {
            const auto& blockRef = mpark::get< ActionExceptionData::BlockReference >( exceptionData.storage );
            checkBlockReferenceException( blockRef, sceneId, sceneWhenBlocks, sceneExceptionBlocks );
        }
        else if( mpark::holds_alternative< ActionExceptionData::ExpressionReference >( exceptionData.storage ) ) {
            const auto& exceptionRef = mpark::get< ActionExceptionData::ExpressionReference >( exceptionData.storage );
            checkExpressionReferenceException( exceptionRef );
        }
    } );
}

void SceneManager::checkBlockReferenceException( const ActionExceptionData::BlockReference& blockReference,
                                                 const Scene::SceneID& sceneId,
                                                 const EventContainer& sceneWhenBlocks,
                                                 const EventContainer& sceneExceptionBlocks ) const {
    AbstractOperator::SharedOperator op;
    if( blockReference.sceneId == sceneId ) {  // reference to block in the same scene
        op = sceneWhenBlocks.findBlockOperator( blockReference.blockId );
        op = op ? op : sceneExceptionBlocks.findBlockOperator( blockReference.blockId );
    }
    else {  // reference to block in other scene
        op = findBlockOperator( blockReference.sceneId, blockReference.blockId );
    }

    if( !op ) {
        LOGGER_ERROR( cc::error( "Invalid reference to block in action exception, sceneId: " ) +
                      cc::note( blockReference.sceneId ) + cc::error( ", blockId: " ) +
                      cc::note( blockReference.blockId ) );
        throw SceneException( Errors::SCENE_BLOCK_DOES_NOT_EXIST, "Block with specified id not found" );
    }
}

void SceneManager::checkExpressionReferenceException(
    const ActionExceptionData::ExpressionReference& expressionReference ) const {
    using Status = SceneEngine::Expressions::ExpressionManager::FetchResult::Status;

    if( !m_expressions.hasExpression( expressionReference.name ) )
        throw SceneException( Errors::SCENES_EXPRESSION_DOES_NOT_EXIST,
                              "Expression doesn't exist: " + expressionReference.name );

    auto fetchResult = m_expressions.fetchValue( expressionReference.name );
    if( Status::NotFound == fetchResult.status )
        throw SceneException( Errors::SCENES_EXPRESSION_ERROR,
                              "Expression value cannot be fetched, expression not found: " + expressionReference.name );
    if( !fetchResult.isOk() )
        throw SceneException( Errors::SCENES_EXPRESSION_ERROR,
                              "Expression value cannot be fetched, expression: " + expressionReference.name );

    ItemValueWrapper actual( std::move( fetchResult.value ) );
    if( !actual.isValid() )
        throw SceneException( Errors::SCENES_EXPRESSION_ERROR, "Cannot create a valid value from expression's value" );

    ItemValueWrapper expected = ActionException::getExpectedExpressionValue();
    if( !actual.isComparableWith( expected ) )
        throw SceneException( Errors::SCENES_EXPRESSION_TYPE_MISMATCH,
                              "Expression's value is incomparable with value expected by action's exception" );
}

// duplicating functionality with requests handler
void SceneManager::checkExpressionItems( const EventContainer& events ) const {
    std::set< std::string > expressionNames;
    for( const auto& eventIt : events.getExpressionItems() ) {
        EventBase& event = *eventIt.second;
        for( const ValueSourceKey& eventValueKey : event.getValueKeys() ) {
            if( eventValueKey.isExpression() ) {
                expressionNames.insert( eventValueKey.getSourceId() );
            }
        }
    }
    for( const auto& name : expressionNames ) {
        if( !m_expressions.hasExpression( name ) ) {
            throw SceneException( Errors::DOES_NOT_EXIST,
                                  std::string{ "Expression does not exist: " }.append( name ).c_str() );
        }
    }
}

void SceneManager::subscribeToTriggerEvents( const std::string& sceneId, Operators& operators, EventMap& events ) {
    auto sceneEventsHandler = &SceneManager::onOperatorSceneStateEvent;
    auto operatorEventsHandler = &SceneManager::onTriggerOperatorStateEvent;
    subscribeToOperatorEvents( sceneId, operators, events, sceneEventsHandler, operatorEventsHandler );
}

void SceneManager::subscribeToExceptionEvents( const std::string& sceneId, Operators& operators, EventMap& events ) {
    auto operatorEventsHandler = &SceneManager::onExceptionOperatorStateEvent;
    subscribeToOperatorEvents( sceneId, operators, events, nullptr, operatorEventsHandler );
}

void SceneManager::subscribeToOperatorEvents( const std::string& sceneId,
                                              Operators& operators,
                                              EventMap& events,
                                              SceneStateEventHandler sceneHandler,
                                              OperatorStateEventHandler operatorHandler ) {
    std::vector< AbstractOperator::SharedOperator > latchParents;
    auto subscribe = [this, &sceneId, &latchParents, sceneHandler, operatorHandler]( AbstractOperator& op ) {
        if( op.getParent() && OperatorNameVisitor::isLatchInAndOperator( op ) ) {
            latchParents.push_back( op.getParent() );
        }

        if( sceneHandler ) {
            op.setSceneStateEventHandler( std::bind( sceneHandler, this, sceneId, std::placeholders::_1 ) );
        }

        if( operatorHandler ) {
            auto oldState = AbstractOperator::OperatorState::False;
            auto blockId = op.getBlockID();
            op.setOperatorStateEventHandler( [this, sceneId, blockId, oldState, operatorHandler](
                                                 AbstractOperator::OperatorState newState ) mutable {
                ( this->*operatorHandler )( sceneId, blockId, oldState, newState );
                oldState = newState;
            } );
        }
    };

    // Logical operators (and, or, not, xor, functions)
    for( auto& op : operators ) {
        subscribe( *op.second );
    }
    // Scene events (isItemState, isBatteryLevel, etc.)
    for( auto& event : events ) {
        subscribe( *event.second->getOperator() );
    }

    auto subscribeOperatorEventResetLatch = [this, &sceneId, &operatorHandler]( AbstractOperator& op ) {
        auto oldState = AbstractOperator::OperatorState::False;
        auto blockId = op.getBlockID();

        LOGGER_DEBUG_RAW( cc::debug( "Reset subscription on operator state events for block with latch, blockId: " ) +
                          cc::note( blockId ) );
        op.setOperatorStateEventHandler(
            [this, sceneId, blockId, oldState, operatorHandler]( AbstractOperator::OperatorState newState ) mutable {
                if( operatorHandler ) {
                    ( this->*operatorHandler )( sceneId, blockId, oldState, newState );
                }
                if( oldState != newState ) {
                    if( newState == AbstractOperator::OperatorState::False )
                        resetAllChildLatches( sceneId, blockId );
                    oldState = newState;
                }
            } );
    };
    for( auto& op : latchParents ) {
        subscribeOperatorEventResetLatch( *op );
    }
}

void SceneManager::updateEventState( EventBase& event, bool state, bool updateParent ) {
    if( updateParent ) {
        event.runActionsIfSucceededCondition( state );
    }
    else {
        event.setOperatorState( state );
        updateBlockStateIfDifferent(
            event.getSceneId(), event.getEventId(), AbstractOperator::toOperatorState( state ) );
    }
}

void SceneManager::updateBlockStateIfDifferent( const Scene::SceneID& sceneId,
                                                const BlockId& blockId,
                                                AbstractOperator::OperatorState state ) {
    const auto stateRecord = m_statesRegistry.readBlockState( sceneId, blockId );
    if( !stateRecord.isValid || stateRecord.state != state ) {
        m_statesRegistry.storeBlockState( sceneId, blockId, state );
    }
}

bool SceneManager::addScene( SceneSharedPtr scenePtr,
                             ActionBlocks&& actions,
                             EventContainer&& triggers,
                             EventContainer&& exceptions,
                             EventContainer&& globalRestriction,
                             bool isEditing ) {
    if( !scenePtr ) {
        return false;
    }

    for( const EventContainer& events :
         { std::ref( triggers ), std::ref( exceptions ), std::ref( globalRestriction ) } ) {
        checkSceneStateEvents( scenePtr, events );
        checkGroupStateEvents( scenePtr, events.groupStateEvents );
        checkExpressionItems( events );
    }

    const Scene::SceneID sceneId = scenePtr->getId();

    checkSceneIdActionArg( sceneId, actions, Model::Resources::ActionNames::RunSceneAction );
    checkSceneIdActionArg( sceneId, actions, Model::Resources::ActionNames::SetSceneStateAction );
    checkRunSceneActionGroupID( actions );
    checkActionExceptions( sceneId, actions, triggers, exceptions );

    SceneActionExecutePtr actionExecutor =
        m_actionFactory.create( std::move( actions ), scenePtr, m_statesRegistry, m_sceneActionContextManager );

    checkSceneGraphCycles( *scenePtr, *actionExecutor, triggers, !isEditing );

    subscribeToTriggerEvents( sceneId, triggers.operators, triggers.events );
    subscribeToExceptionEvents( sceneId, exceptions.operators, exceptions.events );
    subscribeToExceptionEvents( sceneId, globalRestriction.operators, globalRestriction.events );

    SceneData sceneData;
    sceneData.scenePtr = scenePtr;
    sceneData.actionExecutorPtr = actionExecutor;
    sceneData.triggerEvents = std::move( triggers.events );
    sceneData.triggerOperators = std::move( triggers.operators );
    sceneData.exceptionEvents = std::move( exceptions.events );
    sceneData.exceptionOperators = std::move( exceptions.operators );
    sceneData.globalRestrictionEvents = std::move( globalRestriction.events );
    sceneData.globalRestrictionOperators = std::move( globalRestriction.operators );
    auto insertResult = m_scenes.insert( { sceneId, std::move( sceneData ) } );
    if( !insertResult.second ) {
        return false;
    }

    SceneData& insertedData = insertResult.first->second;
    initStateRegistryRecords( sceneId, insertedData.triggerEvents, insertedData.triggerOperators );
    initStateRegistryRecords( sceneId, insertedData.exceptionEvents, insertedData.exceptionOperators );
    initStateRegistryRecords( sceneId, insertedData.globalRestrictionEvents, insertedData.globalRestrictionOperators );

    for( auto& eventData : insertedData.triggerEvents )
        eventData.second->setSceneActionExecutor( actionExecutor );
    for( auto& eventData : insertedData.exceptionEvents )
        eventData.second->setSceneActionExecutor( actionExecutor );
    for( auto& eventData : insertedData.globalRestrictionEvents )
        eventData.second->setSceneActionExecutor( actionExecutor );

    const bool initSceneSubtree = true;
    addValueDrivenEvents( triggers, !initSceneSubtree );
    addValueDrivenEvents( exceptions, initSceneSubtree );
    addValueDrivenEvents( globalRestriction, initSceneSubtree );
    addNonItemEvents( triggers, !initSceneSubtree );
    addNonItemEvents( exceptions, initSceneSubtree );
    addNonItemEvents( globalRestriction, initSceneSubtree );

    m_runCustomScriptActions.clear();

    return true;
}

void SceneManager::registerValueDrivenEvent( const ValueSourceKey& searchKey,
                                             ItemEventBasePtr itemEvent,
                                             bool initSceneSubtree ) {
    if( searchKey.isItem() ) {
        auto controllerInterface = m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() );
        auto onItemChange = [this, itemEvent, initSceneSubtree]( const ControllerInterface::Item& item ) {
            this->onItemChange( item, itemEvent, initSceneSubtree );
        };

        controllerInterface->subscribeOnItemChanges( searchKey.getSourceId(), std::move( onItemChange ) );
    }
    else if( searchKey.isExpression() ) {
        auto checkInitialEventState = [this, searchKey, itemEvent]() {
            auto& expressionName = searchKey.getSourceId();
            Expressions::ExpressionValueProvider expressionProvider( m_expressions, expressionName );
            if( expressionProvider.fetchItemValue() ) {
                this->onExpressionChange( expressionProvider.getCachedItemValue(), itemEvent );
            }
        };
        m_hostService.addTask( std::move( checkInitialEventState ) );
    }
}

void SceneManager::initStateRegistryRecords( const Scene::SceneID& sceneId,
                                             const EventMap& events,
                                             const Operators& operators ) {
    const auto initialState = AbstractOperator::OperatorState::False;

    m_statesRegistry.storeInitialSceneState( sceneId, initialState );

    for( const auto& eventData : events ) {
        const std::string& blockId = eventData.first;
        m_statesRegistry.storeInitialBlockState( sceneId, blockId, initialState );
    }
    for( const auto& operatorData : operators ) {
        const std::string& blockId = operatorData.first;
        m_statesRegistry.storeInitialBlockState( sceneId, blockId, initialState );
    }
}

ControllerInterface::Item SceneManager::getItem( const ValueSourceKey& searchKey ) {
    auto controllerInterface = m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() );
    return controllerInterface->getItem( searchKey.getSourceId() );
}

ItemEvent::ItemValue SceneManager::fetchSourceValue( const ValueSourceKey& key ) {
    using ExpressionStatus = Expressions::ExpressionManager::FetchResult::Status;
    if( key.isExpression() ) {
        auto& expressionName = key.getSourceId();
        auto result = m_expressions.fetchValue( expressionName );

        if( ExpressionStatus::NotFound == result.status )
            throw SceneException( Errors::SCENES_EXPRESSION_DOES_NOT_EXIST,
                                  "Expression does not exist: " + expressionName );

        if( !result.isOk() )
            throw SceneException( Errors::SCENES_EXPRESSION_ERROR, "Expression fetch failed: " + expressionName );

        auto valueType = m_expressions.findValueType( expressionName );
        if( !valueType.empty() )
            return ItemValueWrapper::fromValueAndType( result.value, valueType );
        return ItemValueWrapper( std::move( result.value ) );
    }
    else if( key.isItem() ) {
        Json item;
        if( !m_itemCache.fetchItem( key, item ) || item.empty() ) {
            throw SceneException( Errors::SCENES_ITEM_DOES_NOT_EXIST, "Item does not exist: " + key.getSourceId() );
        }
        return ItemValueWrapper( std::move( item ) );
    }
    throw SceneException( Errors::INTERNAL_OPERATION_ERROR, "Unhandled type of value key" );
}

TransactionalEventsUpdater SceneManager::makeEventsUpdater( const std::string& sceneContextId ) {
    return TransactionalEventsUpdater(
        [this, sceneContextId]() {
            m_actionsRunRequests.runActions( sceneContextId, [this]( const std::string& contextId ) {
                if( m_sceneActionContextManager ) {
                    m_sceneActionContextManager->removeContext( contextId );
                }
            } );
        },
        sceneContextId );
}

void SceneManager::onItemChange( const ControllerInterface::Item& item,
                                 ItemEventBasePtr itemEvent,
                                 bool updateSilently ) {
    LOGGER_TRACE_RAW( "onItemChange: " + cc::j( item ) );

    try {
        ItemValueWrapper itemValue( item );
        if( !itemValue.isValid() ) {
            LOGGER_WARN_RAW( cc::warn( "Got invalid item value in: onItemChange" ) );
            return;
        }

        bool state = itemEvent->compare( itemValue );
        updateEventState( *itemEvent, state, updateSilently );

        if( !itemEvent->getValueKeys().empty() ) {
            std::string controllerId = json_get_str( item, "controllerId" );
            if( controllerId.empty() ) {
                controllerId = HubInfoHelper::getPkDevice();
            }

            std::string itemId = json_get_str( item, "_id" );
            ValueSourceKey key( controllerId, itemId );
            if( m_itemCache.hasKey( key ) ) {
                m_itemCache.updateItem( key, item );
            }
            else {
                m_itemCache.addItem( key, item );
            }
        }
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to process onItemChange: " + HubPlatform::CommonUtils::whatError( e ) ) );
    }
}

void SceneManager::onExpressionChange( const ItemEvent::ItemValue& value, ItemEventBasePtr itemEvent ) {
    LOGGER_TRACE( cc::j( value.getPayload() ) );

    try {
        bool state = itemEvent->compare( value );
        updateEventState( *itemEvent, state, true /* initSceneSubtree */ );
    }
    catch( const std::exception& e ) {
        LOGGER_WARN_RAW(
            cc::warn( "Unable to process onExpressionChange: " + HubPlatform::CommonUtils::whatError( e ) ) );
    }
}

void SceneManager::addValueDrivenEvents( EventContainer& events, bool initSceneSubtree ) {
    auto addItemBaseEvents = [this, initSceneSubtree](
                                 ItemEvents& target, const std::vector< EventContainer::ItemEventBaseEvent >& source ) {
        for( const auto& event : source ) {
            const ValueSourceKey& searchKey = event.first;
            const ItemEventBasePtr& itemEvent = event.second;
            target.emplace( searchKey, itemEvent );
            registerValueDrivenEvent( searchKey, itemEvent, initSceneSubtree );
            cacheExpressionItem( searchKey, *itemEvent );
        }
    };

    auto addItemEvents = [this, initSceneSubtree]( DeviceItemGroupEvents& target,
                                                   const std::vector< EventContainer::ItemEvent >& source ) {
        for( const auto& event : source ) {
            const ValueSourceKey& searchKey = event.first;
            const ItemEventPtr& itemEvent = event.second;
            target.emplace( searchKey, itemEvent );
        }
    };

    addItemBaseEvents( m_valueDrivenEvents, events.getCompareNumbersEvents() );
    addItemBaseEvents( m_valueDrivenEvents, events.userLockEvents );
    addItemBaseEvents( m_valueDrivenEvents, events.buttonStateEvents );
    addItemBaseEvents( m_valueDrivenEvents, events.getCompareNumbersRangeEvents() );
    addItemBaseEvents( m_valueDrivenEvents, events.getCompareStringsEvents() );
    addItemBaseEvents( m_valueDrivenEvents, events.getCompareValuesEvents() );
    addItemBaseEvents( m_valueDrivenEvents, events.getStringOperationEvents() );
    addItemBaseEvents( m_valueDrivenEvents, events.getInArrayEvents() );
    addItemBaseEvents( m_valueDrivenEvents, events.getDeviceItemGroupsEvents() );

    addItemBaseEvents( m_isItemStateChangedEvents, events.getItemStateChangedEvents() );
    addItemEvents( m_valueDeviceItemGroupEvents, events.getItemGroupStateEvents() );

    addItemStateEvents( events, initSceneSubtree );
    addDeviceStateEvents( events, initSceneSubtree );
    addDictionaryValueEvents( events );
    addHotzoneEvents( events );

    for( auto& eventData : events.dictionaryChangedEvents ) {
        m_dictionaryChangedEvents.emplace( std::move( eventData ) );
    }
}

void SceneManager::addHotzoneEvents( EventContainer& events ) {
    for( auto& event : events.hotzoneEvents ) {
        const ValueSourceKey& key = event.first;
        const HotzoneEventPtr& eventPtr = event.second;
        m_hotzoneEvents.insert( event );

        auto controllerInterface = m_controllerFactoryInterface.getControllerInterface( key.getControllerId() );
        controllerInterface->subscribeOnItemChanges(
            key.getSourceId(), [this, eventPtr]( const ControllerInterface::Item& item ) {
                LOGGER_TRACE_RAW( "subscribeOnItemChanges: " + cc::j( item ) );

                try {
                    auto valueIt = item.find( "value" );
                    if( valueIt != item.end() && valueIt->is_object() ) {
                        const bool state = eventPtr->compare( *valueIt );
                        eventPtr->setOperatorState( state );
                    }
                }
                catch( const std::exception& e ) {
                    LOGGER_WARN_RAW( cc::warn( "Unable to add " + whatEvent( *eventPtr ) + " " +
                                               HubPlatform::CommonUtils::whatError( e ) ) );
                }
            } );
    }
}

HubPlatform::CoreD::Model::FeaturePtr SceneManager::getFeature( const std::string& name ) {
    HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( m_hostService );
    HubPlatform::CoreD::Model::FeaturePtr feature = coreRemoteInterface.findFeature( name );
    return feature;
}

void SceneManager::addNonItemEvents( EventContainer& events, bool initSceneSubtree ) {
    for( auto& sunStateEvent : events.sunStateEvents ) {
        if( sunStateEvent->isRangeEvent() ) {
            auto rangeEventBase = std::static_pointer_cast< DateRangeEventBase >( sunStateEvent );
            const bool state = m_schedulerHandler.evaluateCurrentStateForEvent( *rangeEventBase );
            updateEventState( *rangeEventBase, state, initSceneSubtree );
            m_schedulerHandler.push( rangeEventBase );
        }
        else {
            auto dateEventBase = std::static_pointer_cast< DateEventBase >( sunStateEvent );
            m_schedulerHandler.push( dateEventBase );
        }
    }

    for( auto& dateEvent : events.dateEvents ) {
        m_schedulerHandler.push( dateEvent );  // no initialization needed
    }
    for( auto& intervalEvent : events.intervalEvents ) {
        m_schedulerHandler.push( intervalEvent );  // no initialization needed
    }
    for( auto& dateRangeEvent : events.dateRangeEvents ) {
        const bool state = m_schedulerHandler.evaluateCurrentStateForEvent( *dateRangeEvent );
        updateEventState( *dateRangeEvent, state, initSceneSubtree );
        m_schedulerHandler.push( dateRangeEvent );
    }

    for( auto& event : events.houseModeChangedFromEvents ) {
        m_houseModeChangedFromEvents.emplace( std::move( event ) );
    }

    const std::string currentHouseModeId = m_coreInterface.getCurrentHouseModeId();
    for( auto& eventData : events.houseModeChangedToEvents ) {
        const HouseModeEventPtr& event = eventData.second;
        updateEventState( *event, event->compare( currentHouseModeId ), initSceneSubtree );
        m_houseModeChangedToEvents.emplace( std::move( eventData ) );
    }

    for( auto& eventData : events.houseModeAlarmPhaseRangeEvents ) {
        const HouseModeAlarmPhaseRangeEventPtr& event = eventData.second;
        std::string phase;
        std::string status;
        std::tie( phase, status ) = m_coreInterface.getAlarmPhase();
        updateEventState( *event, event->compare( phase, status ), initSceneSubtree );
        m_houseModeAlarmPhaseRangeEvents.emplace( std::move( eventData ) );
    }

    for( auto& eventData : events.houseModeSwitchToRangeEvents ) {
        const HouseModeSwitchToRangeEventPtr& event = eventData.second;
        std::string status = m_coreInterface.getSwitchToPhase();
        updateEventState( *event, event->compare( status ), initSceneSubtree );
        m_houseModeSwitchToRangeEvents.emplace( std::move( eventData ) );
    }

    for( auto& eventData : events.featuresStatusEvents ) {
        const FeatureStateEventPtr& event = eventData;
        std::string name = event->getFeatureName();
        std::string status;
        std::string version;

        // get feature info
        auto feature = getFeature( name );
        if( feature.get() ) {
            auto status = feature->getStatus();
            auto version = feature->getVersion();
            updateEventState( *event, event->compare( name, status, version.toString() ), initSceneSubtree );
        }

        m_featuresStateEvents.emplace_back( std::move( eventData ) );
    }

    for( auto& event : events.batteryStateEvents ) {
        m_batteryStateEvents.emplace_back( std::move( event ) );
    }

    for( auto& event : events.batteryLevelEvents ) {
        m_batteryLevelEvents.emplace_back( std::move( event ) );
    }

    for( auto& event : events.deviceStateEvents ) {
        m_devicesStateEvents.emplace( std::move( event ) );
    }

    for( auto& event : events.networkMainStateEvents ) {
        try {
            event->compare( m_mainNetworkInterface );
            m_networkMainStateEvents.emplace_back( std::move( event ) );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW(
                cc::warn( "Unable to add " + whatEvent( *event ) + " : " + HubPlatform::CommonUtils::whatError( e ) ) );
        }
    }

    for( auto& event : events.networkFailedStateEvents ) {
        event->setMainInterfaceCallback( [this]() { return m_mainNetworkInterface; } );
        m_networkFailedStateEvents.emplace_back( std::move( event ) );
    }

    for( auto& event : events.sceneStateEvents ) {
        m_sceneStateEvents.emplace_back( std::move( event ) );
    }

    for( auto& event : events.groupStateEvents ) {
        m_groupStateEvents.emplace_back( std::move( event ) );
    }

    for( auto& event : events.cloudStateEvents ) {
        m_cloudStateEvents.emplace_back( std::move( event ) );
    }

    for( auto& event : events.firmwareUpdateStateEvents ) {
        m_firmwareUpdateStateEvents.emplace_back( std::move( event ) );
    }
}

void SceneManager::addDeviceStateEvents( const EventContainer& events, bool initSceneSubtree ) {
    for( auto& event : events.getIsDeviceStateEvents() ) {
        try {
            const ValueSourceKey& searchKey = event.first;
            const DeviceStateEventPtr& eventPtr = event.second;
            auto controllerInterface =
                m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() );

            const auto& deviceId = searchKey.getSourceId();
            controllerInterface->subscribeOnDevicesChanges(
                deviceId, [this, eventPtr, deviceId, initSceneSubtree, controllerInterface]() {
                    auto deviceInfo = controllerInterface->getDevice( deviceId );
                    auto state = eventPtr->compare( deviceInfo );
                    updateEventState( *eventPtr, state, initSceneSubtree );
                } );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( "Unable to add " + whatEvent( *event.second ) + " " +
                                       HubPlatform::CommonUtils::whatError( e ) ) );
        }
    }
}

void SceneManager::addItemStateEvents( const EventContainer& events, bool initSceneSubtree ) {
    for( auto& event : events.getItemEvents() ) {
        try {
            const ValueSourceKey& searchKey = event.first;
            const ItemEventPtr& eventPtr = event.second;
            LOGGER_TRACE( "itemEvent: " + cc::j( eventPtr->getValue() ) );
            m_itemArmedEvents.emplace( searchKey, eventPtr );

            auto controllerInterface =
                m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() );
            controllerInterface->subscribeOnItemChanges(
                searchKey.getSourceId(),
                [this, controllerInterface, eventPtr, initSceneSubtree]( const ControllerInterface::Item& item ) {
                    LOGGER_TRACE_RAW( "subscribeOnItemChanges: " + cc::j( item ) );
                    const auto deviceId = getDeviceId( item );
                    controllerInterface->subscribeOnDevicesChanges( deviceId );

                    const auto& itemId = eventPtr->getId();
                    LOGGER_TRACE_RAW( "subscribeOnItemChanges: " + std::to_string( m_itemToDevice.size() ) );
                    ItemToDeviceInfo& info = m_itemToDevice[ itemId ];
                    info.deviceId = deviceId;
                    info.counter++;
                    m_deviceToItem[ deviceId ].insert( itemId );

                    bool state = eventPtr->compare( getItemEvent( itemId, item ) );
                    updateEventState( *eventPtr, state, initSceneSubtree );
                } );

            cacheExpressionItem( searchKey, *eventPtr );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( "Unable to add " + whatEvent( *event.second ) + " " +
                                       HubPlatform::CommonUtils::whatError( e ) ) );
        }
    }
}

void SceneManager::addDictionaryValueEvents( EventContainer& events ) {
    for( auto& event : events.dictionaryValueEvents ) {
        const ValueSourceKey& searchKey = event.first;
        const DictionaryValueEventPtr& eventPtr = event.second;
        m_dictionaryValueEvents.emplace( searchKey, eventPtr );

        auto controllerInterface = m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() );
        controllerInterface->subscribeOnItemChanges(
            searchKey.getSourceId(), [this, eventPtr]( const ControllerInterface::Item& item ) {
                bool state = hasDictionaryElement( item, eventPtr->getElementValue() );
                eventPtr->setOperatorState( state );
            } );
    }
}

bool SceneManager::setSceneEnabled( const Scene::SceneID& id, bool enabled ) {
    if( SceneSharedPtr scenePtr = findSceneById( id ) ) {
        if( scenePtr->isGroup() )
            return setSceneGroupEnabled( scenePtr->getGroupId(), enabled );
        else {
            processSceneStateEvents( id, enabled );
            scenePtr->setEnabled( enabled );
            return true;
        }
    }
    return false;
}

/** @brief Deletes a scene from a collection. Do not use in loops by scenes collection! */
bool SceneManager::deleteScene( const Scene::SceneID& sceneId, bool isGlobal, bool isEditing ) {
    LOGGER_TRACE( "start" );
    auto sceneIt = m_scenes.find( sceneId );
    if( sceneIt == m_scenes.end() || sceneIt->second.scenePtr->isGlobal() != isGlobal ) {
        LOGGER_WARN( "scene not found" );
        return false;
    }

    /* TODO: fix this hack when we implement a partial scene edit;
     *       now, during scene editing, we delete a whole scene (both a c++ representation and a db entry)
     *       which causes Actions to be destroyed and then recreate them again even if they weren't changed;
     *       runCustomScriptAction RAIIs an anonymous plugin and such behavior would delete the anonymous plugin
     *       each time we edit a scene, which is not what we need... */
    if( isEditing ) {
        m_runCustomScriptActions =
            sceneIt->second.actionExecutorPtr->getActions( Model::Resources::ActionNames::RunCustomScriptAction );
    }

    m_statesRegistry.removeRecordsFromScene( sceneId );
    cleanSceneDependencies( sceneId );
    m_scenes.erase( sceneIt );
    LOGGER_TRACE( "exit" );
    return true;
}

void SceneManager::foreachScenes( SceneFuncProcT func ) const {
    for( const auto& scene : m_scenes ) {
        if( func( scene.second.scenePtr ) ) {
            return;
        }
    }
}

void SceneManager::setClock( IClock& clock ) {
    m_clock = clock;
    m_schedulerHandler.setClock( clock );
}

void SceneManager::onSystemTimeChanged() {
    m_schedulerHandler.onSystemTimeChanged();
}

AbstractOperator::SharedOperator SceneManager::findBlockOperator( const Scene::SceneID& sceneId,
                                                                  const BlockId& blockId ) const {
    auto itScene = m_scenes.find( sceneId );
    if( itScene != m_scenes.end() ) {
        const SceneData& sceneData = itScene->second;

        EventBasePtr event = sceneData.findEvent( blockId );
        if( event )
            return event->getOperator();

        return sceneData.findOperator( blockId );
    }

    return nullptr;
}

SceneManager::DeviceId SceneManager::getDeviceId( const JsonObject& item ) {
    if( item.empty() ) {
        LOGGER_DEBUG( "empty item from core" );
        return std::string();
    }
    std::string deviceId;
    if( !json_get( item, "deviceId", deviceId ) ) {
        LOGGER_DEBUG( "empty deviceId from core" );
        return std::string();
    }
    return deviceId;
}

ItemEvent SceneManager::getItemEvent( const ItemEvent::ItemID& itemId, const JsonObject& item ) {
    ItemValueWrapper itemValue( item );
    if( itemValue.empty() ) {
        return ItemEvent();
    }
    if( !itemValue.isValid() ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to getItemEvent: invalid payload" ) );
        return ItemEvent();
    }
    ItemEvent::ArmedState armedState = ItemEvent::ArmedState::EMPTY;
    bool armed;
    if( json_get( item, "deviceArmed", armed ) ) {
        armedState = ItemEvent::toArmedState( armed );
        LOGGER_TRACE( "armed: " + cc::flag( armed ) );
    }
    return ItemEvent( itemId, makeItemValueProvider( std::move( itemValue ) ), armedState );
}

JsonObject SceneManager::getItemValue( const JsonObject& item ) {
    if( isItemEmpty( item ) ) {
        return JsonObject();
    }
    JsonObject value;
    if( !json_get( item, "value", value ) ) {
        LOGGER_DEBUG( "empty value from core" );
        return JsonObject();
    }
    return value;
}

bool SceneManager::hasDictionaryElement( const JsonObject& item, const JsonObject& elementValue ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    if( isItemEmpty( item ) ) {
        return false;
    }
    auto valueIt = item.find( ItemFields::ValueFieldName );
    if( valueIt == item.end() ) {
        return false;
    }
    for( const auto& element : *valueIt ) {
        if( element == elementValue ) {
            return true;
        }
    }
    return false;
}

bool SceneManager::isItemEmpty( const JsonObject& item ) {
    if( item.empty() ) {
        LOGGER_DEBUG( "empty item from core" );
        return true;
    }
    return false;
}

void SceneManager::handleSceneProgressBroadcast( const JsonObject& message ) {
    auto sceneId = json_get_str( message, "scene_id" );
    auto status = json_get_str( message, "status" );
    SceneStateEvent::SceneResult sceneResult;
    if( status == "started" ) {
        return;
    }
    if( status == "failed" ) {
        sceneResult = SceneStateEvent::SceneResult::Failed;
    }
    else if( status == "finished" ) {
        sceneResult = SceneStateEvent::SceneResult::Finished;
    }
    else if( status == "partially_finished" ) {
        sceneResult = SceneStateEvent::SceneResult::PartiallyFinished;
    }
    else if( status == "stopped" ) {
        sceneResult = SceneStateEvent::SceneResult::Stopped;
    }

    processSceneStateEvents( sceneId, sceneResult );
}

SceneManager::TimeScenesList SceneManager::getTimeScenesList() {
    using namespace GregorianCalendar;
    TimeScenesList timeList;
    auto schedulePoints = m_schedulerHandler.getSchedule();
    for( const auto& point : schedulePoints ) {
        Date eventDate;
        const auto& eventTimePoint = point.m_eventTime;
        if( eventTimePoint.isLocalTime() ) {
            eventDate = Date::fromLocalTime( eventTimePoint.duration(), eventTimePoint.offset() );
        }
        else {
            eventDate = Date::fromUtcTime( eventTimePoint.duration(), eventTimePoint.offset() );
        }

        TimeScene timeScene;
        timeScene.id = point.sceneId;
        timeScene.date = eventDate.toString();
        timeScene.name = point.name;
        timeScene.method = point.method;
        timeList.emplace( std::move( timeScene ) );
    }
    return timeList;
}

bool SceneManager::isSceneExist( const Scene::SceneID& id ) const {
    bool sceneExists = false;
    auto equalFunc = [&sceneExists, &id]( const SceneSharedPtr& sceneObj ) -> bool {
        bool exists = sceneObj->getId() == id;
        sceneExists = exists;
        return exists;
    };
    foreachScenes( equalFunc );
    return sceneExists;
}

bool SceneManager::runScene( const Scene::SceneID& sceneId,
                             bool isGlobal,
                             RunSceneActionBlockType block,
                             const std::string& groupId ) {
    auto it = m_scenes.find( sceneId );
    if( it == m_scenes.end() || it->second.scenePtr->isGlobal() != isGlobal ) {
        LOGGER_TRACE( cc::trace( "scene is not found, id=" ) + cc::note( sceneId ) );
        return false;
    }
    auto& sceneData = it->second;
    if( sceneData.hasError() ) {
        LOGGER_TRACE( cc::trace( "cannot run scene, it is in the error state, scene id: " ) + cc::note( sceneId ) +
                      ", error: " + cc::j( sceneData.getError() ) );
        return false;
    }
    if( !groupId.empty() && !isWhenBlockExists( sceneData, groupId ) ) {
        LOGGER_TRACE( cc::trace( "scene does not have a when block with id=" ) + cc::note( groupId ) );
        return false;
    }

    auto contextId = createSceneContext( JSON_EMPTY_OBJ );

    switch( block ) {
    case RunSceneActionBlockType::Empty:
        if( !sceneData.actionExecutorPtr->hasThenActions() && sceneData.actionExecutorPtr->hasElseActions() ) {
            sceneData.actionExecutorPtr->runElseActions( "message" );
            return true;
        }
        // Intentional fallthrough
        // "then" actions are executed by default, unless a scene has only "else" actions (see case Empty)
    case RunSceneActionBlockType::Then:
        sceneData.actionExecutorPtr->runThenActions( contextId );
        return true;
    case RunSceneActionBlockType::Else:
        sceneData.actionExecutorPtr->runElseActions( contextId );
        return true;
    case RunSceneActionBlockType::ThenGroups:
        sceneData.actionExecutorPtr->runThenActions( groupId, contextId );
        return true;
    case RunSceneActionBlockType::ElseGroups:
        sceneData.actionExecutorPtr->runElseActions( groupId, contextId );
        return true;
    }
    return false;
}

bool SceneManager::stopScene( const Scene::SceneID& sceneId, bool isGlobal, const RunSceneActionBlockType blockType ) {
    auto it = m_scenes.find( sceneId );
    if( it == m_scenes.end() || it->second.scenePtr->isGlobal() != isGlobal ) {
        LOGGER_TRACE( cc::trace( "scene is not found, id=" ) + cc::note( sceneId ) );
        return false;
    }
    auto& sceneData = it->second;
    if( sceneData.hasError() ) {
        LOGGER_TRACE( cc::trace( "cannot stop scene, it is in the error state, scene id: " ) + cc::note( sceneId ) +
                      ", error: " + cc::j( sceneData.getError() ) );
        return false;
    }

    sceneData.actionExecutorPtr->stopScene( sceneId, blockType );

    return true;
}

void SceneManager::cleanSceneDependencies( const Scene::SceneID& sceneId ) {
    m_schedulerHandler.erase( sceneId );

    deleteValueDrivenEvent( sceneId );
    deleteItemArmedEvent( sceneId );
    deleteDeviceItemGroupEvents( sceneId );
    deleteDictionaryValueEvent( sceneId );
    deleteIsDictionaryChangedEvent( sceneId );
    deleteHotzoneEvent( sceneId );
    deleteHouseModeEvent( m_houseModeChangedToEvents, sceneId );
    deleteHouseModeEvent( m_houseModeChangedFromEvents, sceneId );
    deleteHouseModeAlarmRangeEvent( m_houseModeAlarmPhaseRangeEvents, sceneId );
    deleteHouseModeSwitchToRangeEvent( m_houseModeSwitchToRangeEvents, sceneId );
    deleteNetworkMainStateEvent( sceneId );
    deleteNetworkFailedStateEvent( sceneId );
    deleteSceneStateEvent( sceneId );
    deleteGroupStateEvent( sceneId );
    deleteCloudStateEvent( sceneId );
    deleteBatteryStateEvent( sceneId );
    deleteBatteryLevelEvent( sceneId );
    deleteDeviceStateEvent( sceneId );
}

void SceneManager::deleteDictionaryValueEvent( const Scene::SceneID& sceneId ) {
    for( auto it = m_dictionaryValueEvents.begin(); it != m_dictionaryValueEvents.end(); ) {
        DictionaryValueEventPtr& dictionaryEvent = it->second;
        if( !dictionaryEvent ) {
            LOGGER_ERROR( cc::error( "Empty armed item event" ) );
            ++it;
            continue;
        }
        if( dictionaryEvent->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        const ValueSourceKey& searchKey = it->first;
        m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
            ->unsubscribeFromItemChanges( searchKey.getSourceId() );
        it = m_dictionaryValueEvents.erase( it );
    }
}

void SceneManager::deleteIsDictionaryChangedEvent( const Scene::SceneID& sceneId ) {
    for( auto it = m_dictionaryChangedEvents.begin(); it != m_dictionaryChangedEvents.end(); ) {
        DictionaryChangedEventPtr& dictionaryChangedEvent = it->second;
        if( !dictionaryChangedEvent ) {
            LOGGER_ERROR( cc::error( "Empty IsDictionaryChangedEvent" ) );
            ++it;
            continue;
        }
        if( dictionaryChangedEvent->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        const ValueSourceKey& searchKey = it->first;
        m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
            ->unsubscribeFromItemChanges( searchKey.getSourceId() );
        it = m_dictionaryChangedEvents.erase( it );
    }
}

void SceneManager::deleteHotzoneEvent( const Scene::SceneID& sceneId ) {
    for( auto it = m_hotzoneEvents.begin(); it != m_hotzoneEvents.end(); ) {
        HotzoneEventPtr& event = it->second;
        if( !event ) {
            LOGGER_ERROR( cc::error( "Empty isDetectedInHotzoneEvent" ) );
            ++it;
            continue;
        }
        if( event->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        const ValueSourceKey& searchKey = it->first;
        m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
            ->unsubscribeFromItemChanges( searchKey.getSourceId() );
        it = m_hotzoneEvents.erase( it );
    }
}

void SceneManager::deleteDeviceStateEvent( const Scene::SceneID& sceneId ) {
    for( auto eventIt = m_devicesStateEvents.begin(); eventIt != m_devicesStateEvents.end(); ) {
        DeviceStateEventPtr& event = eventIt->second;
        if( !event ) {
            LOGGER_ERROR( cc::error( "Empty deleteDeviceStateEvent" ) );
            ++eventIt;
            continue;
        }
        if( event->getSceneId() != sceneId ) {
            ++eventIt;
            continue;
        }
        const ValueSourceKey& searchKey = eventIt->first;
        m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
            ->unsubscribeFromDevicesChanges( searchKey.getSourceId() );

        eventIt = m_devicesStateEvents.erase( eventIt );
    }
}

void SceneManager::deleteItemArmedEvent( const Scene::SceneID& sceneId ) {
    for( auto itemEventIt = m_itemArmedEvents.begin(); itemEventIt != m_itemArmedEvents.end(); ) {
        const ValueSourceKey& searchKey = itemEventIt->first;
        ItemEventPtr& itemEvent = itemEventIt->second;
        if( !itemEvent ) {
            LOGGER_ERROR( cc::error( "Empty armed item event" ) );
            ++itemEventIt;
            continue;
        }
        if( itemEvent->getSceneId() != sceneId ) {
            ++itemEventIt;
            continue;
        }
        const ItemEvent::ItemID& itemId = itemEvent->getId();
        const auto itemToDeviceIt = m_itemToDevice.find( itemId );
        if( itemToDeviceIt != m_itemToDevice.end() ) {
            const ItemEvent::DeviceID& deviceId = itemToDeviceIt->second.deviceId;
            Counter& counter = itemToDeviceIt->second.counter;
            counter--;
            if( counter <= 0 ) {
                const auto deviceToItemIt = m_deviceToItem.find( deviceId );
                if( deviceToItemIt != m_deviceToItem.end() ) {
                    auto& itemIds = deviceToItemIt->second;
                    itemIds.erase( itemId );
                    if( itemIds.empty() ) {
                        m_deviceToItem.erase( deviceToItemIt );
                    }
                }
                m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
                    ->unsubscribeFromDevicesChanges( { deviceId } );
                m_itemToDevice.erase( itemToDeviceIt );
            }
        }
        m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
            ->unsubscribeFromItemChanges( searchKey.getSourceId() );
        itemEventIt = m_itemArmedEvents.erase( itemEventIt );
    }
}

void SceneManager::deleteDeviceItemGroupEvents( const Scene::SceneID& sceneId ) {
    for( auto itemEventIt = m_valueDeviceItemGroupEvents.begin(); itemEventIt != m_valueDeviceItemGroupEvents.end(); ) {
        const ValueSourceKey& searchKey = itemEventIt->first;
        ItemEventPtr& itemGroupEvent = itemEventIt->second;
        if( !itemGroupEvent ) {
            LOGGER_ERROR( cc::error( "Empty armed item event" ) );
            ++itemEventIt;
            continue;
        }
        if( itemGroupEvent->getSceneId() != sceneId ) {
            ++itemEventIt;
            continue;
        }
        itemEventIt = m_valueDeviceItemGroupEvents.erase( itemEventIt );
    }
}

void SceneManager::deleteValueDrivenEvent( const Scene::SceneID& sceneId ) {
    for( auto it = m_valueDrivenEvents.begin(); it != m_valueDrivenEvents.end(); ) {
        const ValueSourceKey& searchKey = it->first;
        ItemEventBasePtr& itemEvent = it->second;
        if( !itemEvent ) {
            LOGGER_ERROR( cc::error( "Empty item event" ) );
            ++it;
            continue;
        }
        if( itemEvent->getSceneId() == sceneId ) {
            if( searchKey.isItem() ) {
                m_controllerFactoryInterface.getControllerInterface( searchKey.getControllerId() )
                    ->unsubscribeFromItemChanges( searchKey.getSourceId() );
            }
            it = m_valueDrivenEvents.erase( it );
        }
        else {
            ++it;
        }
    }
}

void SceneManager::deleteHouseModeEvent( HouseModeEvents& events, const Scene::SceneID& sceneId ) {
    for( auto it = events.begin(); it != events.end(); ) {
        HouseModeEventPtr& event = it->second;
        if( !event ) {
            LOGGER_ERROR( cc::error( "Empty house mode event" ) );
            ++it;
            continue;
        }
        if( event->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        it = events.erase( it );
    }
}

void SceneManager::deleteHouseModeAlarmRangeEvent( HouseModeAlarmPhaseRangeEvents& events,
                                                   const Scene::SceneID& sceneId ) {
    for( auto it = events.begin(); it != events.end(); ) {
        HouseModeAlarmPhaseRangeEventPtr& event = it->second;
        if( !event ) {
            LOGGER_ERROR( cc::error( "Empty house mode event" ) );
            ++it;
            continue;
        }
        if( event->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        it = events.erase( it );
    }
}

void SceneManager::deleteHouseModeSwitchToRangeEvent( HouseModeSwitchToRangeEvents& events,
                                                      const Scene::SceneID& sceneId ) {
    for( auto it = events.begin(); it != events.end(); ) {
        HouseModeSwitchToRangeEventPtr& event = it->second;
        if( !event ) {
            LOGGER_ERROR( cc::error( "Empty house mode event" ) );
            ++it;
            continue;
        }
        if( event->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        it = events.erase( it );
    }
}

template < class Type >
void deleteEvent( Type& events, const Scene::SceneID& sceneId, const std::string& errorMessage ) {
    for( auto it = events.begin(); it != events.end(); ) {
        auto& event = *it;
        if( !event ) {
            LOGGER_ERROR( cc::error( errorMessage ) );
            ++it;
            continue;
        }
        if( event->getSceneId() != sceneId ) {
            ++it;
            continue;
        }
        it = events.erase( it );
    }
}

void SceneManager::deleteBatteryStateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_batteryStateEvents, sceneId, "Empty battery state event" );
}

void SceneManager::deleteBatteryLevelEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_batteryLevelEvents, sceneId, "Empty battery level event" );
}

void SceneManager::deleteNetworkMainStateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_networkMainStateEvents, sceneId, "Empty network main state event" );
}

void SceneManager::deleteNetworkFailedStateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_networkFailedStateEvents, sceneId, "Empty network failed state event" );
}

void SceneManager::deleteSceneStateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_sceneStateEvents, sceneId, "Empty scene state event" );
}

void SceneManager::deleteGroupStateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_groupStateEvents, sceneId, "Empty group state event" );
}

void SceneManager::deleteCloudStateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_cloudStateEvents, sceneId, "Empty cloud state event" );
}

void SceneManager::deleteFirmwareUpdateEvent( const Scene::SceneID& sceneId ) {
    deleteEvent( m_firmwareUpdateStateEvents, sceneId, "Empty firmware update event" );
}

std::string SceneManager::createSceneContext( const JsonObject& message ) {
    std::string contextId;
    try {
        if( m_sceneActionContextManager ) {
            int64_t now = TimeUtils::getCurrentTime();
            contextId = m_sceneActionContextManager->createNewExecutionContext( "", now, message );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "SceneManager create new scene context error: " + ex.what() );
    }

    return contextId;
}

void SceneManager::processExpressionValueChanged( const std::string& expressionName,
                                                  const Json& expressionValue,
                                                  const JsonObject& message ) {
    LOGGER_DEBUG( cc::debug( "name = " ) + cc::note( expressionName ) + cc::debug( ", value = " ) +
                  cc::j( expressionValue ) );

    LOGGER_ERROR( "SceneManager::processExpressionValueChanged" );

    auto sceneContextId = createSceneContext( message );

    TransactionalEventsUpdater updater = makeEventsUpdater( sceneContextId );

    for( const auto& event : m_itemArmedEvents ) {
        const ValueSourceKey& key = event.first;

        auto callback = [this, &updater, &key]( const ItemEventPtr& event, const ItemValueWrapper& itemValue ) {
            ItemEvent::ArmedState armedState;
            if( findArmedStateForItem( key, armedState ) )
                requestUpdateGeneric( updater, event, itemValue, armedState, ItemEvent::EventType::ITEM );
        };
        handleExpressionEvent( *this, key, event.second, expressionName, expressionValue, callback );
    }

    for( const auto& event : m_valueDrivenEvents ) {
        auto callback = [&updater]( const ItemEventBasePtr& event, const ItemValueWrapper& value ) {
            requestUpdateGeneric( updater, event, value );
        };
        handleExpressionEvent( *this, event.first, event.second, expressionName, expressionValue, callback );
    }

    TransactionalEventsUpdater::Events events;
    for( auto& eventData : m_isItemStateChangedEvents ) {
        const ValueSourceKey& eventKey = eventData.first;
        const ItemEventBasePtr& event = eventData.second;
        if( eventKey.isExpression() && eventKey.getSourceId() == expressionName ) {
            if( event->compare( expressionValue ) )
                events.push_back( event );
        }
    }

    updater.requestTrigger( events );
    updater.flush();
    updater.requestReset( events );
    updater.flush();
}

void SceneManager::cacheExpressionItem( const ValueSourceKey& searchKey, const EventBase& event ) {
    for( const ValueSourceKey& eventValueKey : event.getValueKeys() ) {
        if( eventValueKey.isExpression() && searchKey.isItem() && !m_itemCache.hasKey( searchKey ) ) {
            LOGGER_TRACE( cc::trace( "Add item to a cache: (" ) + searchKey.getControllerId() + ", " +
                          searchKey.getSourceId() + ")" );
            Json coreItem;
            if( !m_itemCache.fetchItem( searchKey, coreItem ) ) {
                LOGGER_ERROR( cc::error( "Item to place in cache not found: (" ) + searchKey.getControllerId() + ", " +
                              searchKey.getSourceId() + ")" );
            }
            break;
        }
    }
}

bool SceneManager::isWhenBlockExists( const SceneData& scene, const std::string& whenBlockId ) {
    return scene.triggerOperators.count( whenBlockId ) || scene.triggerEvents.count( whenBlockId );
}

HubPlatform::Services::TaskLoop& SceneManager::getTaskLoop() {
    return m_hostService;
}

bool SceneManager::SceneData::hasError() const {
    if( scenePtr )
        return !scenePtr->getErrorData().empty();
    return false;
}

void SceneManager::SceneData::setError( Json value ) {
    if( scenePtr )
        return;
    scenePtr->setErrorData( std::move( value ) );
}

Json SceneManager::SceneData::getError() const {
    if( scenePtr && !scenePtr->getErrorData().empty() )
        return scenePtr->getErrorData();
    return {};
}

EventBasePtr SceneManager::SceneData::findEvent( const BlockId& blockId ) const {
    auto triggersIt = triggerEvents.find( blockId );
    if( triggersIt != triggerEvents.end() )
        return triggersIt->second;

    auto exceptionsIt = exceptionEvents.find( blockId );
    if( exceptionsIt != exceptionEvents.end() )
        return exceptionsIt->second;

    auto globalRestrictionIt = globalRestrictionEvents.find( blockId );
    if( globalRestrictionIt != globalRestrictionEvents.end() )
        return globalRestrictionIt->second;

    return nullptr;
}

AbstractOperator::SharedOperator SceneManager::SceneData::findOperator( const BlockId& blockId ) const {
    auto triggersIt = triggerOperators.find( blockId );
    if( triggersIt != triggerOperators.end() )
        return triggersIt->second;

    auto exceptionsIt = exceptionOperators.find( blockId );
    if( exceptionsIt != exceptionOperators.end() )
        return exceptionsIt->second;

    auto globalRestrictionIt = globalRestrictionOperators.find( blockId );
    if( globalRestrictionIt != globalRestrictionOperators.end() )
        return globalRestrictionIt->second;

    return nullptr;
}

void SceneManager::SceneData::forEachOperator( const OperatorCallback& callback ) const {
    for( const auto& operatorWithIdPair : triggerOperators )
        callback( operatorWithIdPair.second );

    for( const auto& operatorWithIdPair : exceptionOperators )
        callback( operatorWithIdPair.second );

    for( const auto& operatorWithIdPair : globalRestrictionOperators )
        callback( operatorWithIdPair.second );
}

Json SceneManager::buildSceneStatus( const Scene::SceneID& sceneId ) const {
    auto it = m_scenes.find( sceneId );
    if( it != m_scenes.end() ) {
        const SceneData& sceneData = it->second;

        SceneStatusBuilder statusBuilder( m_statesRegistry );
        return statusBuilder.build( sceneId,
                                    sceneData.triggerEvents,
                                    sceneData.exceptionEvents,
                                    sceneData.globalRestrictionEvents,
                                    sceneData.getError() );
    }

    LOGGER_WARN( cc::warn( "Requested to build a status of a non-existing scene: " ) + cc::note( sceneId ) );
    return {};
}

bool SceneManager::findArmedStateForItem( const ValueSourceKey& key, ItemEvent::ArmedState& armedState ) const {
    auto itemIt = m_itemToDevice.find( key.getSourceId() );
    if( itemIt == m_itemToDevice.end() )
        return false;

    const auto& deviceId = itemIt->second.deviceId;
    LOGGER_TRACE( cc::trace( "deviceId: " + deviceId ) );

    auto controllerInterface = m_controllerFactoryInterface.getControllerInterface( key.getControllerId() );
    auto armedResult = controllerInterface->isDeviceArmed( deviceId );
    armedState = armedResult.second ? ItemEvent::toArmedState( armedResult.first ) : ItemEvent::ArmedState::EMPTY;
    LOGGER_TRACE( cc::trace( "armed: " ) + cc::flag( armedResult.first ) );
    return true;
}

SceneManager::ItemArmedEventDataList SceneManager::readItemsDataFromJson( const ControllerId& controllerId,
                                                                          const std::vector< Json >& jsonItems ) const {
    using namespace HubPlatform::CoreD::Model::Resources;

    ItemArmedEventDataList result;
    for( const auto& item : jsonItems ) {
        ItemEvent::ItemID itemId;
        if( !json_get( item, ItemFields::IdFieldName, itemId ) ) {
            LOGGER_DEBUG( cc::debug( "empty item id" ) );
            continue;
        }

        Json payload;
        Json valueField;
        if( !json_get( item, ItemFields::ValueFieldName, valueField ) ) {
            LOGGER_DEBUG( cc::debug( "empty item value" ) );
            continue;
        }
        payload[ ItemFields::ValueFieldName ] = std::move( valueField );

        Json typeField;
        if( json_get( item, ItemFields::TypeFieldName, typeField ) ) {
            payload[ ItemFields::TypeFieldName ] = std::move( typeField );
        }
        else {
            LOGGER_DEBUG( cc::debug( "empty item type" ) );
        }

        ItemEvent::ItemValue itemValue( std::move( payload ) );
        if( !itemValue.isValid() ) {
            LOGGER_DEBUG( cc::debug( "invalid item payload" ) );
            continue;
        }

        auto range = m_itemArmedEvents.equal_range( { controllerId, itemId } );
        for( auto itemEventIt = range.first; itemEventIt != range.second; ++itemEventIt ) {
            result.push_back( { itemEventIt->second, itemValue } );
        }
    }

    return result;
}

bool SceneManager::checkActionGroupID( const Scene::SceneID& sceneId,
                                       const RunSceneActionBlockType& block,
                                       const std::string& groupId ) {
    if( block == RunSceneActionBlockType::ThenGroups || block == RunSceneActionBlockType::ElseGroups ) {
        auto linkedSceneIter = m_scenes.find( sceneId );
        if( linkedSceneIter == m_scenes.end() ) {
            return false;
        }
        if( !isWhenBlockExists( linkedSceneIter->second, groupId ) ) {
            return false;
        }
        return true;
    }
    else
        return false;
}

bool SceneManager::checkActionGroupIDForAll( const RunSceneActionBlockType& block, const std::string& groupId ) {
    if( block == RunSceneActionBlockType::ThenGroups || block == RunSceneActionBlockType::ElseGroups ) {
        for( const auto& scenes : m_scenes ) {
            if( isWhenBlockExists( scenes.second, groupId ) ) {
                return true;
            }
        }
    }
    return false;
}

void SceneManager::processIsDeviceItemGroupEvent( const Json& itemUpdatedMessage ) {
    if( itemUpdatedMessage.empty() ) {
        LOGGER_ERROR_RAW( cc::error( "item update broadcast in empty: " ) + cc::j( itemUpdatedMessage ) );
        return;
    }
    bool isDeviceGroupFetched = false;
    std::set< std::string > matchedDeviceGroups;
    bool isItemGroupFetched = false;
    std::set< std::string > matchedItemGroups;

    for( auto itemEventIt = m_valueDrivenEvents.cbegin(); itemEventIt != m_valueDrivenEvents.cend(); ++itemEventIt ) {
        const ValueSourceKey& eventKey = itemEventIt->first;

        ItemEventBase& event = *itemEventIt->second;
        if( event.getMethodName() != Model::Resources::MethodNames::isDeviceItemGroup ||
            !eventKey.isDeviceItemGroups() ) {
            continue;
        }
        auto deviceItemGroupIds = ValueSourceKey::unpackDeviceItemGroupIds( eventKey.getSourceId() );
        auto& deviceGroupId = deviceItemGroupIds.first;
        auto& itemGroupId = deviceItemGroupIds.second;

        if( deviceGroupId.empty() && itemGroupId.empty() ) {
            LOGGER_ERROR_RAW( cc::error( "Invalid event key for device and item group" ) );
            continue;
        }

        if( !isDeviceGroupFetched && !deviceGroupId.empty() ) {
            matchedDeviceGroups = m_deviceItemGroupManager.matchDeviceGroupsForItemUpdatedEvent( itemUpdatedMessage );
            if( matchedDeviceGroups.count( deviceGroupId ) ) {
                isDeviceGroupFetched = true;
            }
        }

        if( !isItemGroupFetched && !itemGroupId.empty() ) {
            matchedItemGroups = m_deviceItemGroupManager.matchItemGroupsForItemUpdatedEvent( itemUpdatedMessage );
            if( matchedItemGroups.count( itemGroupId ) ) {
                isItemGroupFetched = true;
            }
        }

        try {
            if( isDeviceGroupFetched || isItemGroupFetched ) {
                const bool updateParent = true;
                updateEventState( event, true, updateParent );
            }
            else {
                const bool updateParent = true;
                updateEventState( event, false, updateParent );
            }
        }
        catch( const std::exception& e ) {
            LOGGER_WARN_RAW( cc::warn( eventProcessError( event, e ) ) );
        }
    }
}

void SceneManager::saveHubBackupRunCallbackPtr(
    const Action& action,
    const HubPlatform::ScenesManager::ActionInterface::ActionExecutorPtr& actionExecutorPtr ) {
    m_callbackPtrQueue.push( std::make_shared< HubPlatform::ScenesManager::ActionInterface::SendFinishCallback >(
        action, actionExecutorPtr ) );
}

void SceneManager::setHubBackupRunState( bool flag ) {
    if( !m_callbackPtrQueue.empty() ) {
        m_sendFinishCallbackPtr = m_callbackPtrQueue.front();
        m_sendFinishCallbackPtr->setResult( flag );
        m_sendFinishCallbackPtr.reset();
        m_callbackPtrQueue.pop();
    }
}

void SceneManager::handleHubBackupCreateProgress( const JsonObject& message ) {
    if( !m_callbackPtrQueue.empty() ) {
        LOGGER_DEBUG( "Processing handleHubBackupCreateProgress" + cc::j( message ) );
        if( !message.empty() || message.is_object() ) {
            JsonObject error;
            json_get_obj( message, "error", error );
            auto status = json_get_str( message, "status" );
            if( error.empty() && status == "finished" ) {
                setHubBackupRunState( true );
            }
            else if( !error.empty() ) {
                setHubBackupRunState( false );
            }
        }
    }
}

void SceneManager::handleHubBackupRestoreProgress( const JsonObject& message ) {
    if( !m_callbackPtrQueue.empty() ) {
        LOGGER_DEBUG( "Processing handleHubBackupRestoreProgress" + cc::j( message ) );
        if( !message.empty() || message.is_object() ) {
            JsonObject error;
            json_get_obj( message, "error", error );
            auto status = json_get_int( message, "completed" );
            if( error.empty() && status == 60 ) {
                setHubBackupRunState( true );
            }
            else if( !error.empty() ) {
                setHubBackupRunState( false );
            }
        }
    }
}

void SceneManager::processFeatureStateEvent( const std::string& controllerId,
                                             const std::string& name,
                                             bool status,
                                             const JsonObject& message ) {
    auto feature = getFeature( name );
    std::string version;
    if( feature.get() ) {
        version = feature->getVersion().toString();
    }
    auto sceneContextId = createSceneContext( message );
    processEventGenericCompare( *this, sceneContextId, m_featuresStateEvents, name, status, version );
}

}  // namespace SceneEngine
