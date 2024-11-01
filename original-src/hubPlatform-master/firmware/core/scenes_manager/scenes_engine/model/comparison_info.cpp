#include "comparison_info.h"
#include "event_names.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "cored/model/values/value_families.h"

namespace SceneEngine {
namespace Model {

const std::string ComparisonMethodsVersion = "1.0.0";

const OperatorsMapper::EventNameEnumOperatorInfoMap& OperatorsMapper::operatorInfoMap() {
    using namespace Model::Resources;
    namespace ValueFamilies = HubPlatform::CoreD::Model::Resources::ValueFamilies;

    static EventNameEnumOperatorInfoMap map = {

        { EventName::Enum::compareNumberRange,
          { { { { ValueComparators::Between, Labels::Between }, { ValueComparators::NotBetween, Labels::NotBetween } },
              MethodNames::compareNumberRange,
              ValueFamilies::Numeric,
              "enum" } } },

        { EventName::Enum::compareNumbers,
          { { { { { ValueComparators::Less, Labels::Less },
                  { ValueComparators::Greater, Labels::Greater },
                  { ValueComparators::LessEqual, Labels::LessEqual },
                  { ValueComparators::GreaterEqual, Labels::GreaterEqual },
                  { ValueComparators::Equal, Labels::Equal },
                  { ValueComparators::NotEqual, Labels::NotEqual } } },
              MethodNames::compareNumbers,
              ValueFamilies::Numeric,
              "enum" } } },

        { EventName::Enum::compareStrings,
          { { { { { ValueComparators::Less, Labels::Less },
                  { ValueComparators::Greater, Labels::Greater },
                  { ValueComparators::LessEqual, Labels::LessEqual },
                  { ValueComparators::GreaterEqual, Labels::GreaterEqual },
                  { ValueComparators::Equal, Labels::Equal },
                  { ValueComparators::NotEqual, Labels::NotEqual } } },
              MethodNames::compareStrings,
              ValueFamilies::Strings,
              "enum" } } },

        { EventName::Enum::stringOperation,
          { { { { StringOperations::Begin, Labels::Begin },
                { StringOperations::End, Labels::End },
                { StringOperations::Contain, Labels::Contain },
                { StringOperations::Length, Labels::Length },
                { StringOperations::NotBegin, Labels::NotBegin },
                { StringOperations::NotEnd, Labels::NotEnd },
                { StringOperations::NotContain, Labels::NotContain },
                { StringOperations::NotLength, Labels::NotLength } },
              MethodNames::stringOperation,
              ValueFamilies::Strings,
              "enum" } } },

        { EventName::Enum::inArray,
          { { { { Resources::ArrayOperations::In, Labels::In }, { Resources::ArrayOperations::NotIn, Labels::NotIn } },
              MethodNames::inArray,
              ValueFamilies::Array,
              "enum" } } },

        { EventName::Enum::compareValues,
          {
              { { { ValueComparators::Less, Labels::Less },
                  { ValueComparators::Greater, Labels::Greater },
                  { ValueComparators::LessEqual, Labels::LessEqual },
                  { ValueComparators::GreaterEqual, Labels::GreaterEqual },
                  { ValueComparators::Equal, Labels::Equal },
                  { ValueComparators::NotEqual, Labels::NotEqual } },
                MethodNames::compareValues,
                ValueFamilies::ValuesWithLess,
                "enum" },

              { { { ValueComparators::Equal, Labels::Equal }, { ValueComparators::NotEqual, Labels::NotEqual } },
                MethodNames::compareValues,
                ValueFamilies::ValuesWithoutLess,
                "enum" },
          } },

        { EventName::Enum::isFeatureState,
          { { { { { ValueComparators::Less, Labels::Less },
                  { ValueComparators::Greater, Labels::Greater },
                  { ValueComparators::LessEqual, Labels::LessEqual },
                  { ValueComparators::GreaterEqual, Labels::GreaterEqual },
                  { ValueComparators::Equal, Labels::Equal },
                  { ValueComparators::NotEqual, Labels::NotEqual } } },
              MethodNames::isFeatureState,
              ValueFamilies::Strings,
              "enum" } } },

    };
    return map;
}

const std::map< EventName::Enum, std::vector< MethodInfo > >& OperatorsMapper::methodInfoMap() {
    using namespace Resources::DataSources;
    static std::map< EventName::Enum, std::vector< MethodInfo > > map = {
        { EventName::Enum::isItemState,
          { { 0, "", { DeviceItem, Item, DeviceGroup, ItemGroup } }, { 1, "", { Constant, Expression } } } },
        { EventName::Enum::isDeviceState, { { 0, "", { Device, DeviceGroup } }, { 1, "", { Constant } } } },
        { EventName::Enum::isItemStateChanged,
          { { 0, "", { Item, Expression } },
            { 1, "start", { Constant, Expression } },
            { 2, "finish", { Constant, Expression } } } },

        { EventName::Enum::compareNumbers,
          { { 0, "", { Item, Expression, DeviceGroup, ItemGroup } }, { 1, "", { Constant, Item, Expression } } } },
        { EventName::Enum::compareNumberRange,
          { { 0, "", { Item, Expression, DeviceGroup, ItemGroup } },
            { 1, "startValue", { Constant } },
            { 2, "endValue", { Constant } } } },
        { EventName::Enum::compareStrings,
          { { 0, "", { Item, Expression, DeviceGroup, ItemGroup } }, { 1, "", { Constant, Item, Expression } } } },
        { EventName::Enum::stringOperation,
          { { 0, "", { Item, Expression, DeviceGroup, ItemGroup } }, { 1, "", { Constant, Item, Expression } } } },
        { EventName::Enum::compareValues,
          { { 0, "", { Item, Expression, DeviceGroup, ItemGroup } }, { 1, "", { Constant, Item, Expression } } } },

        { EventName::Enum::inArray,
          { { 0, "", { Item, Expression, DeviceGroup, ItemGroup } }, { 1, "", { Constant } } } },
        { EventName::Enum::isDeviceItemGroup, { { 0, "", { DeviceGroup, ItemGroup } }, { 1, "", {} } } },
        { EventName::Enum::isFeatureState, { { 0, "status", { Constant } }, { 1, "version", { Constant } } } },
    };

    return map;
}

const std::map< std::string, EventName::Enum >& OperatorsMapper::enumFromMethodNameMap() {
    namespace Methods = Resources::MethodNames;
    static std::map< std::string, EventName::Enum > res = {
        { Methods::isBatteryLevel, EventName::Enum::isBatteryLevel },
        { Methods::isBatteryState, EventName::Enum::isBatteryState },
        { Methods::isButtonState, EventName::Enum::isButtonState },
        { Methods::isCloudState, EventName::Enum::isCloudState },
        { Methods::compareNumberRange, EventName::Enum::compareNumberRange },
        { Methods::compareNumbers, EventName::Enum::compareNumbers },
        { Methods::compareStrings, EventName::Enum::compareStrings },
        { Methods::inArray, EventName::Enum::inArray },
        { Methods::stringOperation, EventName::Enum::stringOperation },
        { Methods::compareValues, EventName::Enum::compareValues },
        { Methods::isDate, EventName::Enum::isDate },
        { Methods::isDateRange, EventName::Enum::isDateRange },
        { Methods::isDictionaryChanged, EventName::Enum::isDictionaryChanged },
        { Methods::isFirmwareUpdateState, EventName::Enum::isFirmwareUpdateState },
        { Methods::isGroupState, EventName::Enum::isGroupState },
        { Methods::isHouseModeChanged, EventName::Enum::isHouseModeChanged },
        { Methods::isItemState, EventName::Enum::isItemState },
        { Methods::isItemStateChanged, EventName::Enum::isItemStateChanged },
        { Methods::isNetworkState, EventName::Enum::isNetworkState },
        { Methods::isOnce, EventName::Enum::isOnce },
        { Methods::isSceneState, EventName::Enum::isSceneState },
        { Methods::isSunState, EventName::Enum::isSunState },
        { Methods::isUserLockOperation, EventName::Enum::isUserLockOperation },
        { Methods::hasAtLeastOneDictionaryValue, EventName::Enum::hasAtLeastOneDictionaryValue },
        { Methods::isDeviceItemGroup, EventName::Enum::isDeviceItemGroup },
        { Methods::isDeviceState, EventName::Enum::isDeviceState },
        { Methods::isFeatureState, EventName::Enum::isFeatureState },
    };

    return res;
}

EventName::Enum OperatorsMapper::getEnumFromMethodName( const std::string& methodName ) {
    auto mapper = enumFromMethodNameMap();
    auto it = mapper.find( methodName );
    if( it != mapper.end() )
        return it->second;
    return EventName::Enum::None;
}

std::string OperatorsMapper::getMethodNameFromEnum( const EventName::Enum enumValue ) {
    auto mapper = enumFromMethodNameMap();
    for( const auto& item : mapper ) {
        if( item.second == enumValue ) {
            return item.first;
        }
    }
    return "";
}

}  // namespace Model
}  // namespace SceneEngine
