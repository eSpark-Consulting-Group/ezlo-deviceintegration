#include "scenes_manager/scenes_processor/parsers/event_parser.h"
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "base_utils/ezlodb.h"
#include "json_utils/json_utils.h"
#include "tiny/tiny_utils.h"
#include "common_utils/cpp14.h"
#include "base_utils/semantic_version.h"

#include "scenes_manager/scenes_engine/events/date_event_factory.h"
#include "scenes_manager/scenes_engine/events/monthly_event.h"
#include "scenes_manager/scenes_engine/events/days_of_period_event.h"
#include "scenes_manager/scenes_engine/events/weeks_of_period_event.h"
#include "scenes_manager/scenes_engine/events/item_event_base_factory.h"
#include "scenes_manager/scenes_engine/events/event_exception.h"
#include "scenes_manager/scenes_engine/operators/scene_operator.h"
#include "scenes_manager/scenes_engine/operators/operator_factory.h"
#include "scenes_manager/scenes_engine/helpers/convert_comparator.h"
#include "scenes_manager/scenes_engine/helpers/clock_interface.h"
#include "scenes_manager/scenes_engine/events/house_mode_alarm_phase_range_state_event.h"
#include "scenes_manager/scenes_engine/events/house_mode_switch_to_range_state_event.h"
#include "cored/model/values/value_type_traits.h"
#include "cored/model/exceptions.h"
#include "scenes_manager/scenes_engine/operators/visitors/operator_name_visitor.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "scenes_manager/scenes_engine/events/is_device_item_group_event.h"
#include "scenes_manager/scenes_engine/events/feature_state_event.h"
#include <utility>

namespace {

template < class T >
std::set< T > jsonNumbersArrayToSet( const JsonArray& jsonNumbers ) {
    std::set< T > arr;
    for( JsonArray::const_iterator it = jsonNumbers.cbegin(); it != jsonNumbers.cend(); ++it ) {
        if( !it->is_number() )
            return {};  // not throwing instead of this return in order to avoid breaking legacy date scenes

        arr.emplace( T( *it ) );
    }
    return arr;
}

}  // anonymous namespace

namespace HubPlatform {
namespace ScenesManager {

namespace EventFields {
const char* const WhenBlock = "when";
const char* const ExceptionsBlock = "exceptions";
const char* const ControllerId = "controllerId";
const char* const BlockOptions = "blockOptions";
const char* const Method = "method";
const char* const Name = "name";
const char* const SceneId = "id";
const char* const Arguments = "args";
const char* const Id = "_id";
const char* const ActionGroup = "actionGroup";
const char* const BlockName = "blockName";
const char* const Function = "function";
const char* const BlockType = "blockType";
const char* const GlobalRestriction = "globalRestriction";

namespace Fields {
const char* const Fields = "fields";
const char* const Item = "item";
const char* const Expression = "expression";
const char* const DeviceGroup = "deviceGroup";
const char* const DeviceGroupType = "device_group";
const char* const ItemGroup = "itemGroup";
const char* const ItemGroupType = "item_group";

const char* const Value = "value";
const char* const Operation = "operation";
const char* const Blocks = "blocks";
const char* const StartValue = "startValue";
const char* const EndValue = "endValue";
const char* const Comparator = "comparator";
const char* const Interval = "interval";
const char* const ValueType = "valueType";
}  // namespace Fields

namespace Values {
const char* const Expression = "expression";
const char* const Item = "item";
const char* const Any = "any";
}  // namespace Values
}  // namespace EventFields
using namespace SceneEngine::Model::Resources;

const std::set< std::string > HouseModeAlarmPhase = { "idle", "alarmed.entryDelay", "alarmed.main" };

EventParser::EventParser( ControllerId controllerId,
                          GetGeoPositionCallback getGeoPosition,
                          ClockGetter clockGetter,
                          MakeGenericValueProvider makeGenericValueProvider,
                          Services::TaskLoop& taskLoop )
: m_defaultControllerId( std::move( controllerId ) )
, m_getGeoPosition( std::move( getGeoPosition ) )
, m_genericValueProvider( std::move( makeGenericValueProvider ) )
, m_taskLoop( taskLoop )
, m_clockGetter( std::move( clockGetter ) ) {
    if( m_clockGetter == nullptr )
        throw std::invalid_argument( "EventParser ctor: clock getter is null" );
    if( m_getGeoPosition == nullptr )
        throw std::invalid_argument( "EventParser ctor: geo position getter is null" );
}

SceneEngine::EventContainer EventParser::parseTriggers( Json& sceneJsonObject,
                                                        IdUtils::PlaceholdersMapping& idsMapping ) {
    SceneEngine::EventContainer eventContainer;
    auto& triggersArray = getTriggerBlocks( sceneJsonObject );

    m_placeholdersMapping = &idsMapping;
    for( JsonObject& triggerBlock : triggersArray )
        parse( triggerBlock, eventContainer, nullptr );
    m_placeholdersMapping = nullptr;

    return eventContainer;
}

SceneEngine::EventContainer EventParser::parseExceptions( Json& sceneJsonObject,
                                                          IdUtils::PlaceholdersMapping& idsMapping ) {
    SceneEngine::EventContainer eventContainer;

    Json::iterator itExceptions = sceneJsonObject.find( EventFields::ExceptionsBlock );
    if( itExceptions != sceneJsonObject.end() ) {
        if( !itExceptions->is_array() )
            throw SceneEngine::SceneException(
                Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::ExceptionsBlock, "array expected" );

        m_placeholdersMapping = &idsMapping;
        for( Json& exceptionBlock : *itExceptions ) {
            if( json_get_str( exceptionBlock, EventFields::BlockType ) != EventFields::GlobalRestriction ) {
                parse( exceptionBlock, eventContainer, nullptr );
            }
        }
        m_placeholdersMapping = nullptr;
    }

    return eventContainer;
}

SceneEngine::EventContainer EventParser::parseGlobalRestriction( SceneEngine::SceneSharedPtr scenePtr,
                                                                 Json& sceneJsonObject,
                                                                 IdUtils::PlaceholdersMapping& idsMapping ) {
    SceneEngine::EventContainer eventContainer;

    Json::iterator itExceptions = sceneJsonObject.find( EventFields::ExceptionsBlock );
    int globalRestrictionBlockCounter = 0;
    if( itExceptions != sceneJsonObject.end() ) {
        if( !itExceptions->is_array() )
            throw SceneEngine::SceneException(
                Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::ExceptionsBlock, "array expected" );

        m_placeholdersMapping = &idsMapping;
        for( Json& exceptionBlock : *itExceptions ) {
            if( json_get_str( exceptionBlock, EventFields::BlockType ) == EventFields::GlobalRestriction ) {
                globalRestrictionBlockCounter++;
                if( globalRestrictionBlockCounter > 1 ) {
                    throw SceneEngine::SceneException( Errors::SCENE_GLOBAL_RESTRICTION_MULTIPLE_BLOCKS,
                                                       "Only a single global restriction block is allowed" );
                }
                parse( exceptionBlock, eventContainer, nullptr );
                scenePtr->setGlobalRestrictionId( json_get_str( exceptionBlock, EventFields::Id ) );
            }
        }
        m_placeholdersMapping = nullptr;
    }

    return eventContainer;
}

void EventParser::setClockGetter( ClockGetter clockGetter ) {
    if( clockGetter == nullptr )
        throw std::invalid_argument( "EventParser: clock getter shouldn't be null" );
    m_clockGetter = std::move( clockGetter );
}

bool parseSourceFields( std::pair< std::string, SceneEngine::ValueSourceKey::SourceType >& sourceInfo,
                        const JsonObject& value,
                        std::string& comparator,
                        const std::string& name,
                        const std::string& type,
                        int& sourcesCount ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    if( name == EventFields::Fields::Item && value.is_string() ) {
        if( type != EventFields::Fields::Item )
            throw SceneException( Errors::SCENE_WHEN_SOURCE_TYPE_MISMATCH, "Invalid source type, item expected" );
        if( sourcesCount++ > 0 )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );
        sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
    }
    else if( name == EventFields::Fields::Expression && value.is_string() ) {
        if( type != EventFields::Fields::Expression )
            throw SceneException( Errors::SCENE_WHEN_SOURCE_TYPE_MISMATCH, "Invalid source type, expression expected" );
        if( sourcesCount++ > 0 )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );
        sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
    }
    else if( name == EventFields::Fields::DeviceGroup && value.is_string() ) {
        if( type != EventFields::Fields::DeviceGroupType )
            throw SceneException( Errors::SCENE_WHEN_SOURCE_TYPE_MISMATCH,
                                  "Invalid source type, deviceGroup expected" );

        auto sourceId = value.get< std::string >();
        if( sourcesCount == 0 ) {
            sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( sourceId, "" ),
                                         ValueSourceKey::SourceType::DeviceItemGroups );
            ++sourcesCount;
            return false;
        }

        if( sourcesCount > 1 || sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );

        auto unpackedIds = ValueSourceKey::unpackDeviceItemGroupIds( sourceInfo.first );
        auto& itemGroupId = unpackedIds.second;
        if( itemGroupId.empty() )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );

        sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( sourceId, itemGroupId ),
                                     ValueSourceKey::SourceType::DeviceItemGroups );
        ++sourcesCount;
    }
    else if( name == EventFields::Fields::ItemGroup && value.is_string() ) {
        if( type != EventFields::Fields::ItemGroupType )
            throw SceneException( Errors::SCENE_WHEN_SOURCE_TYPE_MISMATCH, "Invalid source type, itemGroup expected" );

        auto sourceId = value.get< std::string >();
        if( sourcesCount == 0 ) {
            sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( "", sourceId ),
                                         ValueSourceKey::SourceType::DeviceItemGroups );
            ++sourcesCount;
            return false;
        }

        if( sourcesCount > 1 || sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );

        auto unpackedIds = ValueSourceKey::unpackDeviceItemGroupIds( sourceInfo.first );
        auto& deviceGroupId = unpackedIds.first;
        if( deviceGroupId.empty() )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );

        sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( deviceGroupId, sourceId ),
                                     ValueSourceKey::SourceType::DeviceItemGroups );
        ++sourcesCount;
    }
    else if( name == EventFields::Fields::Comparator && value.is_string() ) {
        comparator = value.get< std::string >();
    }

    return true;
}

void EventParser::parseCompareNumbersEvent( Json& whenBlock,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::pair< std::string, ValueSourceKey::SourceType > valueInfo;
    std::string comparator;

    ItemValueWrapper itemValue;
    bool isExternalValue = false;

    auto fieldsEntryIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "compareNumbers: when block should have an array with fields required for the method" );
    }
    int sourcesCount = 0;
    forEachField(
        *fieldsEntryIt,
        [&sourcesCount, &sourceInfo, &valueInfo, &comparator, &itemValue, &isExternalValue](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) {
            if( name == EventFields::Fields::Value ) {
                LOGGER_TRACE_RAW( "type: " + type );
                if( type == EventFields::Values::Expression ) {
                    // this is an expression, value == expressionName
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
                }
                else if( type == EventFields::Values::Item ) {
                    // this is an item, value == itemId
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
                }
                else {
                    // this is an ordinary value
                    itemValue.setPayload( createItemValueWrapperPayload( value, type, scale ) );
                }
            }

            else {
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
        } );

    if( sourceInfo.first.empty() || comparator.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }

    ItemValueComparator compareFunction = convertComparator( comparator, true );
    if( isExternalValue && valueInfo.first.empty() ) {
        std::string message = "compareNumbers: invalid value id: " + valueInfo.first;
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    else if( !isExternalValue && !itemValue.isValid() ) {
        std::string message = "compareNumbers: cannot create valid item value: " + itemValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    bool keepState = sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups;

    // make value provider
    ItemEventBasePtr compareNumbersEvent;
    if( isExternalValue ) {
        ValueSourceKey valueKey( getControllerId( whenBlock ), valueInfo.first, valueInfo.second );
        if( keepState ) {
            compareNumbersEvent = ItemEventBaseFactory::makeCompareNumbers(
                m_genericValueProvider( std::move( valueKey ) ), compareFunction );
        }
        else {
            compareNumbersEvent = ItemEventBaseFactory::makeCompareNumbersStateless(
                m_genericValueProvider( std::move( valueKey ) ), compareFunction );
        }
    }
    else {
        if( keepState ) {
            compareNumbersEvent = ItemEventBaseFactory::makeCompareNumbers( itemValue, compareFunction );
        }
        else {
            compareNumbersEvent = ItemEventBaseFactory::makeCompareNumbersStateless( itemValue, compareFunction );
        }
    }

    addOperator( compareNumbersEvent, parentOperatorPtr );
    handleEvent( compareNumbersEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, compareNumbersEvent );
    }
    else {
        eventContainer.addCompareNumbersEvent( searchKey, compareNumbersEvent );
    }
}

void EventParser::parseCompareStringsEvent( Json& whenBlock,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::pair< std::string, ValueSourceKey::SourceType > valueInfo;
    std::string comparator;

    ItemValueWrapper itemValue;
    bool isExternalValue = false;

    auto fieldsEntryIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "compareStrings: when block should have an array with fields required for the method" );
    }
    int sourcesCount = 0;
    forEachField(
        *fieldsEntryIt,
        [&sourcesCount, &sourceInfo, &valueInfo, &comparator, &itemValue, &isExternalValue](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) {
            if( name == EventFields::Fields::Value ) {
                if( type == EventFields::Values::Expression ) {
                    // this is an expression, value == expressionName
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
                }
                else if( type == EventFields::Values::Item ) {
                    // this is an item, value == itemId
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
                }
                else {
                    // this is an ordinary value
                    itemValue.setPayload( createItemValueWrapperPayload( value, type, "" ) );
                }
                LOGGER_TRACE_RAW( "type: " + type );
            }
            else {
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
        } );
    if( sourceInfo.first.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "compareStrings: empty id of an item" );
    if( comparator.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "compareStrings: 'comparator' field is empty" );
    if( isExternalValue && valueInfo.first.empty() ) {
        std::string message = "compareStrings: invalid value id: " + valueInfo.first;
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    else if( !isExternalValue && !itemValue.isValid() ) {
        std::string message = "compareStrings: cannot create valid item value: " + itemValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }

    ItemValueComparator compareFunction = convertComparator( comparator, true );

    const bool keepState = sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups;

    // make value provider
    ItemEventBasePtr compareStringsEvent;
    if( isExternalValue ) {
        ValueSourceKey valueKey( getControllerId( whenBlock ), valueInfo.first, valueInfo.second );
        if( keepState ) {
            compareStringsEvent = ItemEventBaseFactory::makeCompareStrings(
                m_genericValueProvider( std::move( valueKey ) ), compareFunction );
        }
        else {
            compareStringsEvent = ItemEventBaseFactory::makeCompareStringsStateless(
                m_genericValueProvider( std::move( valueKey ) ), compareFunction );
        }
    }
    else {
        if( keepState ) {
            compareStringsEvent = ItemEventBaseFactory::makeCompareStrings( itemValue, compareFunction );
        }
        else {
            compareStringsEvent = ItemEventBaseFactory::makeCompareStringsStateless( itemValue, compareFunction );
        }
    }

    addOperator( compareStringsEvent, std::move( parentOperatorPtr ) );
    handleEvent( compareStringsEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, compareStringsEvent );
    }
    else {
        eventContainer.addCompareStringsEvent( searchKey, compareStringsEvent );
    }
}

void EventParser::parseCompareNumberRangeEvent( Json& whenBlock,
                                                SceneEngine::EventContainer& eventContainer,
                                                SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;
    auto fieldsEntryIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            "compareNumberRange: when block should have an array with fields required for the method" );
    }

    int sourcesCount = 0;
    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::string comparator;
    ItemValueWrapper startValue, endValue;
    forEachField(
        *fieldsEntryIt,
        [&sourcesCount, &sourceInfo, &comparator, &startValue, &endValue](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) {
            if( name == EventFields::Fields::Item && value.is_string() ) {
                if( type != EventFields::Fields::Item )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, item expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
            }
            else if( name == EventFields::Fields::Expression && value.is_string() ) {
                if( type != EventFields::Fields::Expression )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, expression expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
            }
            else if( name == EventFields::Fields::DeviceGroup && value.is_string() ) {
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::ItemGroup && value.is_string() ) {
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::Comparator && value.is_string() ) {
                comparator = value.get< std::string >();
            }
            else if( name == EventFields::Fields::StartValue && value.is_number() ) {
                startValue.setPayload( createItemValueWrapperPayload( value, type, scale ) );
            }
            else if( name == EventFields::Fields::EndValue && value.is_number() ) {
                endValue.setPayload( createItemValueWrapperPayload( value, type, scale ) );
            }
        } );

    if( !checkCompareNumberRangeParameters( sourceInfo.first, startValue, endValue ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    if( !startValue.isValid() ) {
        std::string message =
            "compareNumberRange: cannot create valid start value: " + startValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    if( !endValue.isValid() ) {
        std::string message = "compareNumberRange: cannot create valid end value: " + endValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }

    auto compareNumberRangeEvent =
        SceneEngine::ItemEventBaseFactory::makeCompareNumberRange( startValue, endValue, comparator );

    addOperator( compareNumberRangeEvent, parentOperatorPtr );
    handleEvent( compareNumberRangeEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, std::move( compareNumberRangeEvent ) );
    }
    else {
        eventContainer.addCompareNumbersRangeEvent( searchKey, std::move( compareNumberRangeEvent ) );
    }
}

void EventParser::parseStringOperationEvent( Json& whenBlock,
                                             SceneEngine::EventContainer& eventContainer,
                                             SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    auto fieldsIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsIt == whenBlock.end() || !JsonTypes::isArray( *fieldsIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "stringOperation: when block should have an array with fields required for the method" );
    }

    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::pair< std::string, ValueSourceKey::SourceType > valueInfo;
    ItemValueWrapper itemValue;
    std::string operation;
    bool isExternalValue = false;
    int sourcesCount = 0;
    forEachField(
        *fieldsIt,
        [&sourcesCount, &sourceInfo, &valueInfo, &operation, &itemValue, &isExternalValue](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& /*scale*/ ) {
            if( name == EventFields::Fields::Item && value.is_string() ) {
                if( type != EventFields::Fields::Item )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, item expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
            }
            else if( name == EventFields::Fields::Expression && value.is_string() ) {
                if( type != EventFields::Fields::Expression )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, expression expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
            }
            else if( name == EventFields::Fields::DeviceGroup && value.is_string() ) {
                std::string comparator;
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::ItemGroup && value.is_string() ) {
                std::string comparator;
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::Operation && value.is_string() ) {
                operation = value.get< std::string >();
            }
            else if( name == EventFields::Fields::Value ) {
                LOGGER_TRACE_RAW( "type: " + type );
                if( type == EventFields::Values::Expression ) {
                    // this is an expression, value == expressionName
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
                }
                else if( type == EventFields::Values::Item ) {
                    // this is an item, value == itemId
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
                }
                else {
                    // this is an ordinary value
                    itemValue.setPayload( createItemValueWrapperPayload( value, type, "" ) );
                }
            }
        } );

    if( sourceInfo.first.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "stringOperation: empty id of a value source" );
    if( operation.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "stringOperation: 'operation' field is empty" );
    if( isExternalValue && valueInfo.first.empty() ) {
        std::string message = "stringOperation: invalid value id: " + valueInfo.first;
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    else if( !isExternalValue && ( itemValue.empty() || !itemValue.isValid() ) ) {
        std::string message = "stringOperation: cannot create valid item value: " + itemValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    bool keepState = sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups;

    ItemEventBasePtr stringOperationEvent;
    if( isExternalValue ) {
        ValueSourceKey valueKey( getControllerId( whenBlock ), valueInfo.first, valueInfo.second );
        if( keepState ) {
            stringOperationEvent =
                ItemEventBaseFactory::makeStringOperation( m_genericValueProvider( std::move( valueKey ) ), operation );
        }
        else {
            stringOperationEvent = ItemEventBaseFactory::makeStringOperationStateless(
                m_genericValueProvider( std::move( valueKey ) ), operation );
        }
    }
    else {
        if( keepState ) {
            stringOperationEvent = ItemEventBaseFactory::makeStringOperation( itemValue, operation );
        }
        else {
            stringOperationEvent = ItemEventBaseFactory::makeStringOperationStateless( itemValue, operation );
        }
    }

    addOperator( stringOperationEvent, std::move( parentOperatorPtr ) );
    handleEvent( stringOperationEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, std::move( stringOperationEvent ) );
    }
    else {
        eventContainer.addStringOperationEvent( searchKey, std::move( stringOperationEvent ) );
    }
}

void EventParser::parseInArrayEvent( Json& whenBlock,
                                     SceneEngine::EventContainer& eventContainer,
                                     SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    auto fieldsIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsIt == whenBlock.end() || !JsonTypes::isArray( *fieldsIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "inArray: when block should have an array with fields required for the method" );
    }

    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::string operation;
    ItemValueWrapper itemValue;
    int sourcesCount = 0;
    forEachField(
        *fieldsIt,
        [&sourcesCount, &sourceInfo, &operation, &itemValue](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& /*scale*/ ) {
            if( name == EventFields::Fields::Item && value.is_string() ) {
                if( type != EventFields::Fields::Item )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, item expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
            }
            else if( name == EventFields::Fields::Expression && value.is_string() ) {
                if( type != EventFields::Fields::Expression )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, expression expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
            }
            else if( name == EventFields::Fields::DeviceGroup && value.is_string() ) {
                std::string comparator;
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::ItemGroup && value.is_string() ) {
                std::string comparator;
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::Operation && value.is_string() ) {
                operation = value.get< std::string >();
            }
            else if( name == EventFields::Fields::Value ) {
                itemValue.setPayload( createItemValueWrapperPayload( value, type, "" ) );
            }
        } );

    if( sourceInfo.first.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "inArray: empty source value key" );
    if( itemValue.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "inArray: 'value' field is empty" );
    if( !itemValue.isValid() ) {
        const std::string message = "inArray: cannot create valid value: " + itemValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    if( !itemValue.isArray() ) {
        const std::string message = "inArray: value should be an array, actual type: " + itemValue.getValueType();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }

    bool keepState = sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups;

    ItemEventBasePtr inArrayEvent;
    try {
        std::vector< ItemValueWrapper > arrayValues = itemValue.parseArrayValues();
        if( keepState ) {
            inArrayEvent = ItemEventBaseFactory::makeInArray( std::move( arrayValues ), operation );
        }
        else {
            inArrayEvent = ItemEventBaseFactory::makeInArrayStateless( std::move( arrayValues ), operation );
        }
    }
    catch( const EventException& ex ) {
        const std::string message = std::string( "inArray: creation of an event failed: " ) + ex.what();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }

    addOperator( inArrayEvent, std::move( parentOperatorPtr ) );
    handleEvent( inArrayEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, std::move( inArrayEvent ) );
    }
    else {
        eventContainer.addInArrayEvent( searchKey, std::move( inArrayEvent ) );
    }
}

void EventParser::parseIsItemStateChangedEvent( Json& whenBlock,
                                                SceneEngine::EventContainer& eventContainer,
                                                SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    auto fieldsIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsIt == whenBlock.end() || !JsonTypes::isArray( *fieldsIt ) ) {
        throw SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            "isItemStateChanged: when block should have an array with fields required for the method" );
    }

    // parse
    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    ItemValueWrapper startValue;
    ItemValueWrapper finishValue;
    bool startValueIsExpression = false;
    bool finishValueIsExpression = false;

    auto parseValue = []( const JsonObject& value,
                          const std::string& type,
                          const std::string& scale ) -> std::pair< ItemValueWrapper, bool /* isExpression */ > {
        ItemValueWrapper result;
        bool isExpression = false;

        const bool useAny = value.is_string() && value.get< std::string >() == EventFields::Values::Any;
        if( !useAny ) {
            if( EventParser::isExpression( type ) ) {
                isExpression = true;
                result.setPayload( value );
            }
            else {
                result.setPayload( createItemValueWrapperPayload( value, type, scale ) );
            }
        }

        return std::make_pair( std::move( result ), isExpression );
    };
    forEachField(
        *fieldsIt,
        [&]( const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) {
            if( name == EventFields::Fields::Item && value.is_string() ) {
                if( type != EventFields::Fields::Item )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, item expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
            }
            else if( name == EventFields::Fields::Expression && value.is_string() ) {
                if( type != EventFields::Fields::Expression )
                    throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid source type, expression expected" );
                sourceInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
            }
            else if( name == Model::Resources::EventFields::Start ) {
                std::tie( startValue, startValueIsExpression ) = parseValue( value, type, scale );
            }
            else if( name == Model::Resources::EventFields::Finish ) {
                std::tie( finishValue, finishValueIsExpression ) = parseValue( value, type, scale );
            }
        } );

    // validation
    if( sourceInfo.first.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "isItemStateChanged: empty id of a value source" );
    if( !startValue.empty() && !startValue.isValid() ) {
        std::string paramName = Model::Resources::EventFields::Start;
        std::string info = ": cannot create valid '" + paramName + "' value: " + startValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, MethodNames::isItemStateChanged + info );
    }
    if( !finishValue.empty() && !finishValue.isValid() ) {
        std::string paramName = Model::Resources::EventFields::Finish;
        std::string info = ": cannot create valid '" + paramName + "' value: " + finishValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, MethodNames::isItemStateChanged + info );
    }

    // creation of the event
    auto fillValueInfo = [this, &whenBlock]( ItemValueWrapper itemValue, bool isExpression ) -> ItemValueProviderPtr {
        if( isExpression ) {
            std::string expressionName = json_get_str( itemValue.getValue() );
            LOGGER_TRACE_RAW( "parseIsItemStateChangedEvent: Expression name: " + expressionName );
            ValueSourceKey valueKey(
                getControllerId( whenBlock ), expressionName, ValueSourceKey::SourceType::Expression );
            return m_genericValueProvider( std::move( valueKey ) );
        }
        else if( !itemValue.empty() ) {
            return makeItemValueProvider( std::move( itemValue ) );
        }
        return nullptr;
    };
    auto startValueProvider = fillValueInfo( std::move( startValue ), startValueIsExpression );
    auto finishValueProvider = fillValueInfo( std::move( finishValue ), finishValueIsExpression );
    auto event =
        std::make_shared< ItemStateChangedEvent >( std::move( startValueProvider ), std::move( finishValueProvider ) );

    addOperator( event, std::move( parentOperatorPtr ) );
    handleEvent( event, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    eventContainer.addItemStateChangedEvent( searchKey, std::move( event ) );
}

void EventParser::parseIsDeviceItemGroupEvent( Json& whenBlock,
                                               SceneEngine::EventContainer& eventContainer,
                                               SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;

    auto fieldsIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsIt == whenBlock.end() || !JsonTypes::isArray( *fieldsIt ) ) {
        throw SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            "isDeviceItemGroupEvent: when block should have an array with fields required for the method" );
    }

    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    int sourcesCount = 0;
    forEachField(
        *fieldsIt,
        [&sourcesCount, &sourceInfo](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& /*scale*/ ) {
            if( name == EventFields::Fields::DeviceGroup && value.is_string() ) {
                std::string comparator;
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
            else if( name == EventFields::Fields::ItemGroup && value.is_string() ) {
                std::string comparator;
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
        } );

    if( sourceInfo.first.empty() )
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "isDeviceItemGroupEvent: empty source value key" );

    ItemEventBasePtr isDeviceItemGroup;
    isDeviceItemGroup = std::make_shared< IsDeviceItemGroupEvent >();

    addOperator( isDeviceItemGroup, std::move( parentOperatorPtr ) );
    handleEvent( isDeviceItemGroup, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, std::move( isDeviceItemGroup ) );
    }
}

void EventParser::parseCompareValuesEvent( Json& whenBlock,
                                           SceneEngine::EventContainer& eventContainer,
                                           SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;
    auto formatError = []( const std::string& errorInfo ) -> std::string {
        std::string message = std::string{ MethodNames::compareValues } + ": " + errorInfo;
        return message;
    };

    auto fieldsEntryIt = whenBlock.find( EventFields::Fields::Fields );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              formatError( "'when' block should have an array with fields required for the method" ) );
    }
    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::pair< std::string, ValueSourceKey::SourceType > valueInfo;
    std::string comparator, valueType;
    ItemValueWrapper itemValue;
    int sourcesCount = 0;

    bool isExternalValue = false;

    forEachField(
        *fieldsEntryIt,
        [&sourcesCount, &sourceInfo, &valueInfo, &comparator, &itemValue, &isExternalValue, &valueType](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) {
            if( name == EventFields::Fields::ValueType && value.is_string() ) {
                valueType = json_get_str( value );
            }
            else if( name == EventFields::Fields::Value ) {
                if( type == EventFields::Values::Expression ) {
                    // this is an expression, value == expressionName
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Expression );
                }
                else if( type == EventFields::Values::Item ) {
                    // this is an item, value == itemId
                    isExternalValue = true;
                    valueInfo = std::make_pair( value.get< std::string >(), ValueSourceKey::SourceType::Item );
                }
                else {
                    // this is an ordinary value
                    itemValue.setPayload( createItemValueWrapperPayload( value, type, scale ) );
                }
            }
            else {
                if( !parseSourceFields( sourceInfo, value, comparator, name, type, sourcesCount ) ) {
                    return;
                }
            }
        } );
    if( sourceInfo.first.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, formatError( "Empty value source id" ) );
    }

    if( isExternalValue && valueInfo.first.empty() ) {
        std::string message = "compareValues: invalid value id: " + valueInfo.first;
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }
    else if( !isExternalValue && !itemValue.isValid() ) {
        std::string message = "compareValues: cannot create valid item value: " + itemValue.getInvalidityReason();
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }

    if( comparator.empty() ) {
        comparator = ValueComparators::Equal;
    }

    if( valueType.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, formatError( "valueType field is mandatory" ) );
    }

    bool hasIsLess = true;
    if( !valueType.empty() ) {
        auto valueTypeEnum = CoreD::Model::ValueType::fromString( valueType );
        if( !CoreD::Model::isValueTypeValid( valueTypeEnum ) ) {
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                               formatError( " unexpected valueType: " + valueType ) );
        }
        hasIsLess = CoreD::Model::hasIsLess( valueTypeEnum );
    }
    ItemValueComparator compareFunction;

    try {
        compareFunction = SceneEngine::convertComparator( comparator, hasIsLess );
    }
    catch( const ConvertComparatorException& ex ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                           formatError( std::string{ "comparator error: " } + ex.what() ) );
    }

    const bool keepState = sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups;

    ItemEventBasePtr event;
    if( isExternalValue ) {
        ValueSourceKey valueKey( getControllerId( whenBlock ), valueInfo.first, valueInfo.second );

        if( keepState ) {
            event = ItemEventBaseFactory::makeCompareValues(
                m_genericValueProvider( std::move( valueKey ) ), comparator, valueType );
        }
        else {
            event = ItemEventBaseFactory::makeCompareValuesStateless(
                m_genericValueProvider( std::move( valueKey ) ), comparator, valueType );
        }
    }
    else {
        if( itemValue.getValueType() != valueType ) {
            throw SceneEngine::SceneException(
                Errors::SCENE_WHEN_TYPE_MISMATCH,
                formatError( "event value type restricted to " + valueType +
                             ", but left value has different value type: " + itemValue.getValueType() ) );
        }

        // if valueType is present: comparator has already been checked
        event = ItemEventBaseFactory::makeCompareValues( itemValue, comparator, valueType );

        if( keepState ) {
            event = ItemEventBaseFactory::makeCompareValues( itemValue, comparator, valueType );
        }
        else {
            event = ItemEventBaseFactory::makeCompareValuesStateless( itemValue, comparator, valueType );
        }
    }

    addOperator( event, std::move( parentOperatorPtr ) );
    handleEvent( event, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );

    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addDeviceItemGroupsEvent( searchKey, event );
    }
    else {
        eventContainer.addCompareValuesEvent( searchKey, event );
    }
}

bool EventParser::isExpression( const std::string& type ) {
    return type == EventFields::Values::Expression;
}

void EventParser::parseIsItemStateCondition( JsonObject& whenBlock,
                                             SceneEngine::EventContainer& eventContainer,
                                             SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    using namespace SceneEngine::Utils;
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            "isItemState: when block should have an array with fields required for the method" );
    }
    JsonArray& fields = *fieldsEntryIt;
    std::string itemId;

    int sourcesCount = 0;
    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::pair< std::string, ValueSourceKey::SourceType > valueInfo;

    bool ret = findField( "item", fields, [&itemId, &sourcesCount]( JsonObject& entry ) -> void {
        if( sourcesCount++ > 1 )
            throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED, "Only one value source is allowed" );
        entryProcessor( EventFields::Fields::Value,
                        entry,
                        [&itemId]( const JsonObject& entry ) -> void { itemId = entry.get< std::string >(); },
                        JsonTypes::isString,
                        []( const JsonObject& /*obj*/, const std::string& /*name*/ ) {
                            // value with the item id is not set
                            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
                        } );
    } );

    ret |= findField(
        EventFields::Fields::ItemGroup, fields, [&itemId, &sourcesCount, &sourceInfo]( JsonObject& entry ) -> void {
            if( sourcesCount > 1 )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed 2" );

            entryProcessor( EventFields::Fields::Value,
                            entry,
                            [&itemId]( const JsonObject& entry ) -> void { itemId = entry.get< std::string >(); },
                            JsonTypes::isString,
                            []( const JsonObject&, const std::string& ) {
                                // value with the item id is not set
                                throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
                            } );

            if( sourcesCount == 0 ) {
                sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( "", itemId ),
                                             ValueSourceKey::SourceType::DeviceItemGroups );
                ++sourcesCount;
                return;
            }

            if( sourcesCount > 1 || sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed 3" );

            auto unpackedIds = ValueSourceKey::unpackDeviceItemGroupIds( sourceInfo.first );
            auto& deviceGroupId = unpackedIds.first;
            if( deviceGroupId.empty() )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed 4" );

            sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( deviceGroupId, itemId ),
                                         ValueSourceKey::SourceType::DeviceItemGroups );
            ++sourcesCount;
        } );

    bool isDeviceGroup = false;
    ret |= findField(
        EventFields::Fields::DeviceGroup, fields, [&itemId, &sourcesCount, &sourceInfo]( JsonObject& entry ) -> void {
            if( sourcesCount > 1 )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed 5" );

            entryProcessor( EventFields::Fields::Value,
                            entry,
                            [&itemId]( const JsonObject& entry ) -> void { itemId = entry.get< std::string >(); },
                            JsonTypes::isString,
                            []( const JsonObject&, const std::string& ) {
                                // value with the item id is not set
                                throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
                            } );

            if( sourcesCount == 0 ) {
                sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( itemId, "" ),
                                             ValueSourceKey::SourceType::DeviceItemGroups );
                ++sourcesCount;
                return;
            }

            if( sourcesCount > 1 || sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed" );

            auto unpackedIds = ValueSourceKey::unpackDeviceItemGroupIds( sourceInfo.first );
            auto& itemGroupId = unpackedIds.second;
            if( itemGroupId.empty() )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed" );
            sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( itemId, itemGroupId ),
                                         ValueSourceKey::SourceType::DeviceItemGroups );
            ++sourcesCount;
        } );

    if( !ret ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    JsonObject itemValue;
    std::string itemScale, itemType;
    ret = findField( "value", fields, [&itemValue, &itemScale, &itemType]( JsonObject& entry ) -> void {
        entryProcessor( EventFields::Fields::Value,
                        entry,
                        [&itemValue]( const JsonObject& entry ) -> void {
                            LOGGER_TRACE_RAW( "entryProcessor for field value : " + cc::j( entry ) );
                            itemValue = entry;
                        },
                        []( const JsonObject& /*obj*/, const std::string& /*name*/ ) {
                            // value with the item id is not set
                            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
                        } );
        entryProcessor( "scale",
                        entry,
                        [&itemScale]( const JsonObject& entry ) -> void { itemScale = json_get_str( entry ); },
                        []( const JsonObject& /*obj*/, const std::string& /*name*/ ) {
                            // scale field is optional
                        } );
        entryProcessor( "type",
                        entry,
                        [&itemType]( const JsonObject& entry ) -> void { itemType = json_get_str( entry ); },
                        []( const JsonObject& /*obj*/, const std::string& /*name*/ ) {
                            // type field is optional
                        } );
    } );
    if( !ret ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    // common field
    JsonObject armed = JSON_EMPTY_OBJ;
    ret = findField( "armed", fields, [&armed]( JsonObject& entry ) -> void {
        entryProcessor(
            EventFields::Fields::Value, entry, [&armed]( const JsonObject& entry ) -> void { armed = entry; } );
    } );

    bool isExpression = EventParser::isExpression( itemType );

    // make item event
    std::shared_ptr< ItemEvent > itemEvent;
    if( isExpression ) {
        if( m_genericValueProvider == nullptr )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "No generic value provider configured" );

        std::string expressionName = json_get_str( itemValue );
        LOGGER_TRACE( "Expression name: " + expressionName );
        if( expressionName.empty() )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "expression name" );

        ValueSourceKey valueKey{ getControllerId( whenBlock ), expressionName, ValueSourceKey::SourceType::Expression };

        if( !armed.empty() && armed.is_boolean() ) {
            itemEvent = std::make_shared< ItemEvent >(
                itemId, m_genericValueProvider( valueKey ), ItemEvent::toArmedState( armed.get< bool >() ) );
        }
        else {
            itemEvent = std::make_shared< ItemEvent >( itemId, m_genericValueProvider( valueKey ) );
        }
        itemEvent->setEventType( ItemEvent::EventType::EXPRESSION );
        itemEvent->setValueKey( valueKey );
    }
    else {
        auto payload = JSON( "type" << itemType );
        if( !itemScale.empty() )
            payload[ "value" ] = JSON( "value" << itemValue << "scale" << itemScale );
        else
            payload[ "value" ] = itemValue;

        ItemValueWrapper itemWrapper( std::move( payload ) );
        if( !itemWrapper.isValid() )
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid value" );

        if( !armed.empty() && armed.is_boolean() ) {
            itemEvent = std::make_shared< ItemEvent >( itemId,
                                                       makeItemValueProvider( std::move( itemWrapper ) ),
                                                       ItemEvent::toArmedState( armed.get< bool >() ) );
        }
        else {
            itemEvent = std::make_shared< ItemEvent >( itemId, makeItemValueProvider( std::move( itemWrapper ) ) );
        }
    }

    addOperator( itemEvent, parentOperatorPtr );
    handleEvent( itemEvent, eventContainer, whenBlock );

    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );

    if( searchKey.isDeviceItemGroups() ) {
        eventContainer.addItemStateDeviceItemGroupsEvent( searchKey, itemEvent );
    }
    else {
        ValueSourceKey searchKey( getControllerId( whenBlock ), itemId );
        eventContainer.addItemEvent( searchKey, itemEvent );
    }
}

void EventParser::parseIsButtonState( JsonObject& whenBlock,
                                      SceneEngine::EventContainer& eventContainer,
                                      SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "isButtonState: when block should have an array with fields required for the method" );
    }
    std::string itemId, buttonState;

    forEachField( *fieldsEntryIt,
                  [&itemId, &buttonState]( const std::string& name,
                                           const std::string& /*type*/,
                                           const JsonObject& value,
                                           const std::string& /*scale*/ ) {
                      if( name == "item" && value.is_string() ) {
                          itemId = value.get< std::string >();
                      }
                      else if( name == "value" && value.is_string() ) {
                          buttonState = value.get< std::string >();
                      }
                  } );
    if( itemId.empty() || buttonState.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto buttonStateEvent = SceneEngine::ItemEventBaseFactory::makeButtonState( buttonState );
    addOperator( buttonStateEvent, parentOperatorPtr );
    handleEvent( buttonStateEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), itemId );
    eventContainer.buttonStateEvents.push_back( { searchKey, buttonStateEvent } );
}

void EventParser::parseIsBatteryState( JsonObject& whenBlock,
                                       SceneEngine::EventContainer& eventContainer,
                                       SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;

    LOGGER_TRACE( "parseIsBatteryState" );
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "isBatteryState: when block should have an array with fields required for the method" );
    }

    std::string batteryState;

    forEachField( *fieldsEntryIt,
                  [&batteryState]( const std::string& name,
                                   const std::string& /*type*/,
                                   const JsonObject& value,
                                   const std::string& /*scale*/ ) {
                      if( name == "state" && value.is_string() ) {
                          batteryState = value.get< std::string >();
                      }
                  } );
    if( batteryState.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto batteryStateEvent = std::make_shared< BatteryStateEvent >( std::move( batteryState ) );
    addOperator( batteryStateEvent, parentOperatorPtr );
    handleEvent( batteryStateEvent, eventContainer, whenBlock );

    eventContainer.batteryStateEvents.push_back( batteryStateEvent );
}

void EventParser::parseIsFirmwareUpdateState( JsonObject& whenBlock,
                                              SceneEngine::EventContainer& eventContainer,
                                              SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;

    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            "isFirmwareUpdateState: when block should have array with fields required for the method" );
    }

    std::string state;

    forEachField( *fieldsEntryIt,
                  [&state]( const std::string& name,
                            const std::string& /*type*/,
                            const JsonObject& value,
                            const std::string& /*scale*/ ) {
                      if( name == "state" && value.is_string() ) {
                          state = value.get< std::string >();
                      }
                  } );
    if( state.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto firmwareUpdateStateEvent = std::make_shared< FirmwareUpdateStateEvent >( std::move( state ) );
    addOperator( firmwareUpdateStateEvent, parentOperatorPtr );
    handleEvent( firmwareUpdateStateEvent, eventContainer, whenBlock );

    eventContainer.firmwareUpdateStateEvents.push_back( firmwareUpdateStateEvent );
}

void EventParser::parseIsBatteryLevel( JsonObject& whenBlock,
                                       SceneEngine::EventContainer& eventContainer,
                                       SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;

    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "isBatteryLevel: when block should have an array with fields required for the method" );
    }

    std::string compareOperator;
    int level;

    ItemValueWrapper levelItem;

    forEachField(
        *fieldsEntryIt,
        [&compareOperator, &levelItem](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& /*scale*/ ) {
            if( name == "comparator" && value.is_string() ) {
                compareOperator = value.get< std::string >();
            }
            if( name == "value" && value.is_number_integer() ) {
                levelItem.setPayload( Json{
                    { "name", name }, { "type", type }, { "value", value.get< JsonObject::number_integer_t >() } } );
            }
        } );

    if( compareOperator.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }

    SceneEngine::ItemValueComparator compareFunction = convertComparator( compareOperator );

    auto batteryLevelEvent = std::make_shared< BatteryLevelEvent >( std::move( compareFunction ), levelItem );
    addOperator( batteryLevelEvent, parentOperatorPtr );
    handleEvent( batteryLevelEvent, eventContainer, whenBlock );

    eventContainer.batteryLevelEvents.push_back( batteryLevelEvent );
}

void EventParser::parseHasAtLeastOneDictionaryValueEvent(
    JsonObject& whenBlock,
    SceneEngine::EventContainer& eventContainer,
    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    std::string itemId, dictionaryValue;
    forEachField( *fieldsEntryIt,
                  [&itemId, &dictionaryValue]( const std::string& name,
                                               const std::string& /*type*/,
                                               const JsonObject& value,
                                               const std::string& /*scale*/ ) {
                      if( name == "item" && value.is_string() ) {
                          itemId = value.get< std::string >();
                      }
                      else if( name == "value" && value.is_string() ) {
                          dictionaryValue = value.get< std::string >();
                      }
                  } );
    if( itemId.empty() || dictionaryValue.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto dictionaryValueEvent = std::make_shared< DictionaryValueEvent >( std::move( dictionaryValue ) );
    addOperator( dictionaryValueEvent, parentOperatorPtr );
    handleEvent( dictionaryValueEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), itemId );
    eventContainer.dictionaryValueEvents.push_back( { searchKey, dictionaryValueEvent } );
}

void EventParser::parseIsDictionaryChanged( JsonObject& whenBlock,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );

    if( fieldsEntryIt == whenBlock.end() ) {
        LOGGER_ERROR( cc::error( "End of the when block" ) );
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "End of the when block " );
    }

    if( !JsonTypes::isArray( *fieldsEntryIt ) ) {
        LOGGER_ERROR( cc::error( "Is not array" ) );
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Is not array" );
    }

    std::string itemId, dictionaryKey, operation;
    forEachField( *fieldsEntryIt,
                  [&itemId, &dictionaryKey, &operation]( const std::string& name,
                                                         const std::string& /*type*/,
                                                         const JsonObject& value,
                                                         const std::string& /*scale*/ ) {
                      if( name == "item" && value.is_string() ) {
                          itemId = value.get< std::string >();
                      }
                      else if( name == "key" && value.is_string() ) {
                          dictionaryKey = value.get< std::string >();
                      }
                      else if( name == "operation" && value.is_string() ) {
                          operation = value.get< std::string >();
                      }
                  } );
    if( itemId.empty() ) {
        LOGGER_ERROR( cc::error( "Item id is empty" ) );
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Item id is empty" );
    }
    if( dictionaryKey.empty() ) {
        LOGGER_ERROR( cc::error( "dictionaryKey is empty" ) );
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "dictionaryKey is empty" );
    }
    auto dictionaryChangedEvent =
        std::make_shared< DictionaryChangedEvent >( itemId, std::move( dictionaryKey ), std::move( operation ) );
    addOperator( dictionaryChangedEvent, parentOperatorPtr );
    handleEvent( dictionaryChangedEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), itemId );
    LOGGER_TRACE( "item to add to dictionaryChangedEvents: (" + getControllerId( whenBlock ) + ", " + itemId + ")" );
    eventContainer.dictionaryChangedEvents.push_back( { searchKey, dictionaryChangedEvent } );
}

void EventParser::parseIsDetectedInHotzone( JsonObject& whenBlock,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );

    if( fieldsEntryIt == whenBlock.end() ) {
        LOGGER_ERROR( cc::error( "End of the when block" ) );
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "End of the when block " );
    }

    if( !JsonTypes::isArray( *fieldsEntryIt ) ) {
        LOGGER_ERROR( cc::error( "Is not array" ) );
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Is not array" );
    }

    std::string itemId;
    forEachField( *fieldsEntryIt,
                  [&itemId]( const std::string& name,
                             const std::string& /*type*/,
                             const JsonObject& value,
                             const std::string& /*scale*/ ) {
                      if( name == "item" && value.is_string() ) {
                          itemId = value.get< std::string >();
                      }
                  } );

    if( itemId.empty() )
        throw SceneEngine::SceneException( Errors::WRONG_PARAMS, "Field \"item\" is empty or missing" );

    auto getHotzonesField = [this, &whenBlock]( const std::string& name ) -> HotzoneEvent::Hotzones {
        Json field = getOptionalValueFromFields( name, whenBlock );

        if( field.is_array() ) {
            HotzoneEvent::Hotzones hotzones;
            for( auto& element : field ) {
                std::string hotzoneId;
                if( !json_get( element, hotzoneId ) ) {
                    throw SceneEngine::SceneException( Errors::WRONG_FIELD,
                                                       "Hotzone ID is not a string in " + name + " field" );
                }
                hotzones.emplace( hotzoneId );
            }
            return hotzones;
        }
        else if( field.empty() ) {
            return {};
        }
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, name + " field is not array/empty" );
    };

    auto getMatchField = [this, &whenBlock]( const std::string& name ) -> HotzoneEvent::MatchPolicy {
        Json field = getOptionalValueFromFields( name, whenBlock );
        if( field.empty() )
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, "Field " + name + " is missing" );

        std::string matchPolicyString;
        if( !json_get( field, matchPolicyString ) ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, "Field " + name + " is not a string" );
        }

        if( matchPolicyString == "any" )
            return HotzoneEvent::MatchPolicy::Any;
        else if( matchPolicyString == "none" )
            return HotzoneEvent::MatchPolicy::None;
        else if( matchPolicyString == "filter" )
            return HotzoneEvent::MatchPolicy::Filter;
        throw SceneEngine::SceneException( Errors::WRONG_PARAMS, "Invalid policy value for " + name + " field " );
    };

    auto getFiltersField = [this, &whenBlock]( const std::string& name,
                                               HotzoneEvent::MatchPolicy matchPolicy ) -> HotzoneEvent::Filters {
        using namespace HubPlatform::CoreD::Model;
        Json field = getOptionalValueFromFields( name, whenBlock );

        const bool filterRequired = ( matchPolicy == HotzoneEvent::MatchPolicy::Filter );

        if( filterRequired ) {
            if( field.empty() )
                throw SceneEngine::SceneException( Errors::WRONG_FIELD, "Field " + name + " is required" );
        }
        else if( !field.empty() ) {
            throw SceneEngine::SceneException(
                Errors::WRONG_FIELD, "Filters in " + name + " field cannot be used with selected \"match\" policy" );
        }

        HotzoneEvent::Filters filters;
        try {
            for( auto it = field.cbegin(); it != field.cend(); ++it ) {
                std::string detectableType = it.key();
                if( !HotzoneEvent::isDetectableType( detectableType ) )
                    throw SceneEngine::SceneException( Errors::WRONG_PARAMS,
                                                       "Non detectable type in " + name + " field " );
                auto& filterJson = it.value();

                if( !filterJson.is_object() )
                    throw SceneEngine::SceneException( Errors::WRONG_PARAMS,
                                                       "Filter is not an object type in " + name + " field " );

                if( filterJson.empty() )
                    throw SceneEngine::SceneException( Errors::WRONG_PARAMS, "Empty filter in " + name + " field " );

                auto filter = HotzoneEvent::Filter::createValue( it.value() );
                filters.emplace( std::move( detectableType ), std::move( *filter ) );
            }
        }
        catch( const SceneEngine::SceneException& e ) {
            throw;
        }
        catch( ... ) {
            throw SceneEngine::SceneException( Errors::WRONG_PARAMS, "Invalid filter in " + name + " field " );
        }

        return filters;
    };

    auto hotzones = getHotzonesField( "hotzones" );
    auto matchPolicy = getMatchField( "match" );
    auto filters = getFiltersField( "filters", matchPolicy );

    auto event = std::make_shared< HotzoneEvent >( std::move( hotzones ), matchPolicy, std::move( filters ) );
    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );

    ValueSourceKey searchKey( getControllerId( whenBlock ), itemId );
    eventContainer.hotzoneEvents.push_back( { searchKey, event } );
}

void EventParser::parseIsUserLockOperation( JsonObject& whenBlock,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;
    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK,
            "isUserLockOperation: when block should have an array with fields required for the method" );
    }
    std::string itemId, userActionType, userId;
    forEachField( *fieldsEntryIt,
                  [&itemId, &userActionType, &userId]( const std::string& name,
                                                       const std::string& /*type*/,
                                                       const JsonObject& value,
                                                       const std::string& /*scale*/ ) {
                      if( name == "item" && value.is_string() ) {
                          itemId = value.get< std::string >();
                      }
                      else if( name == "state" && value.is_string() ) {
                          userActionType = value.get< std::string >();
                      }
                      else if( name == "user" && value.is_string() ) {
                          userId = value.get< std::string >();
                      }
                  } );

    tiny::tools::trim( userId );

    if( itemId.empty() || userActionType.empty() || userId.empty() ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }

    int userIndex = 0;

    try {
        userIndex = std::stoi( userId );
    }
    catch( const std::invalid_argument& ) {
        throw SceneException( Errors::BAD_PARAMS );
    }

    auto userLockEvent = SceneEngine::ItemEventBaseFactory::makeUserLock( itemId, userActionType, userIndex );
    addOperator( userLockEvent, parentOperatorPtr );
    handleEvent( userLockEvent, eventContainer, whenBlock );
    ValueSourceKey searchKey( getControllerId( whenBlock ), itemId );
    eventContainer.userLockEvents.push_back( { searchKey, userLockEvent } );
}

void EventParser::parseIsIntervalCondition( JsonObject& whenBlock,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;

    SceneEngine::IntervalEventPtr event;
    entryProcessor( EventFields::BlockOptions,
                    whenBlock,
                    [&whenBlock, &event, this, parentOperatorPtr]( const JsonObject& blockOptions ) -> void {
                        event = createIntervalEvent( whenBlock, blockOptions );
                    } );

    if( !event )
        return;

    auto fieldsIt = getFields( whenBlock );
    auto intervalEntryIt = getField( EventFields::Fields::Interval, *fieldsIt );

    int64_t startTimeRaw = 0;
    if( !json_get( *intervalEntryIt, "startTime", startTimeRaw ) ) {
        auto currentTime = m_clockGetter().localTimeSinceEpoch();
        startTimeRaw = std::chrono::duration_cast< std::chrono::seconds >( currentTime ).count();
        ( *intervalEntryIt )[ "startTime" ] = startTimeRaw;
    }
    event->setStartTime( std::chrono::seconds( startTimeRaw ) );

    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );
    eventContainer.intervalEvents.push_back( event );
}

void EventParser::parseIsSunStateCondition( JsonObject& whenObject,
                                            SceneEngine::EventContainer& eventContainer,
                                            SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    auto isObject = []( const JsonObject& obj ) { return obj.is_object(); };
    JsonPathLeafType path{ { EventFields::BlockOptions, EventFields::Method, EventFields::Arguments }, isObject };

    entryProcessor(
        path, whenObject, [&whenObject, this, &eventContainer, parentOperatorPtr]( JsonObject& args ) -> void {
            entryProcessor(
                EventFields::Fields::Fields,
                whenObject,
                [&args, &whenObject, this, &eventContainer, parentOperatorPtr]( JsonObject& fields ) -> void {
                    auto sunStateEvent = createSunStateEvent( args, fields );
                    addOperator( sunStateEvent, parentOperatorPtr );
                    handleEvent( sunStateEvent, eventContainer, whenObject );
                    eventContainer.sunStateEvents.push_back( sunStateEvent );
                },
                JsonTypes::isArray );
        } );
}

void EventParser::parseIsDateCondition( JsonObject& whenBlock,
                                        SceneEngine::EventContainer& eventContainer,
                                        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto event = createDateEvent( whenBlock, eventContainer );
    if( event == nullptr ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );
}

void EventParser::parseIsDateRangeCondition( JsonObject& whenBlock,
                                             SceneEngine::EventContainer& eventContainer,
                                             SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto event = createDateRangeEvent( whenBlock );
    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );
    eventContainer.dateRangeEvents.push_back( event );
}

void EventParser::parseIsOnceCondition( JsonObject& whenBlock,
                                        SceneEngine::EventContainer& eventContainer,
                                        SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto event = createOnceEvent( whenBlock );
    if( event == nullptr ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );
    eventContainer.dateEvents.push_back( event );
}

void EventParser::parseIsHouseModeChangedTo( JsonObject& whenBlock,
                                             SceneEngine::EventContainer& eventContainer,
                                             SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto houseModeEvent = parseIsHouseMode( whenBlock, parentOperatorPtr );
    handleEvent( houseModeEvent.second, eventContainer, whenBlock );
    eventContainer.houseModeChangedToEvents.emplace_back( std::move( houseModeEvent ) );
}

void EventParser::parseIsHouseModeChangedFrom( JsonObject& whenBlock,
                                               SceneEngine::EventContainer& eventContainer,
                                               SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto houseModeEvent = parseIsHouseMode( whenBlock, parentOperatorPtr );
    handleEvent( houseModeEvent.second, eventContainer, whenBlock );
    eventContainer.houseModeChangedFromEvents.emplace_back( std::move( houseModeEvent ) );
}

void EventParser::parseIsHouseModeAlarmPhaseRange( JsonObject& whenBlock,
                                                   SceneEngine::EventContainer& eventContainer,
                                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto houseModeEvent = parseHouseModeAlarmPhaseRange( whenBlock, parentOperatorPtr );
    handleEvent( houseModeEvent.second, eventContainer, whenBlock );
    eventContainer.houseModeAlarmPhaseRangeEvents.emplace_back( std::move( houseModeEvent ) );
}

void EventParser::parseIsHouseModeSwitchToRange( JsonObject& whenBlock,
                                                 SceneEngine::EventContainer& eventContainer,
                                                 SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto houseModeEvent = parseHouseModeSwitchToRange( whenBlock, parentOperatorPtr );
    handleEvent( houseModeEvent.second, eventContainer, whenBlock );
    eventContainer.houseModeSwitchToRangeEvents.emplace_back( std::move( houseModeEvent ) );
}

void EventParser::parseIsFeatureState( JsonObject& whenBlock,
                                       SceneEngine::EventContainer& eventContainer,
                                       SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    auto featureStateEvent = parseFeatureStateEvent( whenBlock, parentOperatorPtr );
    handleEvent( featureStateEvent.second, eventContainer, whenBlock );
    eventContainer.featuresStatusEvents.emplace_back( std::move( featureStateEvent.second ) );
}

void EventParser::parseIsNetworkState( JsonObject& whenBlock,
                                       SceneEngine::EventContainer& eventContainer,
                                       SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto state = json_get_str( getValueFromFields( "state", whenBlock ) );
    if( state.empty() ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "wrong state field" );
    }
    auto interfaceId = json_get_str( getValueFromFields( "id", whenBlock ) );
    if( state == "main" ) {
        auto event = std::make_shared< NetworkMainStateEvent >( interfaceId );
        addOperator( event, parentOperatorPtr );
        handleEvent( event, eventContainer, whenBlock );
        eventContainer.networkMainStateEvents.emplace_back( std::move( event ) );
    }
    else if( state == "failed" ) {
        auto event = std::make_shared< NetworkFailedStateEvent >( interfaceId );
        addOperator( event, parentOperatorPtr );
        handleEvent( event, eventContainer, whenBlock );
        eventContainer.networkFailedStateEvents.emplace_back( std::move( event ) );
    }
    else {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "wrong 'state' field" );
    }
}

void EventParser::parseIsSceneState( JsonObject& whenBlock,
                                     SceneEngine::EventContainer& eventContainer,
                                     SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto sceneId = json_get_str( getValueFromFields( "scene", whenBlock ) );
    auto state = json_get_str( getValueFromFields( "state", whenBlock ) );
    SceneStateEvent::Action sceneAction;
    if( state == "failed" ) {
        sceneAction = SceneStateEvent::Action::FailedResult;
    }
    else if( state == "finished" ) {
        sceneAction = SceneStateEvent::Action::FinishedResult;
    }
    else if( state == "stopped" ) {
        sceneAction = SceneStateEvent::Action::StoppedResult;
    }
    else if( state == "partially_finished" ) {
        sceneAction = SceneStateEvent::Action::PartiallyFinishedResult;
    }
    else if( state == "any_result" ) {
        sceneAction = SceneStateEvent::Action::AnyResult;
    }
    else if( state == "scene_enabled" ) {
        sceneAction = SceneStateEvent::Action::SceneEnabled;
    }
    else if( state == "scene_disabled" ) {
        sceneAction = SceneStateEvent::Action::SceneDisabled;
    }
    else {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "wrong 'state' field" );
    }
    auto event = std::make_shared< SceneStateEvent >( sceneId, sceneAction );
    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );
    eventContainer.sceneStateEvents.emplace_back( std::move( event ) );
}

void EventParser::parseIsGroupState( JsonObject& whenBlock,
                                     SceneEngine::EventContainer& eventContainer,
                                     SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto sceneId = json_get_str( getValueFromFields( "scene", whenBlock ) );
    auto group = json_get_str( getValueFromFields( "group", whenBlock ) );
    auto stateStr = json_get_str( getValueFromFields( "state", whenBlock ) );
    GroupStateEvent::State state;
    if( stateStr == "true" ) {
        state = GroupStateEvent::State::True;
    }
    else if( stateStr == "false" ) {
        state = GroupStateEvent::State::False;
    }
    else if( stateStr == "changed" ) {
        state = GroupStateEvent::State::Changed;
    }
    else {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "state", "Invalid group state specified" );
    }
    auto event = std::make_shared< GroupStateEvent >( std::move( sceneId ), std::move( group ), state );
    addOperator( event, std::move( parentOperatorPtr ) );
    handleEvent( event, eventContainer, whenBlock );
    eventContainer.groupStateEvents.emplace_back( std::move( event ) );
}

void EventParser::parseIsCloudState( JsonObject& whenBlock,
                                     SceneEngine::EventContainer& eventContainer,
                                     SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto state = json_get_str( getValueFromFields( "state", whenBlock ) );
    CloudStateEvent::State cloudState;
    if( state == "connected" ) {
        cloudState = CloudStateEvent::State::Connected;
    }
    else if( state == "disconnected" ) {
        cloudState = CloudStateEvent::State::Disconnected;
    }
    else {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "wrong 'state' field" );
    }
    auto event = std::make_shared< CloudStateEvent >( cloudState );
    addOperator( event, parentOperatorPtr );
    handleEvent( event, eventContainer, whenBlock );
    eventContainer.cloudStateEvents.emplace_back( std::move( event ) );
}

void EventParser::parseIsDeviceState( JsonObject& whenBlock,
                                      SceneEngine::EventContainer& eventContainer,
                                      SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;

    JsonObject::iterator fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                              "isDeviceState: when block should have an array with fields required for the method" );
    }
    std::string deviceId;
    DeviceStateEvent::DeviceStateParams params;

    int sourcesCount = 0;
    std::pair< std::string, ValueSourceKey::SourceType > sourceInfo;
    std::pair< std::string, ValueSourceKey::SourceType > valueInfo;

    forEachField(
        *fieldsEntryIt,
        [&deviceId, &params, &sourceInfo, &valueInfo, &sourcesCount](
            const std::string& name, const std::string& type, const JsonObject& value, const std::string& /*scale*/ ) {
            if( name == "device" ) {
                if( value.is_string() ) {
                    deviceId = value.get< std::string >();
                }
                else {
                    throw SceneException( Errors::BAD_PARAMS, name );
                }
            }
            else {
                if( name == "armed" || name == "reachable" ) {
                    if( !value.is_boolean() )
                        throw SceneException( Errors::BAD_PARAMS, name );
                    params[ name ] = value;
                }
            }
        } );

    JsonArray& fields = *fieldsEntryIt;
    std::string itemId;

    bool isDeviceGroup = false;
    findField(
        EventFields::Fields::DeviceGroup, fields, [&itemId, &sourcesCount, &sourceInfo]( JsonObject& entry ) -> void {
            if( sourcesCount > 1 )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed" );

            entryProcessor( EventFields::Fields::Value,
                            entry,
                            [&itemId]( const JsonObject& entry ) -> void { itemId = entry.get< std::string >(); },
                            JsonTypes::isString,
                            []( const JsonObject&, const std::string& ) {
                                // value with the item id is not set
                                throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
                            } );

            if( sourcesCount == 0 ) {
                sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( itemId, "" ),
                                             ValueSourceKey::SourceType::DeviceItemGroups );
                ++sourcesCount;
                return;
            }

            if( sourcesCount > 1 || sourceInfo.second != ValueSourceKey::SourceType::DeviceItemGroups )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed 6" );

            auto unpackedIds = ValueSourceKey::unpackDeviceItemGroupIds( sourceInfo.first );
            auto& itemGroupId = unpackedIds.second;
            if( itemGroupId.empty() )
                throw SceneException( Errors::SCENE_WHEN_MULTIPLE_SOURCES_PROHIBITED,
                                      "Only one value source is allowed 7" );
            sourceInfo = std::make_pair( ValueSourceKey::packDeviceItemGroupIds( itemId, itemGroupId ),
                                         ValueSourceKey::SourceType::DeviceItemGroups );
            ++sourcesCount;
        } );

    ValueSourceKey searchKey( getControllerId( whenBlock ), sourceInfo.first, sourceInfo.second );
    if( !searchKey.isDeviceItemGroups() && deviceId.empty() ) {
        throw SceneException( Errors::BAD_PARAMS, "Not found 'device' parameter" );
    }

    if( !searchKey.isDeviceItemGroups() && params.empty() ) {
        throw SceneException( Errors::BAD_PARAMS, "Not found condition params" );
    }

    auto event = std::make_shared< DeviceStateEvent >( params );
    addOperator( event, std::move( parentOperatorPtr ) );
    handleEvent( event, eventContainer, whenBlock );

    if( searchKey.isDeviceItemGroups() ) {
        event->setKeepState( false );
        eventContainer.deviceStateEvents.push_back( { searchKey, event } );
    }
    else {
        ValueSourceKey searchKey( getControllerId( whenBlock ), deviceId );
        eventContainer.deviceStateEvents.push_back( { searchKey, event } );
    }
}

void EventParser::parseAndOperator( JsonObject& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    auto field = getField( "blocks", *fields );
    auto value = field->find( "value" );
    if( value == field->end() || !value->is_array() || value->empty() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    std::set< std::pair< std::string, std::string > > deviceTriggerMethods;
    auto checkMethod = [this, &deviceTriggerMethods](
                           const std::string& methodName, Errors::Error error, JsonArray& fields ) {
        auto itemId = getString( "item", fields );
        if( !deviceTriggerMethods.insert( { methodName, itemId } ).second ) {
            throw SceneEngine::SceneException( error );
        }
    };

    using namespace SceneEngine::Model::Resources;
    std::multimap< std::string, Range< Json > > compareNumbersRanges;
    for( JsonObject& whenBlock : *value ) {
        std::string methodName;
        if( !json_get( whenBlock, { "blockOptions", "method", "name" }, methodName ) ) {
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Can't read method name of a child" );
        }
        auto fields = getFields( whenBlock );
        auto itemCheck = checkItemParam( "item", *fields );
        if( !itemCheck ) {
            break;
        }
        if( methodName == MethodNames::isItemState ) {
            checkMethod( methodName, Errors::SCENE_WHEN_SAME_ITEM_IN_AND, *fields );
        }
        else if( methodName == MethodNames::isButtonState ) {
            checkMethod( methodName, Errors::SCENE_WHEN_SAME_BUTTON_IN_AND, *fields );
        }
        else if( methodName == MethodNames::compareNumbers ) {
            Json itemValue = getItemValue( *fields );
            if( itemValue.is_number() ) {
                auto itemId = getString( "item", *fields );
                auto comparator = getString( "comparator", *fields );
                if( !isIntersection( compareNumbersRanges, itemId, itemValue, comparator ) )
                    throw SceneException( Errors::SCENE_WHEN_NOT_INTERSECT_NUMBERS );
            }
        }
    }

    auto andOperator = OperatorFactory::makeAndOperator();
    andOperator->setParent( parentOperatorPtr );
    handleOperator( andOperator, eventContainer, whenBlock );
    for( JsonObject& whenBlock : *value ) {
        if( whenBlock.empty() ) {
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
        }
        parse( whenBlock, eventContainer, andOperator );
    }
}

void EventParser::parseNotOperator( JsonObject& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    auto field = getField( "block", *fields );
    auto value = field->find( "value" );
    if( value == field->end() || !value->is_object() || value->empty() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto notOperator = OperatorFactory::makeNotOperator();
    notOperator->setParent( parentOperatorPtr );
    handleOperator( notOperator, eventContainer, whenBlock );
    parse( *value, eventContainer, notOperator );
}

void EventParser::parseOrOperator( JsonObject& whenBlock,
                                   SceneEngine::EventContainer& eventContainer,
                                   SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    auto field = getField( "blocks", *fields );
    auto value = field->find( "value" );
    if( value == field->end() || !value->is_array() || value->empty() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto orOperator = OperatorFactory::makeOrOperator();
    orOperator->setParent( parentOperatorPtr );
    handleOperator( orOperator, eventContainer, whenBlock );
    for( JsonObject& whenBlock : *value ) {
        if( whenBlock.empty() ) {
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
        }
        parse( whenBlock, eventContainer, orOperator );
    }
}

void EventParser::parseXorOperator( JsonObject& whenBlock,
                                    SceneEngine::EventContainer& eventContainer,
                                    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    auto field = getField( "blocks", *fields );
    auto value = field->find( "value" );
    if( value == field->end() || !value->is_array() || value->empty() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
    auto xorOperator = OperatorFactory::makeXorOperator();
    xorOperator->setParent( parentOperatorPtr );
    handleOperator( xorOperator, eventContainer, whenBlock );
    for( JsonObject& childWhenBlock : *value ) {
        if( childWhenBlock.size() < 2 ) {
            LOGGER_ERROR( cc::error( "XOR operator needs at least two elements" ) );
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK,
                                               "XOR operator needs at least two elements" );
        }
        parse( childWhenBlock, eventContainer, xorOperator );
    }
}

void EventParser::parseFunctionOperator( JsonObject& whenBlock,
                                         SceneEngine::EventContainer& eventContainer,
                                         SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    Json settings = JSON_EMPTY_OBJ;
    if( !json_get( whenBlock, { EventFields::BlockOptions, EventFields::Function }, settings ) ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "No 'function' key in function operator" );
    }

    auto fields = getFields( whenBlock );
    auto field = getField( EventFields::Fields::Blocks, *fields );
    auto value = field->find( EventFields::Fields::Value );
    if( value == field->end() || !value->is_array() || value->empty() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }

    SceneEngine::AbstractOperator::SharedOperator functionOperator;
    try {
        if( settings.count( FunctionFields::For ) ) {
            functionOperator = parseForFunctionOperator( settings[ FunctionFields::For ] );
        }
        else if( settings.count( FunctionFields::Repeat ) ) {
            functionOperator = parseRepeatFunctionOperator( settings[ FunctionFields::Repeat ] );
        }
        else if( settings.count( FunctionFields::Follow ) ) {
            functionOperator = parseFollowFunctionOperator( settings[ FunctionFields::Follow ] );
        }
        else if( settings.count( FunctionFields::Pulse ) ) {
            functionOperator = parsePulseFunctionOperator( settings[ FunctionFields::Pulse ] );
        }
        else if( settings.count( FunctionFields::Latch ) ) {
            functionOperator = parseLatchFunctionOperator( settings[ FunctionFields::Latch ] );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Couldn't create function operator: " ) + cc::attention( ex.what() ) );
        const std::string message = std::string{ "Wrong function operator: " }.append( ex.what() );
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
    }

    if( functionOperator == nullptr ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Wrong function operator" );
    }

    functionOperator->setParent( parentOperatorPtr );
    handleOperator( functionOperator, eventContainer, whenBlock );
    for( JsonObject& childWhenBlock : *value ) {
        parse( childWhenBlock, eventContainer, functionOperator );
    }
}

SceneEngine::AbstractOperator::SharedOperator EventParser::parseForFunctionOperator( const Json& options ) const {
    const std::string method = json_get_str( options, FunctionFields::Method );
    const int secondsInterval = json_get_int( options, FunctionFields::Seconds );

    const auto interval = std::chrono::seconds( secondsInterval );
    return SceneEngine::OperatorFactory::makeForFunctionOperator( method, interval, m_taskLoop );
}

SceneEngine::AbstractOperator::SharedOperator EventParser::parseRepeatFunctionOperator( const Json& options ) const {
    const int times = json_get_int( options, FunctionFields::Times, -1 );
    const int secondsInterval = json_get_int( options, FunctionFields::Seconds );
    return SceneEngine::OperatorFactory::makeRepeatFunctionOperator( times, std::chrono::seconds( secondsInterval ) );
}

SceneEngine::AbstractOperator::SharedOperator EventParser::parseFollowFunctionOperator( const Json& options ) const {
    const int resetDelaySec = json_get_int( options, FunctionFields::DelayReset );
    return SceneEngine::OperatorFactory::makeFollowFunctionOperator( m_taskLoop,
                                                                     std::chrono::seconds( resetDelaySec ) );
}

SceneEngine::AbstractOperator::SharedOperator EventParser::parsePulseFunctionOperator( const Json& options ) const {
    const int truePeriodSec = json_get_int( options, FunctionFields::TruePeriod );
    const int falsePeriodSec = json_get_int( options, FunctionFields::FalsePeriod, 0 );

    std::chrono::seconds truePeriod( truePeriodSec );
    std::chrono::seconds falsePeriod( falsePeriodSec );

    int times = 0;
    if( json_get( options, FunctionFields::Times, times ) ) {
        return SceneEngine::OperatorFactory::makePulseFunctionOperator( m_taskLoop, truePeriod, falsePeriod, times );
    }
    return SceneEngine::OperatorFactory::makePulseFunctionOperator( m_taskLoop, truePeriod, falsePeriod );
}

SceneEngine::AbstractOperator::SharedOperator EventParser::parseLatchFunctionOperator( const Json& options ) const {
    Json enabled;
    if( json_get( options, "enabled", enabled ) && enabled.is_boolean() && enabled.get< bool >() ) {
        try {
            return SceneEngine::OperatorFactory::makeLatchFunctionOperator( enabled.get< bool >() );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Couldn't create function operator/latch: " ) + cc::attention( ex.what() ) );
            std::string message = std::string{ "Wrong function operator/latch: " }.append( ex.what() );
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, message.c_str() );
        }
    }
    else {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "function/latch/enabled should be true" );
    }
}

SceneEngine::EventContainer::HouseModeEvent EventParser::parseIsHouseMode(
    JsonObject& whenBlock,
    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    JsonArray jsonModes = getValueFromFields( "houseMode", whenBlock );
    if( !jsonModes.is_array() ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "houseMode field is not array" );
    }
    HouseModeEvent::Modes modes;
    for( auto& mode : jsonModes ) {
        std::string tmp;
        if( !json_get( mode, tmp ) ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD, "mode is not string" );
        }
        modes.emplace( std::move( tmp ) );
    }
    auto event = std::make_shared< HouseModeEvent >( std::move( modes ) );
    addOperator( event, parentOperatorPtr );
    return EventContainer::HouseModeEvent{ getControllerId( whenBlock ), event };
}

SceneEngine::EventContainer::HouseModeAlarmPhaseRangeEvent EventParser::parseHouseModeAlarmPhaseRange(
    JsonObject& whenBlock,
    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    std::string phaseStr = getString( "phase", *fields );
    if( !HouseModeAlarmPhase.count( phaseStr ) ) {
        throw SceneEngine::SceneException( Errors::WRONG_PARAMS, "invalid \"phase\" field value" );
    }

    auto event = std::make_shared< HouseModeAlarmedPhaseRangeEvent >( phaseStr );

    addOperator( event, parentOperatorPtr );
    return EventContainer::HouseModeAlarmPhaseRangeEvent{ getControllerId( whenBlock ), event };
}

SceneEngine::EventContainer::HouseModeSwitchToRangeEvent EventParser::parseHouseModeSwitchToRange(
    JsonObject& whenBlock,
    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    auto event = std::make_shared< HouseModeSwitchToRangeEvent >();

    addOperator( event, parentOperatorPtr );
    return EventContainer::HouseModeSwitchToRangeEvent{ getControllerId( whenBlock ), event };
}

SceneEngine::EventContainer::FeatureStateEvent EventParser::parseFeatureStateEvent(
    JsonObject& whenBlock,
    SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine;
    auto fields = getFields( whenBlock );
    std::string name = getString( "name", *fields );

    if( name.empty() ) {
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::WhenBlock, "required field 'name' is missing" );
    }

    auto statusField = SceneEngine::Utils::findField( "status", *fields );
    std::string version = getOptionalString( "version", *fields );
    std::string comparator = getOptionalString( "comparator", *fields );

    bool isStatusField = false;
    if( statusField != ( *fields ).end() ) {
        isStatusField = true;
    }

    if( !comparator.empty() &&
        ( comparator != ValueComparators::Equal && comparator != ValueComparators::NotEqual &&
          comparator != ValueComparators::Greater && comparator != ValueComparators::GreaterEqual &&
          comparator != ValueComparators::Less && comparator != ValueComparators::LessEqual ) ) {
        auto errorString = "comparator: " + comparator + " is wrong";
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::WhenBlock, errorString.c_str() );
    }

    if( !version.empty() ) {
        HubPlatform::Common::SemanticVersion< int > semver( version );
        if( !semver.isValid() ) {
            auto errorString = "version: " + comparator + " is not valid";
            throw SceneEngine::SceneException(
                Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::WhenBlock, errorString.c_str() );
        }
    }

    bool statusValue = json_get_bool( *statusField, "value" );
    auto event = std::make_shared< FeatureStateEvent >( name, isStatusField, statusValue, version, comparator );
    addOperator( event, parentOperatorPtr );
    return EventContainer::FeatureStateEvent{ getControllerId( whenBlock ), event };
}

JsonArray& EventParser::getTriggerBlocks( JsonObject& sceneJsonObject ) {
    JsonObject::iterator itBlock = sceneJsonObject.find( EventFields::WhenBlock );
    if( itBlock == sceneJsonObject.end() ) {
        throw SceneEngine::SceneException(
            Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::WhenBlock, "required field is missing" );
    }
    if( !itBlock->is_array() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, EventFields::WhenBlock, "array expected" );
    }
    return *itBlock;
}

void EventParser::parseActionGroupMapping( Json& whenBlock,
                                           SceneEngine::EventContainer::ActionGroupMappings& mappings ) {
    auto groupId = json_get_str( whenBlock, EventFields::ActionGroup );
    auto blockId = json_get_str( whenBlock, EventFields::Id );
    if( !groupId.empty() ) {
        mappings[ groupId ] = blockId;
    }
    whenBlock[ EventFields::ActionGroup ] = blockId;
}

void EventParser::parse( Json& whenBlock,
                         SceneEngine::EventContainer& eventContainer,
                         SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    using namespace SceneEngine::Utils;
    std::string methodName;
    JsonPathLeafType path{ { EventFields::BlockOptions, EventFields::Method, EventFields::Name }, JsonTypes::isString };
    entryProcessor( path,
                    whenBlock,
                    [&methodName]( JsonObject& obj ) -> void { methodName = obj.get< std::string >(); },
                    []( const JsonObject& arg, const std::string& errorFieldName ) -> void {
                        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
                    } );

    addEventId( whenBlock );
    parseActionGroupMapping( whenBlock, eventContainer.m_actionGroupMappings );

    using namespace SceneEngine::Model::Resources;
    static const EventParserMethods parsers = {
        { MethodNames::compareNumbers, &EventParser::parseCompareNumbersEvent },
        { MethodNames::compareNumberRange, &EventParser::parseCompareNumberRangeEvent },
        { MethodNames::compareStrings, &EventParser::parseCompareStringsEvent },
        { MethodNames::stringOperation, &EventParser::parseStringOperationEvent },
        { MethodNames::inArray, &EventParser::parseInArrayEvent },
        { MethodNames::compareValues, &EventParser::parseCompareValuesEvent },
        { MethodNames::isItemState, &EventParser::parseIsItemStateCondition },
        { MethodNames::isFeatureState, &EventParser::parseIsFeatureState },
        { MethodNames::isDeviceItemGroup, &EventParser::parseIsDeviceItemGroupEvent },
        { MethodNames::isItemStateChanged, &EventParser::parseIsItemStateChangedEvent },
        { MethodNames::isButtonState, &EventParser::parseIsButtonState },
        { MethodNames::isBatteryState, &EventParser::parseIsBatteryState },
        { MethodNames::isBatteryLevel, &EventParser::parseIsBatteryLevel },
        { MethodNames::isUserLockOperation, &EventParser::parseIsUserLockOperation },
        { MethodNames::isDate, &EventParser::parseIsDateCondition },
        { MethodNames::isDateRange, &EventParser::parseIsDateRangeCondition },
        { MethodNames::isOnce, &EventParser::parseIsOnceCondition },
        { MethodNames::isInterval, &EventParser::parseIsIntervalCondition },
        { MethodNames::isSunState, &EventParser::parseIsSunStateCondition },
        { MethodNames::hasAtLeastOneDictionaryValue, &EventParser::parseHasAtLeastOneDictionaryValueEvent },
        { MethodNames::isDictionaryChanged, &EventParser::parseIsDictionaryChanged },
        { MethodNames::isDetectedInHotzone, &EventParser::parseIsDetectedInHotzone },
        { MethodNames::isHouseModeChangedTo, &EventParser::parseIsHouseModeChangedTo },
        { MethodNames::isHouseModeChangedFrom, &EventParser::parseIsHouseModeChangedFrom },
        { MethodNames::isNetworkState, &EventParser::parseIsNetworkState },
        { MethodNames::isSceneState, &EventParser::parseIsSceneState },
        { MethodNames::isGroupState, &EventParser::parseIsGroupState },
        { MethodNames::isCloudState, &EventParser::parseIsCloudState },
        { MethodNames::isFirmwareUpdateState, &EventParser::parseIsFirmwareUpdateState },
        { MethodNames::isDeviceState, &EventParser::parseIsDeviceState },
        { MethodNames::IsHouseModeAlarmPhaseRange, &EventParser::parseIsHouseModeAlarmPhaseRange },
        { MethodNames::IsHouseModeAlarmSwitchToRange, &EventParser::parseIsHouseModeSwitchToRange },
        { OperatorNames::AndOperator, &EventParser::parseAndOperator },
        { OperatorNames::NotOperator, &EventParser::parseNotOperator },
        { OperatorNames::OrOperator, &EventParser::parseOrOperator },
        { OperatorNames::XorOperator, &EventParser::parseXorOperator },
        { OperatorNames::FunctionOperator, &EventParser::parseFunctionOperator },
    };

    auto it = parsers.find( methodName );
    if( it != parsers.cend() ) {
        const auto handler = it->second;
        LOGGER_TRACE_RAW( cc::trace( "Parsing when block: " + methodName ) );
        ( this->*handler )( whenBlock, eventContainer, parentOperatorPtr );
    }
    else {
        Log::error( cc::error( "Scenes doesn't support that method: " ) + cc::attention( methodName ) );
        throw SceneEngine::SceneException( Errors::SCENE_IS_USED_NOT_SUPPORTED_METHOD,
                                           std::string{ "Method: " }.append( methodName ).c_str() );
    }
}

SceneEngine::ItemValueComparator EventParser::convertComparator( const std::string& comparator,
                                                                 bool valueTypeHasIsLess ) {
    try {
        return SceneEngine::convertComparator( comparator, valueTypeHasIsLess );
    }
    catch( const SceneEngine::ConvertComparatorException& ex ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "Invalid comparator passed: " + comparator );
    }
}

void EventParser::addOperator( SceneEngine::EventBasePtr event,
                               SceneEngine::AbstractOperator::SharedOperator parentOperatorPtr ) {
    if( parentOperatorPtr ) {
        event->setOperator( parentOperatorPtr );
    }
}

SceneEngine::IntervalEventPtr EventParser::createIntervalEvent( const JsonObject& whenBlock,
                                                                const JsonObject& blockOptions ) {
    using namespace SceneEngine;
    JsonObject method;
    JsonObject arguments;
    if( !json_get_obj( blockOptions, "method", method ) ) {
        throw SceneException( Errors::WRONG_FIELD );
    }
    if( !json_get_obj( method, EventFields::Arguments, arguments ) ) {
        throw SceneException( Errors::WRONG_FIELD );
    }
    std::string comparisonValueField;
    if( !json_get( arguments, EventFields::Fields::Interval, comparisonValueField ) ) {
        throw SceneException( Errors::WRONG_FIELD );
    }
    JsonObject intervalValue;
    if( json_helper::get_field_from_array( comparisonValueField, whenBlock, intervalValue ) ) {
        return std::make_shared< IntervalEvent >( intervalValue );
    }
    return nullptr;
}

SceneEngine::SunStateEventPtr EventParser::createSunStateEvent( JsonObject& args, JsonObject& fields ) {
    using namespace SceneEngine;
    if( !fields.is_array() ) {
        throw SceneException( Errors::WRONG_FIELD );
    }

    JsonObject::iterator stateTypeIt = args.find( "sunstate" );
    if( stateTypeIt != args.end() && stateTypeIt->is_string() ) {
        std::string valueTypeStr;
        SceneEngine::Utils::FuncProcessor valueTypeSetter = [&valueTypeStr]( JsonObject& field ) {
            JsonObject::iterator valueIt = field.find( "value" );
            if( valueIt == field.end() ) {
                throw SceneException( Errors::WRONG_FIELD, "Wrong 'sunstate' field" );
            }
            valueTypeStr = valueIt->get< std::string >();
        };
        const auto& sunStateTypeStr = stateTypeIt->get< std::string >();
        SceneEngine::Utils::findField( sunStateTypeStr, fields, valueTypeSetter );

        if( valueTypeStr.empty() ) {
            throw SceneException( Errors::WRONG_FIELD, "Wrong 'sunstate' field" );
        }

        SunStateEvent::State stateType = SunStateEvent::stateTypeFrom( sunStateTypeStr );
        SunStateEvent::OffsetType valueType = SunStateEvent::valueTypeFrom( valueTypeStr );

        SunStateEventPtr sunStateEvent =
            std::make_shared< SunStateEvent >( stateType, valueType, m_getGeoPosition, m_clockGetter );

        JsonObject::iterator argTimeIt = args.find( "time" );
        if( valueType != SunStateEvent::OffsetType::IN_TIME && argTimeIt != args.end() && argTimeIt->is_string() ) {
            SceneEngine::Utils::FuncProcessor timeSetter = [&sunStateEvent]( JsonObject& field ) {
                JsonObject::iterator valueIt = field.find( "value" );
                if( valueIt == field.end() ) {
                    throw SceneException( Errors::WRONG_FIELD, "Wrong 'time' field" );
                }
                sunStateEvent->setTimeOffset( *valueIt );
            };
            const std::string& timeFieldValue = argTimeIt->get< std::string >();
            SceneEngine::Utils::findField( timeFieldValue, fields, timeSetter );
        }

        JsonObject::iterator argWeekDaysIt = args.find( "weekdays" );
        if( argWeekDaysIt != args.end() && argWeekDaysIt->is_string() ) {
            SceneEngine::Utils::FuncProcessor weekDaysSetter = [&sunStateEvent]( JsonObject& field ) {
                JsonObject::iterator valueIt = field.find( "value" );
                if( valueIt == field.end() ) {
                    throw SceneException( Errors::WRONG_FIELD, "Wrong 'weekdays' field" );
                }
                sunStateEvent->setWeekdays( *valueIt );
            };
            const std::string& weekDaysFieldName = argWeekDaysIt->get< std::string >();
            SceneEngine::Utils::findField( weekDaysFieldName, fields, weekDaysSetter );
        }

        JsonObject::iterator argDaysIt = args.find( "days" );
        if( argDaysIt != args.end() && argDaysIt->is_string() ) {
            if( argWeekDaysIt != args.end() ) {
                throw SceneException( Errors::WRONG_FIELD, "'weekdays' field is used with 'days' field" );
            }
            SceneEngine::Utils::FuncProcessor daysSetter = [&sunStateEvent]( JsonObject& field ) {
                JsonObject::iterator valueIt = field.find( "value" );
                if( valueIt == field.end() ) {
                    throw SceneException( Errors::WRONG_FIELD, "Wrong 'days' field" );
                }
                sunStateEvent->setDays( *valueIt );
            };
            const std::string& daysFieldName = argDaysIt->get< std::string >();
            SceneEngine::Utils::findField( daysFieldName, fields, daysSetter );
        }

        JsonObject::iterator argRangeIt = args.find( "range" );
        if( argRangeIt != args.end() && argRangeIt->is_string() ) {
            SceneEngine::Utils::FuncProcessor rangeSetter = [&sunStateEvent]( JsonObject& field ) {
                auto rangeTypeField = json_get_str( field, "value" );
                if( rangeTypeField.empty() ) {
                    throw SceneException( Errors::EMPTY_FIELD, "Empty 'range' field" );
                }
                if( rangeTypeField == "at" ) {
                    sunStateEvent->setRangeType( SunStateEvent::RangeType::AtMoment );
                }
                else if( rangeTypeField == "after" ) {
                    sunStateEvent->setRangeType( SunStateEvent::RangeType::TillMidnight );
                }
                else if( rangeTypeField == "before" ) {
                    sunStateEvent->setRangeType( SunStateEvent::RangeType::FromMidnight );
                }
                else {
                    throw SceneException( Errors::WRONG_FIELD, ( "Wrong range field: " + rangeTypeField ).c_str() );
                }
            };
            const std::string& rangeFieldName = argRangeIt->get< std::string >();
            SceneEngine::Utils::findField( rangeFieldName, fields, rangeSetter );
        }
        return sunStateEvent;
    }
    else {
        // isSunState should contain sunstate field at least !!!
        throw SceneException( Errors::WRONG_FIELD, "Wrong 'sunstate' field" );
    }
    return nullptr;
}

SceneEngine::DateEventBasePtr EventParser::createDateEvent( const JsonObject& whenBlock,
                                                            SceneEngine::EventContainer& eventContainer ) {
    using namespace SceneEngine;
    JsonObject fields = JSON_EMPTY_OBJ;
    if( !json_get_arr( whenBlock, "fields", fields ) ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    std::string dateEventType;
    bool isOk = SceneEngine::Utils::findField( "type", fields, [&dateEventType]( JsonObject& field ) {
        JsonObject::iterator valueIt = field.find( "value" );
        if( valueIt == field.end() || !valueIt->is_string() ) {
            throw SceneEngine::SceneException( Errors::WRONG_FIELD );
        }
        dateEventType = valueIt->get< std::string >();
    } );
    if( !isOk ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }

    if( dateEventType == "daily" ) {
        auto event = DateEventFactory::makeDailyEvent( parseTimeList( whenBlock ) );
        eventContainer.dateEvents.push_back( event );
        return event;
    }
    else if( dateEventType == "weekly" ) {
        return createPeriodicEvent( whenBlock, eventContainer, DaysOfPeriodEvent::Period::Week );
    }
    else if( dateEventType == "monthly" ) {
        return createPeriodicEvent( whenBlock, eventContainer, DaysOfPeriodEvent::Period::Month );
    }
    else if( dateEventType == "weeks" ) {
        JsonArray weeksJson = getValueFromFields( "weeks", whenBlock );
        auto weeks = jsonNumbersArrayToSet< DateEvent::Week >( weeksJson );
        auto event = std::make_shared< WeeksOfPeriodEvent >( WeeksOfPeriodEvent::Period::Month, std::move( weeks ) );
        eventContainer.dateRangeEvents.push_back( event );
        return event;
    }
    else if( dateEventType == "yearWeeks" ) {
        JsonArray weeksJson = getValueFromFields( "weeks", whenBlock );
        auto weeks = jsonNumbersArrayToSet< DateEvent::Week >( weeksJson );
        auto event = std::make_shared< WeeksOfPeriodEvent >( WeeksOfPeriodEvent::Period::Year, std::move( weeks ) );
        eventContainer.dateRangeEvents.push_back( event );
        return event;
    }
    return nullptr;
}

SceneEngine::DateEventBasePtr EventParser::createPeriodicEvent( const JsonObject& whenBlock,
                                                                SceneEngine::EventContainer& eventContainer,
                                                                SceneEngine::DaysOfPeriodEvent::Period period ) {
    using namespace SceneEngine;

    DateEvent::TimeList dailyTime = parseTimeList( whenBlock );
    const bool isRangeEvent = dailyTime.empty();

    JsonArray daysJson;
    switch( period ) {
    case DaysOfPeriodEvent::Period::Month:
        daysJson = getValueFromFields( "days", whenBlock );
        break;
    case DaysOfPeriodEvent::Period::Week:
        daysJson = getValueFromFields( "weekdays", whenBlock );
        break;
    }

    auto days = jsonNumbersArrayToSet< DateEventBase::Day >( daysJson );

    if( isRangeEvent ) {
        auto event = std::make_shared< DaysOfPeriodEvent >( period, std::move( days ) );
        eventContainer.dateRangeEvents.push_back( event );
        return event;
    }

    DateEventBasePtr event;
    switch( period ) {
    case DaysOfPeriodEvent::Period::Month:
        event = DateEventFactory::makeMonthlyEvent( std::move( dailyTime ), std::move( days ) );
        break;
    case DaysOfPeriodEvent::Period::Week:
        event = DateEventFactory::makeWeeklyEvent( std::move( dailyTime ), std::move( days ) );
        break;
    }
    eventContainer.dateEvents.push_back( event );
    return event;
}

SceneEngine::DateEventPtr EventParser::createOnceEvent( const JsonObject& whenBlock ) {
    using namespace SceneEngine;
    JsonObject fields = JSON_EMPTY_OBJ;
    if( !json_get_arr( whenBlock, "fields", fields ) ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    int64_t year;
    uint8_t month;
    uint8_t day;
    std::string time;
    if( !json_get( getValueFromFields( "year", whenBlock ), year ) ) {
        return nullptr;
    }
    if( !json_get( getValueFromFields( "month", whenBlock ), month ) ) {
        return nullptr;
    }
    if( !json_get( getValueFromFields( "day", whenBlock ), day ) ) {
        return nullptr;
    }
    if( !json_get( getValueFromFields( "time", whenBlock ), time ) ) {
        return nullptr;
    }

    GregorianCalendar::Date date;
    if( !date.setDate( year, month, day ) ) {
        LOGGER_ERROR( cc::error( "Specified date is not correct" ) );
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_DATE );
    }

    auto parsedTime = parseTime( time );
    const bool timeSetCorrectly = date.setHour( parsedTime.hour ) && date.setMinute( parsedTime.minute );
    if( !timeSetCorrectly ) {
        LOGGER_ERROR( cc::error( "Specified time is not correct" ) );
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_TIME );
    }

    /** Use local time with timezone offset at the moment of parsing. Update tzOffset when
     * timezone or DST changes */
    date.setTimeZoneOffset( m_clockGetter().timeZoneOffset() );

    LOGGER_DEBUG_RAW( "isOnce: " + date.toString() );

    return DateEventFactory::makeOnceEvent( date );
}

SceneEngine::DateRangeEventPtr EventParser::createDateRangeEvent( const JsonObject& whenBlock ) {
    using namespace SceneEngine;
    JsonObject fields = JSON_EMPTY_OBJ;
    if( !json_get_arr( whenBlock, "fields", fields ) ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD );
    }
    int64_t startYear = 1, endYear = std::numeric_limits< int64_t >::max();
    int8_t startMonth = 1, endMonth = 12;
    int8_t startDay = 1, endDay = 31;
    std::string startTimeStr = "00:00", endTimeStr = "23:59";

    bool isStartDefined = false;
    isStartDefined |= json_get( getOptionalValueFromFields( "startTime", whenBlock ), startTimeStr );
    isStartDefined |= json_get( getOptionalValueFromFields( "startDay", whenBlock ), startDay );
    isStartDefined |= json_get( getOptionalValueFromFields( "startMonth", whenBlock ), startMonth );
    isStartDefined |= json_get( getOptionalValueFromFields( "startYear", whenBlock ), startYear );

    bool isEndDefined = false;
    isEndDefined |= json_get( getOptionalValueFromFields( "endTime", whenBlock ), endTimeStr );
    isEndDefined |= json_get( getOptionalValueFromFields( "endDay", whenBlock ), endDay );
    isEndDefined |= json_get( getOptionalValueFromFields( "endMonth", whenBlock ), endMonth );
    isEndDefined |= json_get( getOptionalValueFromFields( "endYear", whenBlock ), endYear );

    if( !isStartDefined && !isEndDefined ) {
        throw SceneException( Errors::SCENE_DATE_RANGE_NOT_DEFINED );
    }

    auto dateRangeEvent = std::make_shared< DateRangeEvent >();
    if( !dateRangeEvent->setYearRange( startYear, endYear ) ) {
        throw SceneException( Errors::SCENE_WRONG_DATE );
    }
    if( !dateRangeEvent->setMonthAndDayRanges( { startMonth, endMonth }, { startDay, endDay } ) ) {
        throw SceneException( Errors::SCENE_WRONG_DATE );
    }
    auto startTime = parseTime( startTimeStr );
    auto endTime = parseTime( endTimeStr );
    if( !dateRangeEvent->setHourRange( startTime.hour, endTime.hour ) ||
        !dateRangeEvent->setMinuteRange( startTime.minute, endTime.minute ) ) {
        throw SceneException( Errors::SCENE_WRONG_TIME );
    }
    return dateRangeEvent;
}

Range< Json > EventParser::createRange( JsonObject json, const std::string& comparator ) {
    using namespace SceneEngine;
    using IntegerLimits = std::numeric_limits< JsonObject::number_integer_t >;
    using FloatLimits = std::numeric_limits< JsonObject::number_float_t >;
    if( json.is_number_integer() ) {
        auto number = json.get< JsonObject::number_integer_t >();
        if( comparator == ValueComparators::Equal ) {
            return { json, json, RangeType::Included };
        }
        else if( comparator == ValueComparators::NotEqual ) {
            return { json, json, RangeType::Excluded };
        }
        else if( comparator == ValueComparators::Greater ) {
            ++number;
            return { number, IntegerLimits::max(), RangeType::Included };
        }
        else if( comparator == ValueComparators::GreaterEqual ) {
            return { number, IntegerLimits::max(), RangeType::Included };
        }
        else if( comparator == ValueComparators::Less ) {
            --number;
            return { IntegerLimits::min(), number, RangeType::Included };
        }
        else if( comparator == ValueComparators::LessEqual ) {
            return { IntegerLimits::min(), number, RangeType::Included };
        }
        else {
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
        }
    }
    else if( json.is_number_float() ) {
        auto number = json.get< JsonObject::number_float_t >();
        if( comparator == ValueComparators::Equal ) {
            return { json, json, RangeType::Included };
        }
        else if( comparator == ValueComparators::NotEqual ) {
            return { json, json, RangeType::Excluded };
        }
        else if( comparator == ValueComparators::Greater ) {
            number += FloatLimits::epsilon();
            return { number, FloatLimits::max(), RangeType::Included };
        }
        else if( comparator == ValueComparators::GreaterEqual ) {
            return { number, FloatLimits::max(), RangeType::Included };
        }
        else if( comparator == ValueComparators::Less ) {
            number -= FloatLimits::epsilon();
            return { FloatLimits::min(), number, RangeType::Included };
        }
        else if( comparator == ValueComparators::LessEqual ) {
            return { FloatLimits::min(), number, RangeType::Included };
        }
        else {
            throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
        }
    }
    else {
        throw SceneException( Errors::SCENE_WRONG_WHEN_BLOCK );
    }
}

bool EventParser::isIntersection( std::multimap< std::string, Range< Json > >& compareNumbersRanges,
                                  const std::string& itemId,
                                  const JsonObject& itemValue,
                                  const std::string& comparator ) {
    Range< Json > range = createRange( itemValue, comparator );
    auto rangesIteratorPair = compareNumbersRanges.equal_range( itemId );
    for( auto rangeIt = rangesIteratorPair.first; rangeIt != rangesIteratorPair.second; ++rangeIt ) {
        if( !range.isIntersection( rangeIt->second ) ) {
            return false;
        }
    }
    compareNumbersRanges.insert( { itemId, range } );
    return true;
}

Json EventParser::getValueFromFields( const std::string& key, const JsonObject& whenBlock ) {
    JsonObject value;
    if( !json_helper::get_field_from_array( key, whenBlock, value ) ) {
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, "no array in field" );
    }
    return value;
}

Json EventParser::getOptionalValueFromFields( const std::string& key, const JsonObject& whenBlock ) {
    JsonObject value = JSON_EMPTY_OBJ;
    json_helper::get_field_from_array( key, whenBlock, value );
    return value;
}

JsonObject::iterator EventParser::getFields( JsonObject& whenBlock ) {
    auto fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !SceneEngine::Utils::JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "no fields are present" );
    }
    return fieldsEntryIt;
}

JsonObject::iterator EventParser::getField( const std::string& name, JsonArray& fields ) {
    auto field = SceneEngine::Utils::findField( name, fields );
    if( field == fields.end() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "field in not found" );
    }
    return field;
}

bool EventParser::checkItemParam( const std::string& name, JsonArray& fields ) {
    auto field = SceneEngine::Utils::findField( name, fields );
    if( field == fields.end() ) {
        return false;
    }
    return true;
}

JsonObject::const_iterator EventParser::getFields( const JsonObject& whenBlock ) {
    auto fieldsEntryIt = whenBlock.find( "fields" );
    if( fieldsEntryIt == whenBlock.end() || !SceneEngine::Utils::JsonTypes::isArray( *fieldsEntryIt ) ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "no fields are present" );
    }
    return fieldsEntryIt;
}

JsonObject::const_iterator EventParser::getField( const std::string& name, const JsonArray& fields ) {
    auto field = SceneEngine::Utils::findField( name, fields );
    if( field == fields.end() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "field in not found" );
    }
    return field;
}

JsonObject EventParser::getItemValue( JsonArray& fields ) {
    auto field = getField( "value", fields );
    Json itemValue;
    if( !json_get( *field, "value", itemValue ) ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "field value in not found" );
    }
    return itemValue;
}

JsonObject EventParser::getItemScale( JsonArray& fields ) {
    auto field = getField( "value", fields );
    Json itemScale;
    const char* key = "scale";
    if( !json_get( *field, key, itemScale ) ) {
        return Json{};
    }
    return itemScale;
}

std::string EventParser::getString( const std::string& fieldName, JsonArray& fields ) {
    auto field = getField( fieldName, fields );
    auto value = json_get_str( *field, "value" );
    if( value.empty() ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_WHEN_BLOCK, "string value is empty" );
    }
    return value;
}

std::string EventParser::getOptionalString( const std::string& fieldName, JsonArray& fields ) {
    auto field = SceneEngine::Utils::findField( fieldName, fields );
    auto value = json_get_str( *field, "value" );
    return value;
}

EventParser::ControllerId EventParser::getControllerId( const Json& whenBlock ) {
    auto controllerId = json_get_str( whenBlock, EventFields::ControllerId );
    if( controllerId.empty() ) {
        controllerId = m_defaultControllerId;
    }
    return controllerId;
}

EventParser::Time EventParser::parseTime( const std::string& time ) {
    auto secondsDelimiter = time.find( ':' );
    if( secondsDelimiter == std::string::npos ) {
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_TIME_FORMAT, "wrong time format" );
    }
    try {
        const std::string hoursStr = time.substr( 0, secondsDelimiter );
        const std::string minutesStr = time.substr( secondsDelimiter + 1 );
        return Time{ std::stoi( hoursStr ), std::stoi( minutesStr ) };
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_TIME_FORMAT, "wrong time format" );
    }
}

std::set< SceneEngine::DateEvent::TimeType > EventParser::parseTimeList( const JsonObject& whenBlock ) {
    auto fieldsIt = getFields( whenBlock );
    auto timeFieldIt = getField( "time", *fieldsIt );

    JsonArray timeList;
    if( !json_get_arr( *timeFieldIt, EventFields::Fields::Value, timeList ) )
        throw SceneEngine::SceneException( Errors::SCENE_WRONG_TIME_FORMAT,
                                           "wrong time format: the field is not an array" );

    std::set< SceneEngine::DateEvent::TimeType > result;
    for( JsonArray::const_iterator it = timeList.cbegin(); it != timeList.cend(); ++it ) {
        if( !it->is_string() )
            throw SceneEngine::SceneException( Errors::SCENE_WRONG_TIME_FORMAT, "wrong time format" );

        auto time = parseTime( *it );
        result.emplace( time.hour, time.minute );
    }
    return result;
}

void EventParser::addEventId( Json& whenBlock ) const {
    auto it = whenBlock.find( EventFields::Id );
    if( it == whenBlock.end() ) {
        whenBlock[ EventFields::Id ] = Db::genOid();
    }
    else if( m_placeholdersMapping ) {
        std::string id = json_get_str( *it );
        if( IdUtils::isPlaceholder( id ) ) {
            *it = IdUtils::replacePlaceholder( id, *m_placeholdersMapping );
        }
    }
}

void EventParser::handleEvent( SceneEngine::EventBasePtr eventPtr,
                               SceneEngine::EventContainer& eventContainer,
                               const Json& whenBlock ) {
    eventPtr->setEnabled( isWhenBlockEnabled( whenBlock ) );

    std::string id = getBlockID( whenBlock );
    eventPtr->setEventId( id );

    std::string blockName = getBlockName( whenBlock );
    eventPtr->setBlockName( std::move( blockName ) );

    auto blockOperatorPtr = eventContainer.findBlockOperator( id );
    if( blockOperatorPtr.get() == nullptr ) {
        eventContainer.events.emplace( id, eventPtr );
    }
    else {
        LOGGER_ERROR( cc::error( "Event with the same ID already exists, id: " ) + id +
                      cc::error( ", event method name: " ) + eventPtr->getMethodName() );
        throw SceneEngine::SceneException( Errors::SCENE_BLOCK_ID_DUPLICATE,
                                           "A block with such id already exists: " + eventPtr->getEventId() );
    }
}

void EventParser::handleOperator( SceneEngine::AbstractOperator::SharedOperator operatorPtr,
                                  SceneEngine::EventContainer& eventContainer,
                                  const Json& whenBlock ) {
    operatorPtr->setEnabled( isWhenBlockEnabled( whenBlock ) );

    std::string id = getBlockID( whenBlock );
    operatorPtr->setBlockID( id );

    std::string blockName = getBlockName( whenBlock );
    operatorPtr->setBlockName( std::move( blockName ) );

    auto blockOperatorPtr = eventContainer.findBlockOperator( id );
    if( blockOperatorPtr.get() == nullptr ) {
        eventContainer.operators.emplace( id, operatorPtr );
    }
    else {
        SceneEngine::OperatorNameVisitor nameVisitor;
        operatorPtr->accept( nameVisitor );

        std::string operatorName = nameVisitor.name();
        if( nameVisitor.isFunction() )
            operatorName += "." + nameVisitor.functionName();
        LOGGER_ERROR( cc::error( "Operator with the same ID already exists, id: " ) + id +
                      cc::error( ", operator name: " ) + operatorName );
        throw SceneEngine::SceneException( Errors::SCENE_BLOCK_ID_DUPLICATE,
                                           "An operator with such id already exists: " + id );
    }
}

bool EventParser::isWhenBlockEnabled( const Json& whenBlock ) {
    return json_get_bool( whenBlock, "enabled", true );
}

std::string EventParser::getBlockID( const Json& whenBlock ) {
    return json_get_str( whenBlock, EventFields::Id );
}

std::string EventParser::getBlockName( const Json& whenBlock ) {
    return json_get_str( whenBlock, EventFields::BlockName );
}

bool EventParser::checkCompareNumberRangeParameters( std::string& itemId,
                                                     SceneEngine::ItemValueWrapper& startValue,
                                                     SceneEngine::ItemValueWrapper& endValue ) const {
    if( itemId.empty() ) {
        LOGGER_ERROR( "Wrong compareNumberRange itemId parameter" );
        return false;
    }
    else if( startValue.empty() ) {
        LOGGER_ERROR( "Wrong compareNumberRange startValue parameter" );
        return false;
    }
    else if( endValue.empty() ) {
        LOGGER_ERROR( "Wrong compareNumberRange endValue parameter" );
        return false;
    }
    else if( startValue > endValue ) {
        LOGGER_ERROR( "Wrong compareNumberRange boundaries" );
        return false;
    }

    return true;
}

JsonObject EventParser::createItemValueWrapperPayload( const Json& value,
                                                       const std::string& type,
                                                       const std::string& scale ) {
    JsonObject payload = JSON( "type" << type );
    if( !scale.empty() )
        payload[ "value" ] = JSON( "value" << value << "scale" << scale );
    else
        payload[ "value" ] = value;
    return payload;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
