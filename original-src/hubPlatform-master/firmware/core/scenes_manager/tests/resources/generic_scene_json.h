#pragma once
#include "jsonrpc/jsonrpc.h"

namespace SceneEngine {
namespace UT {

extern JsonObject emptyScene;
extern JsonObject intervalCondition;
extern JsonObject sunstateCondition;
extern JsonObject sunstateConditionRange;
extern JsonObject dateCondition;
extern JsonObject onceCondition;
extern JsonObject itemEventExpressionCondition;
extern JsonObject compareNumbersEventExpressionCondition;
extern JsonArray whenBlockArray;
extern JsonArray wrongWhenBlockArray;
extern JsonObject compareNumbersRangeCondition;
extern std::string compareOperatorWhenBlockTemplate;
extern std::string compareValuesWhenBlockTemplate;
extern std::string itemOperationWhenBlockTemplate;
extern std::string isItemStateChangedWhenBlockTemplate;
extern std::string isDateWeeksWhenBlockTemplate;
extern std::string isDateWithTimeBlockTemplate;
extern JsonObject firmwareUpdateStateCondition;
extern JsonObject sceneWithExceptionsInActions;
extern JsonObject sceneWithInnerRelationsInActions;
extern JsonObject editedSceneWithInnerRelationsInActions;

extern const JsonObject setItemValueAction;
extern const JsonObject setItemValueActionForTemperature;
extern const JsonObject setDeviceArmed;
extern const JsonArray thenBlockArray;
extern const JsonArray thenBlockEmptyBlockOptions;
extern const JsonArray thenBlockEmptyMethod;
extern const JsonArray thenBlockEmptyName;
extern const JsonObject hubBackupRunAction;
extern const JsonObject hubForwardNotificationAction;

extern const JsonObject sceneBlocksTemplates;
extern const std::vector< JsonObject > itemsList;
extern const std::vector< JsonObject > tokenItemsList;
extern const std::vector< JsonObject > optionItemsList;
extern const std::vector< JsonObject > switchItemsList;
extern const std::vector< JsonObject > motionItemsList;

extern const JsonObject createScene;
extern const JsonObject createScene1;
extern const JsonObject createScene2;
extern const JsonObject createScene3;
extern const JsonObject createScene3Expected;
extern const JsonObject createSceneForStatusGetRequest;
extern const JsonObject createSceneForStatusGetExpectedResult;
extern const JsonObject editedScene;
extern const JsonObject createSceneWithScalesItemState;
extern const JsonObject createSceneWithScalesCompareNumbers;

extern const std::string isDictionaryChangedWhenBlockTemplate;
extern const std::string isDetectedInHotzoneWhenBlockTemplate;

extern const std::string blockReferenceExceptionTemplate;
extern const std::string externalBlockReferenceExceptionTemplate;
extern const std::string expressionReferenceExceptionTemplate;

extern const std::string functionOperatorWhenBlock;
extern const std::string logicalOperatorWhenBlock;

extern const JsonObject createSceneWithGlobalRestriction;

}  // namespace UT
}  // namespace SceneEngine
