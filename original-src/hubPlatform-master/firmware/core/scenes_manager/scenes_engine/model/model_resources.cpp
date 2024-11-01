#include "model_resources.h"

namespace SceneEngine {
namespace Model {

std::vector< std::string > ScenesValueTypes = { "24_hours_interval", "interval", "hms_interval" };

/** NB: If you are adding any new logic operator, it should also be added to
    modify_scenes/sceneScaleModifier::isLogicOperatorBlock */
bool isLogicOperator( const std::string& methodName ) {
    using namespace Resources::OperatorNames;
    return isOperatorAnd( methodName ) || isOperatorOr( methodName ) || isOperatorNot( methodName ) ||
           isOperatorXor( methodName ) || isOperatorFunction( methodName );
}

bool isOperatorNot( const std::string& methodName ) {
    using namespace Resources::OperatorNames;
    return methodName == NotOperator;
}

bool isOperatorAnd( const std::string& methodName ) {
    using namespace Resources::OperatorNames;
    return methodName == AndOperator;
}

bool isOperatorOr( const std::string& methodName ) {
    using namespace Resources::OperatorNames;
    return methodName == OrOperator;
}

bool isOperatorXor( const std::string& methodName ) {
    using namespace Resources::OperatorNames;
    return methodName == XorOperator;
}

bool isOperatorFunction( const std::string& methodName ) {
    using namespace Resources::OperatorNames;
    return methodName == FunctionOperator;
}

namespace Resources {

namespace MethodNames {
const char* const isBatteryLevel = "isBatteryLevel";
const char* const isBatteryState = "isBatteryState";
const char* const isButtonState = "isButtonState";
const char* const isCloudState = "isCloudState";
const char* const compareNumberRange = "compareNumberRange";
const char* const compareNumbers = "compareNumbers";
const char* const compareStrings = "compareStrings";
const char* const stringOperation = "stringOperation";
const char* const inArray = "inArray";
const char* const compareValues = "compareValues";
const char* const isDate = "isDate";
const char* const isDateRange = "isDateRange";
const char* const isInterval = "isInterval";
const char* const isDictionaryChanged = "isDictionaryChanged";
const char* const isDetectedInHotzone = "isDetectedInHotzone";
const char* const isFirmwareUpdateState = "isFirmwareUpdateState";
const char* const isGroupState = "isGroupState";
const char* const isHouseModeChanged = "isHouseModeChanged";
const char* const isItemState = "isItemState";
const char* const isDeviceItemGroup = "isDeviceItemGroup";
const char* const isItemStateChanged = "isItemStateChanged";
const char* const isNetworkState = "isNetworkState";
const char* const isOnce = "isOnce";
const char* const isSceneState = "isSceneState";
const char* const isSunState = "isSunState";
const char* const isUserLockOperation = "isUserLockOperation";
const char* const hasAtLeastOneDictionaryValue = "hasAtLeastOneDictionaryValue";
const char* const isHouseModeChangedTo = "isHouseModeChangedTo";
const char* const isHouseModeChangedFrom = "isHouseModeChangedFrom";
const char* const isDeviceState = "isDeviceState";
const char* const IsHouseModeAlarmPhaseRange = "isHouseModeAlarmPhaseRange";
const char* const IsHouseModeAlarmSwitchToRange = "isHouseModeSwitchToRange";
const char* const isFeatureState = "isFeatureState";
}  // namespace MethodNames

namespace OperatorNames {
const char* const AndOperator = "and";
const char* const NotOperator = "not";
const char* const OrOperator = "or";
const char* const XorOperator = "xor";
const char* const FunctionOperator = "function";
}  // namespace OperatorNames

namespace FunctionFields {
const char* const For = "for";
const char* const ForLess = "for/less";
const char* const ForAtLeast = "for/at_least";
const char* const Method = "method";
const char* const Seconds = "seconds";
const char* const Repeat = "repeat";
const char* const Times = "times";
const char* const Follow = "follow";
const char* const DelayReset = "delayReset";
const char* const Pulse = "pulse";
const char* const TruePeriod = "truePeriod";
const char* const FalsePeriod = "falsePeriod";
const char* const Latch = "latch";
const char* const Enabled = "enabled";
}  // namespace FunctionFields

namespace StringOperations {
const char* const Begin = "begin";
const char* const End = "end";
const char* const Contain = "contain";
const char* const Length = "length";
const char* const NotBegin = "not_begin";
const char* const NotEnd = "not_end";
const char* const NotContain = "not_contain";
const char* const NotLength = "not_length";
}  // namespace StringOperations

namespace HotzoneEventFields {
const char* const Persons = "persons";
const char* const Objects = "objects";
const char* const QRCodes = "qrcodes";
}  // namespace HotzoneEventFields

namespace ValueComparators {
const char* const Equal = "==";
const char* const NotEqual = "!=";
const char* const Greater = ">";
const char* const Less = "<";
const char* const GreaterEqual = ">=";
const char* const LessEqual = "<=";
const char* const Between = "between";
const char* const NotBetween = "not_between";
}  // namespace ValueComparators

namespace Labels {
const char* const Equal = "equal";
const char* const NotEqual = "not equal";
const char* const Greater = "greater";
const char* const Less = "less";
const char* const GreaterEqual = "greater equal";
const char* const LessEqual = "less equal";
const char* const Between = "between";
const char* const NotBetween = "not between";
const char* const Begin = "begins with";
const char* const End = "ends with";
const char* const Contain = "contains";
const char* const Length = "length equal to";
const char* const NotBegin = "doesn't begin with";
const char* const NotEnd = "doesn't end with";
const char* const NotContain = "doesn't contain";
const char* const NotLength = "length not equal to";
const char* const In = "in";
const char* const NotIn = "not in";
}  // namespace Labels

namespace ArrayOperations {
const char* const In = "in";
const char* const NotIn = "not_in";
}  // namespace ArrayOperations

namespace DataSources {
const char* const Item = "item";
const char* const Device = "device";
const char* const DeviceItem = "device_item";
const char* const Constant = "constant";
const char* const Expression = "expression";
const char* const DeviceGroup = "device_group";
const char* const ItemGroup = "item_group";
}  // namespace DataSources

namespace EventFields {
const char* const Start = "start";
const char* const Finish = "finish";
}  // namespace EventFields

namespace ActionNames {
const char* const RunCustomScriptAction = "runCustomScript";
const char* const RunPluginScriptAction = "runPluginScript";
const char* const RunSceneAction = "runScene";
const char* const StopSceneAction = "stopScene";
const char* const SetSceneStateAction = "setSceneState";
const char* const RunCloudAPIAction = "cloudAPI";
}  // namespace ActionNames

namespace DeviceGroup {
const char* const Id = "id";
const char* const Name = "name";
const char* const Categories = "categories";
const char* const Devices = "devices";
const char* const Exceptions = "exceptions";
const char* const Category = "category";
const char* const Subcategories = "subcategories";
const char* const Persistent = "persistent";
const char* const Sync = "sync";
const char* const Ids = "ids";
const char* const DeviceId = "deviceId";
const char* const Subcategory = "subcategory";
const char* const DeviceGroupIds = "deviceGroupIds";
const char* const DeviceGroupId = "deviceGroupId";
const char* const DeviceGroups = "deviceGroups";
}  // namespace DeviceGroup

namespace ItemGroup {
const char* const Id = "id";
const char* const Name = "name";
const char* const HasGetter = "hasGetter";
const char* const HasSetter = "hasSetter";
const char* const Persistent = "persistent";
const char* const ItemNames = "itemNames";
const char* const ValueType = "valueType";
const char* const Enum = "enum";
const char* const ValueTypeFamily = "valueTypeFamily";
const char* const Sync = "sync";
};  // namespace ItemGroup

namespace ScenesState {
const char* const Started = "started";
const char* const Finished = "finished";
const char* const PartiallyFinished = "partially_finished";
const char* const Failed = "failed";
const char* const Stopped = "stopped";
}  // namespace ScenesState

}  // namespace Resources

}  // namespace Model
}  // namespace SceneEngine
