#include "scene_scale_resources.h"

namespace HubPlatform {
namespace UT {
namespace SceneScale {

Json testScene = R"(
{
    "_id": "5fbd0a562231401412538a12",
    "enabled": true,
    "is_group": false,
    "name": "Thermostat.001",
    "parent_id": "",
    "then": [
        {
            "blockOptions": {
                "method": {
                    "args": {
                        "item": "item",
                        "value": "value"
                    },
                    "name": "setItemValue"
                }
            },
            "blockType": "then",
            "fields": [
                {
                    "name": "item",
                    "type": "item",
                    "value": "5fbd01d922314013c9b37c9f"
                },
                {
                    "name": "value",
                    "type": "float",
                    "value": 23
                }
            ]
        }
    ],
    "user_notifications": [],
    "when": [
        {
            "blockOptions": {
                "method": {
                    "args": {
                        "blocks": "blocks"
                    },
                    "name": "or"
                }
            },
            "blockType": "when",
            "fields": [
                {
                    "name": "blocks",
                    "type": "blocks",
                    "value": [
                        {
                            "blockOptions": {
                                "method": {
                                    "args": {
                                        "comparator": "comparator",
                                        "item": "item",
                                        "value": "value"
                                    },
                                    "name": "compareNumbers"
                                }
                            },
                            "blockType": "when",
                            "fields": [
                                {
                                    "name": "item",
                                    "type": "item",
                                    "value": "5fbd01d922314013c9b37c9b"
                                },
                                {
                                    "name": "comparator",
                                    "type": "string",
                                    "value": "<"
                                },
                                {
                                    "name": "value",
                                    "type": "float",
                                    "value": 22
                                }
                            ]
                        },
                        {
                            "blockOptions": {
                                "method": {
                                    "args": {
                                        "item": "item",
                                        "value": "value"
                                    },
                                    "name": "isItemState"
                                }
                            },
                            "blockType": "when",
                            "fields": [
                                {
                                    "name": "item",
                                    "type": "item",
                                    "value": "5fbd01d922314013c9b37c9c"
                                },
                                {
                                    "name": "value",
                                    "type": "token",
                                    "value": "heat"
                                }
                            ]
                        }
                    ]
                }
            ]
        }
    ]
}
)"_json;

/* Divide the large scene on sub-blocks */

std::string emptyScene = R"({
    "_id": "5fbd0a562231401412538a12",
    "enabled": true,
    "is_group": false,
    "name": "__SCENE_NAME__",
    "parent_id": "",
    "then": __THEN_BLOCK_ARRAY__,
    "user_notifications": [],
    "when": __WHEN_BLOCK_ARRAY__
})";

std::string thenBlockSetItemValue = R"([
{
    "blockOptions": {
        "method": {
            "args": {
                "item": "item",
                "value": "value"
            },
            "name": "setItemValue"
        }
    },
    "blockType": "then",
    "fields": [
        {
            "name": "item",
            "type": "item",
            "value": "__ITEM_ID__"
        },
        {
            "name": "value",
            "type": "__VALUE_TYPE__",
            "value": __COMPOUND_VALUE__
        }
    ]
}
])";

std::string anyOperatorBlock = R"({
    "blockOptions": {
        "method": {
            "args": {
                "blocks": "blocks"
            },
            "name": "__OPERATOR_NAME__"
        }
    },
    "blockType": "when",
    "fields": [
        {
            "name": "blocks",
            "type": "blocks",
            "value": __WHEN_BLOCK_ARRAY__
        }
    ]
}
)";

std::string compareNumbersBlock = R"({
    "blockOptions": {
        "method": {
            "args": {
                "comparator": "comparator",
                "item": "item",
                "value": "value"
            },
            "name": "compareNumbers"
        }
    },
    "blockType": "when",
    "fields": [
        {
            "name": "item",
            "type": "item",
            "value": "__ITEM_ID__"
        },
        {
            "name": "comparator",
            "type": "string",
            "value": "__COMPARATOR__"
        },
        {
            "name": "value",
            "type": "__VALUE_TYPE__",
            "value": __COMPOUND_VALUE__
        }
    ]
})";

std::string isItemStateBlock = R"(
{
    "blockOptions": {
        "method": {
            "args": {
                "item": "item",
                "value": "value"
            },
            "name": "isItemState"
        }
    },
    "blockType": "when",
    "fields": [
        {
            "name": "item",
            "type": "item",
            "value": "__ITEM_ID__"
        },
        {
            "name": "value",
            "type": "__VALUE_TYPE__",
            "value": __COMPOUND_VALUE__
        }
    ]
}
)";

std::string compareNumberRangeBlock = R"({
    "blockOptions": {
        "method": {
            "args": {
                "comparator": "comparator",
                "item": "item",
                "startValue": "startValue",
                "endValue": "endValue"
            },
            "name": "compareNumberRange"
        }
    },
    "blockType": "when",
    "fields": [
        {
            "name": "item",
            "type": "item",
            "value": "__ITEM_ID__"
        },
        {
            "name": "comparator",
            "type": "string",
            "value": "__COMPARATOR__"
        },
        {
            "name": "startValue",
            "type": "__START_VALUE_TYPE__",
            "value": __START_COMPOUND_VALUE__
        },
        {
            "name": "endValue",
            "type": "__END_VALUE_TYPE__",
            "value": __END_COMPOUND_VALUE__
        }
    ]
})";

Json itemSetPointHeating = R"({
    "_id": "5fbd01d922314013c9b37c9f",
    "deviceId": "5fbd01d922314013c9b37c95",
    "enum": [],
    "hasGetter": true,
    "hasSetter": true,
    "name": "thermostat_setpoint_heating",
    "show": true,
    "type": "temperature",
    "value": {"scale": "fahrenheit","value": 71},
    "valueMax": {"scale": "celsius","value": 33},
    "valueMin": {"scale": "celsius","value": 0}
})"_json;

Json itemThermostatMode = R"({
    "_id": "5fbd01d922314013c9b37c9c",
    "deviceId": "5fbd01d922314013c9b37c95",
    "enum": ["off","heat","cool","auto","energy_saving_heat","energy_saving_cool"],
    "hasGetter": true,
    "hasSetter": true,
    "name": "thermostat_mode",
    "show": true,
    "type": "token",
    "value": "heat"
})"_json;

Json itemThermostatSetPoint = R"({
    "_id": "5fbd01d922314013c9b37c9b",
    "deviceId": "5fbd01d922314013c9b37c95",
    "enum": [],
    "hasGetter": true,
    "hasSetter": true,
    "name": "thermostat_setpoint",
    "show": false,
    "type": "temperature",
    "value": {"scale": "celsius","value": 0},
    "valueMax": {"scale": "celsius","value": 45},
    "valueMin": {"scale": "celsius","value": -2}
})"_json;

}  // namespace SceneScale
}  // namespace UT
}  // namespace HubPlatform