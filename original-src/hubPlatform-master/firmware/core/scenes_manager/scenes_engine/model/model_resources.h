#pragma once
// model_resources.h
#include <string>
#include <vector>
#include <map>

namespace SceneEngine {
namespace Model {

struct BinaryOperation {
    std::string name;
    std::string op;
    std::string method;
};

extern std::vector< std::string > ScenesValueTypes;

bool isLogicOperator( const std::string& methodName );
bool isOperatorNot( const std::string& methodName );
bool isOperatorAnd( const std::string& methodName );
bool isOperatorOr( const std::string& methodName );
bool isOperatorXor( const std::string& methodName );
bool isOperatorFunction( const std::string& methodName );

namespace Resources {

namespace MethodNames {
extern const char* const isBatteryLevel;
extern const char* const isBatteryState;
extern const char* const isButtonState;
extern const char* const isCloudState;
extern const char* const compareNumberRange;
extern const char* const compareNumbers;
extern const char* const compareStrings;
extern const char* const stringOperation;
extern const char* const inArray;
extern const char* const compareValues;
extern const char* const isDate;
extern const char* const isDateRange;
extern const char* const isInterval;
extern const char* const isDictionaryChanged;
extern const char* const isDetectedInHotzone;
extern const char* const isFirmwareUpdateState;
extern const char* const isGroupState;
extern const char* const isHouseModeChanged;
extern const char* const isItemState;
extern const char* const isDeviceItemGroup;
extern const char* const isItemStateChanged;
extern const char* const isNetworkState;
extern const char* const isOnce;
extern const char* const isSceneState;
extern const char* const isSunState;
extern const char* const isUserLockOperation;
extern const char* const hasAtLeastOneDictionaryValue;
extern const char* const isHouseModeChangedTo;
extern const char* const isHouseModeChangedFrom;
extern const char* const isDeviceState;
extern const char* const IsHouseModeAlarmPhaseRange;
extern const char* const IsHouseModeAlarmSwitchToRange;
extern const char* const isFeatureState;
}  // namespace MethodNames

// If you need to add a new block operator, add it to isLogicOperator(string) function also
namespace OperatorNames {
extern const char* const AndOperator;
extern const char* const NotOperator;
extern const char* const OrOperator;
extern const char* const XorOperator;
extern const char* const FunctionOperator;
}  // namespace OperatorNames

namespace FunctionFields {
extern const char* const For;
extern const char* const ForLess;
extern const char* const ForAtLeast;
extern const char* const Method;
extern const char* const Seconds;
extern const char* const Repeat;
extern const char* const Times;
extern const char* const Follow;
extern const char* const DelayReset;
extern const char* const Pulse;
extern const char* const TruePeriod;
extern const char* const FalsePeriod;
extern const char* const Latch;
extern const char* const Enabled;
}  // namespace FunctionFields

namespace StringOperations {
extern const char* const Begin;
extern const char* const End;
extern const char* const Contain;
extern const char* const Length;
extern const char* const NotBegin;
extern const char* const NotEnd;
extern const char* const NotContain;
extern const char* const NotLength;
}  // namespace StringOperations

namespace HotzoneEventFields {
extern const char* const Persons;
extern const char* const Objects;
extern const char* const QRCodes;
}  // namespace HotzoneEventFields

namespace ValueComparators {
extern const char* const Equal;
extern const char* const NotEqual;
extern const char* const Greater;
extern const char* const Less;
extern const char* const GreaterEqual;
extern const char* const LessEqual;
extern const char* const Between;
extern const char* const NotBetween;
}  // namespace ValueComparators

namespace Labels {
extern const char* const Equal;
extern const char* const NotEqual;
extern const char* const Greater;
extern const char* const Less;
extern const char* const GreaterEqual;
extern const char* const LessEqual;
extern const char* const Between;
extern const char* const NotBetween;
extern const char* const Begin;
extern const char* const End;
extern const char* const Contain;
extern const char* const Length;
extern const char* const NotBegin;
extern const char* const NotEnd;
extern const char* const NotContain;
extern const char* const NotLength;
extern const char* const In;
extern const char* const NotIn;
}  // namespace Labels

namespace ArrayOperations {
extern const char* const In;
extern const char* const NotIn;
}  // namespace ArrayOperations

namespace DataSources {
extern const char* const Item;
extern const char* const Device;
extern const char* const DeviceItem;
extern const char* const Constant;
extern const char* const Expression;
extern const char* const DeviceGroup;
extern const char* const ItemGroup;
}  // namespace DataSources

namespace EventFields {
extern const char* const Start;
extern const char* const Finish;
}  // namespace EventFields

namespace ActionNames {
extern const char* const RunCustomScriptAction;
extern const char* const RunPluginScriptAction;
extern const char* const RunSceneAction;
extern const char* const StopSceneAction;
extern const char* const SetSceneStateAction;
extern const char* const RunCloudAPIAction;
}  // namespace ActionNames

namespace DeviceGroup {
extern const char* const Id;
extern const char* const Name;
extern const char* const Categories;
extern const char* const Devices;
extern const char* const Exceptions;
extern const char* const Category;
extern const char* const Subcategories;
extern const char* const Persistent;
extern const char* const Sync;
extern const char* const Ids;
extern const char* const DeviceId;
extern const char* const Subcategory;
extern const char* const DeviceGroupIds;
extern const char* const DeviceGroupId;
extern const char* const DeviceGroups;
}  // namespace DeviceGroup

namespace ItemGroup {
extern const char* const Id;
extern const char* const Name;
extern const char* const HasGetter;
extern const char* const HasSetter;
extern const char* const Persistent;
extern const char* const ItemNames;
extern const char* const ValueType;
extern const char* const Enum;
extern const char* const ValueTypeFamily;
extern const char* const Sync;
}  // namespace ItemGroup

namespace ScenesState {
extern const char* const Started;
extern const char* const Finished;
extern const char* const PartiallyFinished;
extern const char* const Failed;
extern const char* const Stopped;
}  // namespace ScenesState

}  // namespace Resources

}  // namespace Model
}  // namespace SceneEngine
