#pragma once

#include "scenes_manager/scenes_engine/events/item_event_base.h"
#include "scenes_manager/scenes_engine/events/is_item_state_changed_event.h"
#include "scenes_manager/scenes_engine/events/date_event.h"
#include "scenes_manager/scenes_engine/events/date_range_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_value_event.h"
#include "scenes_manager/scenes_engine/events/dictionary_changed_event.h"
#include "scenes_manager/scenes_engine/events/hotzone_event.h"
#include "scenes_manager/scenes_engine/events/interval_event.h"
#include "scenes_manager/scenes_engine/events/item_event.h"
#include "scenes_manager/scenes_engine/events/sun_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_event.h"
#include "scenes_manager/scenes_engine/events/network_state_event.h"
#include "scenes_manager/scenes_engine/events/scene_state_event.h"
#include "scenes_manager/scenes_engine/events/group_state_event.h"
#include "scenes_manager/scenes_engine/events/cloud_state_event.h"
#include "scenes_manager/scenes_engine/events/battery_state_event.h"
#include "scenes_manager/scenes_engine/events/battery_level_event.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/scenes_engine/events/firmware_update_state_event.h"
#include "scenes_manager/scenes_engine/events/device_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_alarm_phase_range_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_switch_to_range_state_event.h"
#include "scenes_manager/scenes_engine/events/feature_state_event.h"

#include <map>

namespace SceneEngine {

struct EventContainer {
    using ControllerId = std::string;
    using BlockId = std::string;

    using ItemEventBaseEvent = std::pair< ValueSourceKey, ItemEventBasePtr >;
    using DictionaryValueEvent = std::pair< ValueSourceKey, DictionaryValueEventPtr >;
    using DictionaryChangedEvent = std::pair< ValueSourceKey, DictionaryChangedEventPtr >;
    using HotzoneEvent = std::pair< ValueSourceKey, HotzoneEventPtr >;
    using ItemEvent = std::pair< ValueSourceKey, ItemEventPtr >;
    using HouseModeEvent = std::pair< ControllerId, HouseModeEventPtr >;
    using DeviceStateEvent = std::pair< ValueSourceKey, DeviceStateEventPtr >;
    using HouseModeAlarmPhaseRangeEvent = std::pair< ControllerId, HouseModeAlarmPhaseRangeEventPtr >;
    using HouseModeSwitchToRangeEvent = std::pair< ControllerId, HouseModeSwitchToRangeEventPtr >;
    using FeatureStateEvent = std::pair< ControllerId, FeatureStateEventPtr >;

    using UserLockEvents = std::vector< ItemEventBaseEvent >;
    using ButtonStateEvents = std::vector< ItemEventBaseEvent >;
    using CompareNumbersEvents = std::vector< ItemEventBaseEvent >;
    using CompareNumbersRangeEvents = std::vector< ItemEventBaseEvent >;
    using CompareStringsEvents = std::vector< ItemEventBaseEvent >;
    using CompareValuesEvents = std::vector< ItemEventBaseEvent >;
    using StringOperationEvents = std::vector< ItemEventBaseEvent >;
    using InArrayEvents = std::vector< ItemEventBaseEvent >;
    using DateEvents = std::vector< DateEventPtr >;
    using DateRangeEvents = std::vector< DateRangeEventBasePtr >;
    using ItemStateChangedEvents = std::vector< ItemEventBaseEvent >;
    using DeviceItemGroupsEvents = std::vector< ItemEventBaseEvent >;

    using DictionaryValueEvents = std::vector< DictionaryValueEvent >;
    using DictionaryChangedEvents = std::vector< DictionaryChangedEvent >;
    using HotzoneEvents = std::vector< HotzoneEvent >;
    using IntervalEvents = std::vector< IntervalEventPtr >;
    using ItemEvents = std::vector< ItemEvent >;
    using BatteryStateEvents = std::vector< BatteryStateEventPtr >;
    using BatteryLevelEvents = std::vector< BatteryLevelEventPtr >;
    using SunStateEvents = std::vector< SunStateEventPtr >;
    using HouseModeChangedToEvents = std::vector< HouseModeEvent >;
    using HouseModeChangedFromEvents = std::vector< HouseModeEvent >;
    using HouseModeAlarmPhaseRangeEvents = std::vector< HouseModeAlarmPhaseRangeEvent >;
    using HouseModeSwitchToRangeEvents = std::vector< HouseModeSwitchToRangeEvent >;
    using NetworkFailedStateEvents = std::vector< NetworkFailedStateEventPtr >;
    using NetworkMainStateEvents = std::vector< NetworkMainStateEventPtr >;
    using SceneStateEvents = std::vector< SceneStateEventPtr >;
    using GroupStateEvents = std::vector< GroupStateEventPtr >;
    using CloudStateEvents = std::vector< CloudStateEventPtr >;
    using FirmwareUpdateStateEvents = std::vector< FirmwareUpdateStateEventPtr >;
    using DeviceStateEvents = std::vector< DeviceStateEvent >;
    using FeatureStateEvents = std::vector< FeatureStateEventPtr >;

    using Events = std::map< BlockId, EventBasePtr >;
    using Operators = std::map< BlockId, AbstractOperator::SharedOperator >;
    // Item keys assigned to any expression as left part of comparison, add then to ItemCache and fill their values
    using ExpressionItems = std::map< ValueSourceKey, EventBasePtr >;
    // Table of action group IDs mapped to corresponding when block IDs
    using ActionGroupMappings = std::map< std::string, std::string >;

public:
    void addItemEvent( const ValueSourceKey& key, ItemEventPtr eventPtr );
    void addItemStateChangedEvent( const ValueSourceKey& key, ItemStateChangedEventPtr eventPtr );
    void addExpressionItem( const ValueSourceKey& key, EventBasePtr eventPtr );
    void addCompareNumbersEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addCompareNumbersRangeEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addCompareStringsEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addStringOperationEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addInArrayEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addCompareValuesEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addDeviceItemGroupsEvent( const ValueSourceKey& key, ItemEventBasePtr eventPtr );
    void addItemStateDeviceItemGroupsEvent( const ValueSourceKey& key, ItemEventPtr eventPtr );

    const ItemEvents& getItemEvents() const;
    const ItemStateChangedEvents& getItemStateChangedEvents() const;
    const ItemEvents& getItemGroupStateEvents() const;
    const ExpressionItems& getExpressionItems() const;
    const CompareNumbersEvents& getCompareNumbersEvents() const;
    const CompareNumbersRangeEvents& getCompareNumbersRangeEvents() const;
    const CompareStringsEvents& getCompareStringsEvents() const;
    const StringOperationEvents& getStringOperationEvents() const;
    const InArrayEvents& getInArrayEvents() const;
    const CompareValuesEvents& getCompareValuesEvents() const;
    const DeviceItemGroupsEvents& getDeviceItemGroupsEvents() const;
    const HouseModeAlarmPhaseRangeEvents& getHouseModeAlarmPhaseRangeEvents() const;

    const DeviceStateEvents& getIsDeviceStateEvents() const;
    const DeviceStateEvents& getIsDeviceStateGroupEvents() const;
    const FeatureStateEvents& getFeatureStateEvents() const;

    void clear();
    bool empty() const;

    AbstractOperator::SharedOperator findBlockOperator( const BlockId& blockId ) const;

public:
    UserLockEvents userLockEvents;
    ButtonStateEvents buttonStateEvents;
    BatteryStateEvents batteryStateEvents;
    BatteryLevelEvents batteryLevelEvents;
    DateEvents dateEvents;
    DateRangeEvents dateRangeEvents;
    DictionaryValueEvents dictionaryValueEvents;
    DictionaryChangedEvents dictionaryChangedEvents;
    HotzoneEvents hotzoneEvents;
    IntervalEvents intervalEvents;
    SunStateEvents sunStateEvents;
    HouseModeChangedToEvents houseModeChangedToEvents;
    HouseModeChangedFromEvents houseModeChangedFromEvents;
    HouseModeAlarmPhaseRangeEvents houseModeAlarmPhaseRangeEvents;
    HouseModeSwitchToRangeEvents houseModeSwitchToRangeEvents;
    NetworkFailedStateEvents networkFailedStateEvents;
    NetworkMainStateEvents networkMainStateEvents;
    SceneStateEvents sceneStateEvents;
    GroupStateEvents groupStateEvents;
    CloudStateEvents cloudStateEvents;
    FirmwareUpdateStateEvents firmwareUpdateStateEvents;
    DeviceStateEvents deviceStateEvents;
    DeviceStateEvents deviceStateGroupEvents;
    ItemEvents m_itemGroupEvents;
    FeatureStateEvents featuresStatusEvents;

    Events events;
    Operators operators;

    ActionGroupMappings m_actionGroupMappings;

private:
    template < class Event, class EventPtr >
    void addSearchableEvent( std::vector< Event >& container, const ValueSourceKey& key, EventPtr eventPtr );

private:
    ItemEvents m_itemEvents;
    ItemStateChangedEvents m_itemStateChangedEvents;
    CompareNumbersEvents m_compareNumbersEvents;
    CompareNumbersRangeEvents m_compareNumbersRangeEvents;
    CompareStringsEvents m_compareStringsEvents;
    CompareValuesEvents m_compareValuesEvents;
    StringOperationEvents m_stringOperationEvents;
    InArrayEvents m_inArrayEvents;
    DeviceItemGroupsEvents m_deviceItemGroupsEvents;
    FeatureStateEvents m_featureStateEvents;

    ExpressionItems m_expressionItems;
};

}  // namespace SceneEngine
