#include "common_test_resources/resources/uid_resources.h"

namespace HubPlatform {
namespace Tests {

const std::string hubFavoriteSet = R"(
{
    "method": "hub.favorite.set",
    "id": "_ID_",
    "params": {
        "devices": [
            {
                "_id": "__DEVICE_ID_1__",
                "favorite": __IS_FAVORITE_DEVICE_1__
            },
            {
                "_id": "__DEVICE_ID_2__",
                "favorite": __IS_FAVORITE_DEVICE_2__
            }
        ],
        "items": [
            {
                "_id": "__ITEM_ID_1__",
                "favorite": __IS_FAVORITE_ITEM_1__
            },
            {
                "_id": "__ITEM_ID_2__",
                "favorite": __IS_FAVORITE_ITEM_2__
            }
        ],
        "rules": [
            {
                "_id": "__RULE_ID_1__",
                "favorite": __IS_FAVORITE_RULE_1__
            },
            {
                "_id": "__RULE_ID_2__",
                "favorite": __IS_FAVORITE_RULE_2__
            }
        ]
    }
}
)";

const std::string hubFavoriteDeviceSet = R"(
{
    "method": "hub.favorite.set",
    "id": "_ID_",
    "params": {
        "devices": [
            {
                "_id": "__DEVICE_ID__",
                "favorite": __IS_FAVORITE_DEVICE__
            }
        ]
    }
}
)";

const std::string hubFavoriteItemSet = R"(
{
    "method": "hub.favorite.set",
    "id": "_ID_",
    "params": {
        "items": [
            {
                "_id": "__ITEM_ID__",
                "favorite": __IS_FAVORITE_ITEM__
            }
        ]
    }
}
)";

const std::string hubFavoriteRuleSet = R"(
{
    "method": "hub.favorite.set",
    "id": "_ID_",
    "params": {
        "rules": [
            {
                "_id": "__RULE_ID__",
                "favorite": __IS_FAVORITE_RULE__
            }
        ]
    }
}
)";

const std::string hubFavoriteList = R"(
{
    "method": "hub.favorite.list",
    "id": "_ID_",
    "params": {
        "request": [
            "__DEVICES__",
            "__ITEMS__",
            "__RULES__"
        ]
    }
}
)";

const std::string hubItemsList = R"(
{
    "method": "hub.items.list",
    "id": "_ID_",
    "params": {
        "deviceIds": __DEVICES__
    }
}
)";

const std::string hubCustomNodesRequest = R"(
{
    "method": "hub.software.update.execute",
    "id": "_ID_",
    "params": {
        __ACTIONS__
    }
}
)";

const std::string hubCustomNodesInstallUpgrade = R"(
{
    "method": "hub.software.update.execute",
    "id": "_ID_",
    "params": {
        "actions": [
            {
                "operation": __OPERATION__,
                "source": __SOURCE__,
                "type": __TYPE__,
                "id": __ID__,
                "version": __VERSION__,
                "archive_hash_md5": __HASH__
            }
        ]
    }
}
)";

const std::string hubCustomNodesRemove = R"(
{
    "method": "hub.software.update.execute",
    "id": "_ID_",
    "params": {
        "actions": [
            {
                "operation": __OPERATION__,
                "type": __TYPE__,
                "id": __ID__
            }
        ]
    }
}
)";

const std::string hubCustomNodesList = R"(
{
    "method": "hub.custom.nodes.list",
    "id": "_ID_",
    "params": {
         "ids": __IDS__,
         "name": __NAME__,
         "author": __AUTH__,
         "showMeta": __SHOW_META__,
         "showAdvanced": __SHOW_ADV__,
         "showInterface": __SHOW_INT__
    }
}
)";

const std::string hubScenesDataList = R"(
{
    "method": "hub.scenes.block.data.list",
        "params": {
            "valueTypes": __VAL_TYPES__,
            "scalableValueTypes": __SCALABLE_VAL_TYPES__,
            "valueScales": __VAL_SCALES__,
            "scenesValueTypes": __SCENES_VAL_TYPES__,
            "valueTypeFamilies": __VAL_TYPE_FAMILIES__,
            "comparisonOperators": __COMP_OP__,
            "comparisonMethods": __COMP__METH__,
            "actionMethods": __ACTION__METH__,
            "advancedScenesVersion": __VER__
        }
}
)";

const std::string hubItemValuePercentApply = R"(
{
    "method": "hub.item.value.percent.apply",
    "id": "_ID_",
    "params": {
        "_id": "__ITEMID__",
        "percent": __PERCENT__
    }
}
)";

const std::string hubDataList = R"(
{
    "method": "hub.data.list",
    "id": "_ID_",
    "params": {
    }
}
)";

}  // namespace Tests
}  // namespace HubPlatform
