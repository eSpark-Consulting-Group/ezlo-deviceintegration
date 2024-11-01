#pragma once

namespace SceneEngine {
namespace Model {

class EventName {
public:
    enum class Enum {
        isBatteryLevel,
        isBatteryState,
        isButtonState,
        isCloudState,
        compareNumberRange,
        compareNumbers,
        compareStrings,
        stringOperation,
        compareValues,
        inArray,
        isDate,
        isDateRange,
        isDictionaryChanged,
        isFirmwareUpdateState,
        isGroupState,
        isHouseModeChanged,
        isItemState,
        isDeviceState,
        isItemStateChanged,
        isNetworkState,
        isOnce,
        isSceneState,
        isSunState,
        isUserLockOperation,
        hasAtLeastOneDictionaryValue,
        isDeviceItemGroup,
        isFeatureState,

        // Add all fields before this one
        Count,
        None
    };
};

}  // namespace Model
}  // namespace SceneEngine