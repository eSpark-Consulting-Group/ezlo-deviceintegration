#pragma once

#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Tests {

extern const JsonObject createSceneJson;
extern const JsonObject cloneSceneJson;
extern const JsonObject createEmptyWhenBlockSceneJson;
extern const JsonObject createSunsetScene;
extern const JsonObject createPeriodicalScene;
extern const JsonObject createInterval1HourScene;
extern const JsonObject createDailyScene;
extern const JsonObject createWeeklyScene;
extern const JsonObject createMonthlyScene;
extern const JsonObject createSetArmedDeviceAction;
extern const JsonObject createSetItemValueAction;
extern const JsonObject createSetItemValueMulticastAction;
extern const JsonObject createEmptyScene;
extern const JsonObject createHouseModeScene;
extern const JsonObject createIsSunStateDailyScene;
extern const JsonObject createIsSunStateWeeklyScene;
extern const JsonObject createIsSunStateMonthlyScene;
extern const JsonObject createNotificationScene;
extern const JsonObject createIsOnceScene;
extern const JsonObject createActionDelay;
extern const JsonObject createIsItemState;
extern const JsonObject createIsItemStateMultipleItems;
extern const JsonObject createIsItemStateNotArmedDevices;
extern const JsonObject createCompareNumbersScene;
extern const JsonObject createCompareNumbersScalableScene;
extern const JsonObject createIsButtonStateScene;
extern const JsonObject createSceneWithMetaInParamsRequest;
extern const JsonObject createSceneWithMetaInParamsEditSceneRequest;

extern const JsonObject editScene;
extern const JsonObject getScenesList;
extern const JsonObject getScene;
extern const JsonObject deleteScene;
extern const JsonObject getTimeScenesList;
extern const JsonObject getScenesBlocksList;
extern const JsonObject getScenesTriggerDevicesList;

extern const JsonObject setSceneRoom;
extern const JsonObject setGeoCoordinates;
extern const JsonObject runSceneManually;
extern const JsonObject addSceneNotification;
extern const JsonObject removeSceneNotification;
extern const JsonObject enableScene;
extern const JsonObject disableScene;

extern const JsonObject EFWR_1561_createScene;
extern const JsonObject EFWR_1561_editScene;

extern const JsonObject EFWR_1580_createScene;
extern const JsonObject EFWR_1580_editScene;
extern const std::string EFWR_2195_editSceneRequest;

extern const JsonObject EFWR_3664_createScene;

extern const JsonObject createSameActionsDifferentDelays;
extern const std::string compareNumbersRangeConditionBlock;
extern const std::string isDeviceItemGroupEvent;

extern const std::string createSceneRequest;
extern const std::string createSceneWithExceptionsRequest;
extern const std::string deleteSceneRequest;
extern const std::string runSceneRequest;
extern const std::string stopSceneRequest;
extern const std::string editScenesRequest;
extern const std::string editSceneWithExceptionsRequest;

extern const std::string setEnableSceneRequest;
extern const std::string setIconRequest;
extern const std::string setNameRequest;
extern const std::string setMetaRequest;
extern const std::string setBlockMetaRequest;
extern const std::string setHouseModesRequest;
extern const std::string setBlockEnabledSetRequest;
extern const std::string testActionBlock;

extern const std::string isItemStateWhenBlock;
extern const std::string isItemStateChangedWhenBlock;
extern const std::string isItemStateOptionsWhenBlock;
extern const std::string isButtonStateWhenBlock;
extern const std::string compareNumbersWhenBlock;
extern const std::string compareStringsWhenBlock;
extern const std::string itemOperationWhenBlock;
extern const std::string compareValuesWhenBlock;
extern const std::string isOnceSceneWhenBlock;
extern const std::string isDateDailyWhenBlock;
extern const std::string isDateWithTimeWhenBlock;
extern const std::string isDateWeeksWhenBlockTemplate;
extern const std::string isIntervalWhenBlock;
extern const std::string isSunStateWhenBlock;
extern const std::string isUserLockOperationWhenBlock;
extern const std::string isUserLockOperationWhenBlockWithUserCodeAction;
extern const std::string isDateRangeWhenBlock;
extern const std::string isHouseModeChangedToWhenBlock;
extern const std::string isHouseModeChangedFromWhenBlock;
extern const std::string isHouseModeAlarmedRangeWhenBlock;
extern const std::string isHouseModeSwitchToRangeWhenBlock;
extern const std::string isNetworkStateWhenBlock;
extern const std::string isSceneStateWhenBlock;
extern const std::string isFeatureStateWhenBlock;
extern const std::string isFeatureStateWithoutStatusWhenBlock;
extern const std::string isFeatureStateWithoutVersionWhenBlock;
extern const std::string isCloudStateWhenBlock;
extern const std::string hasAtLeastOneDictionaryValueWhenBlock;
extern const std::string isDictionaryChangedWhenBlockTemplate;
extern const std::string isDetectedInHotzoneWhenBlockTemplate;
extern const std::string andOperatorWhenBlock;
extern const std::string notOperatorWhenBlock;
extern const std::string orOperatorWhenBlock;
extern const std::string xorOperatorWhenBlock;
extern const std::string checkBatteryConditionBlock;
extern const std::string checkBatteryLevelConditionBlock;
extern const std::string isGroupStateWhenBlock;
extern const std::string isDeviceStateWhenBlock;
extern const std::string isDeviceStateWhenBlockArmed;
extern const std::string isDeviceStateWhenBlockReachable;
extern const std::string isDeviceStateWhenBlockFailed;
extern const std::string isDeviceStateWhenBlockFailedDevice;

extern const std::string sendCloudAbstractCommandThenBlock;
extern const std::string setItemValueThenBlock;
extern const std::string setItemValueThenBlockWithScale;
extern const std::string groupSetItemValueThenBlock;
extern const std::string groupToggleValueThenBlock;
extern const std::string groupSetDeviceArmedThenBlock;
extern const std::string switchHouseModeThenBlock;
extern const std::string sendHttpRequestThenBlock;
extern const std::string sendHttpRequestSaveResultThenBlock;
extern const std::string runSceneThenBlock;
extern const std::string stopSceneThenBlock;
extern const std::string stopSceneElseBlock;
extern const std::string runSceneThenGroupsBlock;
extern const std::string setSceneStateThenBlock;
extern const std::string resetLatchThenBlock;
extern const std::string resetSceneLatchesThenBlock;
extern const std::string runPluginScriptThenBlock;

extern const std::string addScriptRequest;
extern const std::string scenesScriptsGetRequest;
extern const std::string deleteScriptRequest;

extern const std::string blockReferenceExceptionBlock;
extern const std::string externalBlockReferenceExceptionBlock;
extern const std::string expressionReferenceExceptionBlock;
extern const std::string setExpressionCommand;

extern const std::string createGlobalSceneRequest;
extern const std::string deleteGlobalSceneRequest;
extern const std::string getGlobalScenesListRequest;
extern const std::string editGlobalScenesRequest;
extern const std::string enableGlobalSceneRequest;
extern const std::string runGlobalSceneRequest;

extern const std::string globalIsItemStateWhenBlock;
extern const std::string globalIsItemArmedStateWhenBlock;
extern const std::string globalIsButtonStateWhenBlock;
extern const std::string globalHasAtLeastOneDictionaryValueWhenBlock;

extern const std::string globalSetItemValueThenBlock;
extern const std::string globalSetDeviceArmed;

extern const std::string addCustomScriptRequest;
extern const std::string deleteCustomScriptRequest;
extern const std::string runCustomScriptThenBlock;
extern const std::string softwareUpdateConditionBlock;
extern const std::string setVariableExpression;
extern const std::string cloudAPIAction;
extern const std::string cloudAPIActionSaveResultThenBlock;
extern const std::string functionOperatorWhenBlock;
extern const std::string setExpression;
extern const std::string setExpressionWithParams;

extern const std::string setValueActonBlock;

extern const std::string setExpressionWithError;

extern const std::string toggleExpressionValueThenBlock;
extern const std::string toggleItemValueThenBlock;
extern const std::string toggleItemValueDelayedThenBlock;

extern const std::string getSceneStatusRequest;

extern const JsonObject cloudAccessKeysSyncResponseTemplate;

extern const std::string scenesDeviceGroupCreate;
extern const std::string scenesPersistentDeviceGroupCreate;
extern const std::string scenesDeviceGroupDelete;
extern const std::string scenesDeviceGroupUpdate;
extern const std::string deviceGroupsList;
extern const std::string deviceGroupGet;
extern const std::string deviceGroupFind;
extern const std::string scenesItemGroupCreate;
extern const std::string scenesItemGroupCreateWithMultipleItemNames;
extern const std::string scenesItemGroupDelete;
extern const std::string scenesPersistentItemGroupCreate;
extern const std::string scenesItemGroupUpdate;
extern const std::string itemGroupsList;
extern const std::string itemGroupGet;
extern const std::string scenesDeviceGroupCreateWithDevices;
extern const std::string scenesDeviceGroupCreateWithDevicesAndDelay;
extern const std::string isItemStateWithSourceWhenBlock;
extern const std::string isDeviceStateWithSourceWhenBlock;
extern const std::string deviceItemGroupExpand;
extern const std::string deviceGroupExpand;

extern const std::string sceneEventFieldsSource;

extern const std::string forwardNotificationThenBlock;

}  // namespace Tests
}  // namespace HubPlatform
