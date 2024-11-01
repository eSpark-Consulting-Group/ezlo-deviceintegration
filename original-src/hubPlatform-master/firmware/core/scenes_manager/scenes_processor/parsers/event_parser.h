#pragma once

#include "scenes_manager/scenes_engine/event_container.h"
#include "scenes_manager/scenes_engine/events/item_value_provider.h"
#include "scenes_manager/scenes_engine/events/days_of_period_event.h"
#include "scenes_manager/scenes_processor/parsers/event_parser_interface.h"

#include "json_utils/json_utils.h"
#include "base_utils/range.h"

namespace HubPlatform {

namespace UT {
class EventParserTest;
}
namespace Services {
class TaskLoop;
}

namespace ScenesManager {

class EventParser : public EventParserInterface {
public:
    struct Time {
        int hour;
        int minute;
    };

    EventParser( ControllerId controllerId,
                 GetGeoPositionCallback getGeoPosition,
                 ClockGetter clockGetter,
                 MakeGenericValueProvider makeGenericValueProvider,
                 Services::TaskLoop& taskLoop );

public:
    SceneEngine::EventContainer parseTriggers( Json& sceneJsonObject,
                                               IdUtils::PlaceholdersMapping& idsMapping ) override;
    SceneEngine::EventContainer parseExceptions( Json& sceneJsonObject,
                                                 IdUtils::PlaceholdersMapping& idsMapping ) override;
    SceneEngine::EventContainer parseGlobalRestriction( SceneEngine::SceneSharedPtr scenePtr,
                                                        Json& sceneJsonObject,
                                                        IdUtils::PlaceholdersMapping& idsMapping ) override;

    void setClockGetter( ClockGetter clockGetter ) override;

public:
    void parseCompareNumbersEvent( Json& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseCompareStringsEvent( Json& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseCompareValuesEvent( Json& whenBlock,
                                  SceneEngine::EventContainer& eventContainer,
                                  SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsItemStateCondition( JsonObject& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsButtonState( JsonObject& whenBlock,
                             SceneEngine::EventContainer& eventContainer,
                             SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsBatteryState( JsonObject& whenBlock,
                              SceneEngine::EventContainer& eventContainer,
                              SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsBatteryLevel( JsonObject& whenBlock,
                              SceneEngine::EventContainer& eventContainer,
                              SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseHasAtLeastOneDictionaryValueEvent( JsonObject& whenBlock,
                                                 SceneEngine::EventContainer& eventContainer,
                                                 SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsDictionaryChanged( JsonObject& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsDetectedInHotzone( JsonObject& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsUserLockOperation( JsonObject& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseCompareNumberRangeEvent( Json& whenBlock,
                                       SceneEngine::EventContainer& eventContainer,
                                       SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseStringOperationEvent( Json& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseInArrayEvent( Json& whenBlock,
                            SceneEngine::EventContainer& eventContainer,
                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsDeviceItemGroupEvent( Json& whenBlock,
                                      SceneEngine::EventContainer& eventContainer,
                                      SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsItemStateChangedEvent( Json& whenBlock,
                                       SceneEngine::EventContainer& eventContainer,
                                       SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsIntervalCondition( JsonObject& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsSunStateCondition( JsonObject& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsDateCondition( JsonObject& whenBlock,
                               SceneEngine::EventContainer& eventContainer,
                               SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsDateRangeCondition( JsonObject& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsOnceCondition( JsonObject& whenBlock,
                               SceneEngine::EventContainer& eventContainer,
                               SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsHouseModeChangedTo( JsonObject& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsHouseModeChangedFrom( JsonObject& whenBlock,
                                      SceneEngine::EventContainer& eventContainer,
                                      SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsNetworkState( JsonObject& whenBlock,
                              SceneEngine::EventContainer& eventContainer,
                              SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsSceneState( JsonObject& whenBlock,
                            SceneEngine::EventContainer& eventContainer,
                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsGroupState( JsonObject& whenBlock,
                            SceneEngine::EventContainer& eventContainer,
                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsCloudState( JsonObject& whenBlock,
                            SceneEngine::EventContainer& eventContainer,
                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseAndOperator( JsonObject& whenBlock,
                           SceneEngine::EventContainer& eventContainer,
                           SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseNotOperator( JsonObject& whenBlock,
                           SceneEngine::EventContainer& eventContainer,
                           SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseOrOperator( JsonObject& whenBlock,
                          SceneEngine::EventContainer& eventContainer,
                          SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseXorOperator( JsonObject& whenBlock,
                           SceneEngine::EventContainer& eventContainer,
                           SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseFunctionOperator( JsonObject& whenBlock,
                                SceneEngine::EventContainer& eventContainer,
                                SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    void parseIsFirmwareUpdateState( JsonObject& whenBlock,
                                     SceneEngine::EventContainer& eventContainer,
                                     SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsHouseModeAlarmPhaseRange( JsonObject& whenBlock,
                                          SceneEngine::EventContainer& eventContainer,
                                          SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsHouseModeSwitchToRange( JsonObject& whenBlock,
                                        SceneEngine::EventContainer& eventContainer,
                                        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsFeatureState( JsonObject& whenBlock,
                              SceneEngine::EventContainer& eventContainer,
                              SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    void parseIsDeviceState( JsonObject& whenBlock,
                             SceneEngine::EventContainer& eventContainer,
                             SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    static void parseActionGroupMapping( Json& whenBlock, SceneEngine::EventContainer::ActionGroupMappings& );

private:
    using EventParserMethod = void ( EventParser::* )( JsonObject& whenBlock,
                                                       SceneEngine::EventContainer& eventContainer,
                                                       SceneEngine::AbstractOperator::SharedOperator );
    using EventMethodName = std::string;
    using EventParserMethods = std::map< EventMethodName, EventParserMethod >;

    SceneEngine::EventContainer::HouseModeEvent parseIsHouseMode(
        JsonObject& whenBlock,
        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    SceneEngine::EventContainer::HouseModeAlarmPhaseRangeEvent parseHouseModeAlarmPhaseRange(
        JsonObject& whenBlock,
        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    SceneEngine::EventContainer::HouseModeSwitchToRangeEvent parseHouseModeSwitchToRange(
        JsonObject& whenBlock,
        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    SceneEngine::EventContainer::FeatureStateEvent parseFeatureStateEvent(
        JsonObject& whenBlock,
        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );

    SceneEngine::AbstractOperator::SharedOperator parseForFunctionOperator( const Json& options ) const;
    SceneEngine::AbstractOperator::SharedOperator parseRepeatFunctionOperator( const Json& options ) const;
    SceneEngine::AbstractOperator::SharedOperator parseFollowFunctionOperator( const Json& options ) const;
    SceneEngine::AbstractOperator::SharedOperator parsePulseFunctionOperator( const Json& options ) const;
    SceneEngine::AbstractOperator::SharedOperator parseLatchFunctionOperator( const Json& options ) const;

    static JsonArray& getTriggerBlocks( JsonObject& sceneJsonObject );

    void parse( Json& whenBlock,
                SceneEngine::EventContainer& eventContainer,
                SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    SceneEngine::ItemValueComparator convertComparator( const std::string& comparator, bool valueTypeHasIsLess = true );
    void addOperator( SceneEngine::EventBasePtr event,
                      SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr );
    SceneEngine::IntervalEventPtr createIntervalEvent( const JsonObject& whenBlock, const JsonObject& blockOptions );
    SceneEngine::SunStateEventPtr createSunStateEvent( JsonObject& args, JsonObject& fields );
    SceneEngine::DateEventBasePtr createDateEvent( const JsonObject& whenBlock,
                                                   SceneEngine::EventContainer& eventContainer );
    SceneEngine::DateEventBasePtr createPeriodicEvent( const JsonObject& whenBlock,
                                                       SceneEngine::EventContainer& eventContainer,
                                                       SceneEngine::DaysOfPeriodEvent::Period period );
    SceneEngine::DateEventPtr createOnceEvent( const JsonObject& whenBlock );
    SceneEngine::DateRangeEventPtr createDateRangeEvent( const JsonObject& whenBlock );
    Range< Json > createRange( JsonObject json, const std::string& comparator );

    bool isIntersection( std::multimap< std::string, Range< Json > >& compareNumbersRanges,
                         const std::string& itemId,
                         const JsonObject& itemValue,
                         const std::string& comparator );

    Json getValueFromFields( const std::string& key, const JsonObject& whenBlock );
    Json getOptionalValueFromFields( const std::string& key, const JsonObject& whenBlock );
    JsonObject::iterator getFields( JsonObject& whenBlock );
    JsonObject::iterator getField( const std::string& name, JsonArray& fields );
    bool checkItemParam( const std::string& name, JsonArray& fields );
    JsonObject::const_iterator getFields( const JsonObject& whenBlock );
    JsonObject::const_iterator getField( const std::string& name, const JsonArray& fields );
    JsonObject getItemValue( JsonArray& fields );
    JsonObject getItemScale( JsonArray& fields );
    std::string getString( const std::string& fieldName, JsonArray& fields );
    std::string getOptionalString( const std::string& fieldName, JsonArray& fields );
    ControllerId getControllerId( const Json& whenBlock );

    Time parseTime( const std::string& time );
    SceneEngine::DateEvent::TimeList parseTimeList( const JsonArray& timeList );

    void addEventId( Json& whenBlock ) const;

    void handleEvent( SceneEngine::EventBasePtr eventPtr,
                      SceneEngine::EventContainer& eventContainer,
                      const Json& whenBlock );
    void handleOperator( SceneEngine::AbstractOperator::SharedOperator operatorPtr,
                         SceneEngine::EventContainer& eventContainer,
                         const Json& whenBlock );
    static bool isWhenBlockEnabled( const Json& whenBlock );
    static std::string getBlockID( const Json& whenBlock );
    static std::string getBlockName( const Json& whenBlock );

    bool checkCompareNumberRangeParameters( std::string& itemId,
                                            SceneEngine::ItemValueWrapper& startValue,
                                            SceneEngine::ItemValueWrapper& endValue ) const;
    static bool isExpression( const std::string& type );
    static JsonObject createItemValueWrapperPayload( const Json& value,
                                                     const std::string& type,
                                                     const std::string& scale );

private:
    ControllerId m_defaultControllerId;
    GetGeoPositionCallback m_getGeoPosition;
    MakeGenericValueProvider m_genericValueProvider;
    Services::TaskLoop& m_taskLoop;
    ClockGetter m_clockGetter;

    IdUtils::PlaceholdersMapping* m_placeholdersMapping = nullptr;

    friend class UT::EventParserTest;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
