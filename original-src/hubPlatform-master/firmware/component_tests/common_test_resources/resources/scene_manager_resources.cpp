#include "common_test_resources/resources/scene_manager_resources.h"

namespace HubPlatform {
namespace Tests {

const JsonObject createSceneJson = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "newTestRule1",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [
            {
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time",
                        "weekdays":"weekdays"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "weekly"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "9:15", "00:17", "19:40", "11:20"  ]
                },
                {
                    "name": "weekdays",
                    "type": "int_array",
                    "value": [ 1, 2, 6, 7, 5 ]
                }
            ]
            }
        ]
    }
}
)"_json;

const JsonObject createEmptyWhenBlockSceneJson = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "newTestRule1",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : []
    }
}
)"_json;

const JsonObject createSceneWithMetaInParamsRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "newTestRule1",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [],
        "meta" : {
            "key": "value"
        }
}
}
)"_json;

const JsonObject createSceneWithMetaInParamsEditSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params": {
        "_id": "__SCENE_ID__",
        "eo": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "newTestRule1",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [],
        "meta" : {
            "key": "value"
        }
      }
    }
}
)"_json;

const JsonObject createSunsetScene = R"(
{
  "method": "hub.scenes.create",
  "params": {
    "enabled": true,
    "house_modes": [
      "1",
      "2",
      "3",
      "4"
    ],
    "is_group": false,
    "name": "S58",
    "parent_id": "",
    "then": [],
    "user_notifications": [
      "1810131"
    ],
    "when": [
      {
        "blockOptions": {
          "method": {
            "args": {
              "sunstate": "sunset",
              "time": "time",
              "days": "days"
            },
            "name": "isSunState"
          }
        },
        "blockType": "when",
        "fields": [
          {

            "name": "days",
            "type": "int_array",
             "value": [
              22
            ]
          },
          {
            "name": "time",
            "type": "hms_interval",
            "value": "00:05:00"
          },
          {

            "name": "sunset",
            "type": "string",
        "value": "intime"
      }
        ]
      }
    ]
  },
  "id": "67092324@hub.scenes.create.method.websocket.api",
  "jsonrpc": "2.0"
}
)"_json;

const JsonObject createPeriodicalScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "IntervalScene",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [
            {
               "blockOptions":{
                  "method":{
                     "args":{
                        "interval":"interval"
                     },
                     "name":"isInterval"
                  }
               },
               "blockType":"when",
               "fields":[
                  {
                     "name":"interval",
                     "type":"interval",
                     "value":"2s"
                  }
               ]
            }
        ]
    }
}
)"_json;

extern const JsonObject createInterval1HourScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "IntervalScene",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [
            {
               "blockOptions":{
                  "method":{
                     "args":{
                        "interval":"interval"
                     },
                     "name":"isInterval"
                  }
               },
               "blockType":"when",
               "fields":[
                  {
                     "name":"interval",
                     "type":"interval",
                     "value":"1h"
                  }
               ]
            }
        ]
    }
}
)"_json;

const JsonObject createDailyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "DailyEvent",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "daily"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "12:45", "23:17" ]
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createWeeklyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "WeeklyEvent",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time",
                        "weekdays":"weekdays"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "weekly"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "10:20", "12:40" ]
                },
                {
                    "name": "weekdays",
                    "type": "int_array",
                    "value": [ 2, 5 ]
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createMonthlyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "MonthlyEvent",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type": "type",
                        "time":"time",
                        "days":"days"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "monthly"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "10:20", "12:40" ]
                },
                {
                    "name": "days",
                    "type": "int_array",
                    "value": [ 5, 12 ]
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createSetArmedDeviceAction = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetArmedDeviceAction",
        "parent_id": "5c6ec961cc01eb07f86f9dd9",
        "then" : [{
            "blockOptions": {
                "method":{
                    "args":{
                        "device":"device",
                        "deviceFlag":"deviceFlag"
                    },
                    "name":"setDeviceArmed"
                }
            },
            "blockType":"then",
            "fields":[
                {
                    "name":"device",
                    "type":"device",
                    "value" : "5d287f77f587be6495c2d178"
                },
                {
                    "name":"deviceFlag",
                    "type":"bool",
                    "value": false
                }
            ]
        }],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "daily"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "12:45", "23:17" ]
                }
            ]
        }]
    }
}
)"_json;

extern const JsonObject createSetItemValueAction = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetItemValue",
        "then" : [
            {
                "blockOptions":{
                    "method":{
                        "args":{
                            "item":"item",
                            "value": "value"
                        },
                        "name":"setItemValue"
                    }
                },
                "blockType":"then",
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value" : "__ITEM_ID__"
                    },
                    {
                        "name": "value",
                        "type": "__ITEM_VALUE_TYPE__",
                        "value": 12345
                    }
                ]
            }
        ],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "daily"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "12:45", "23:17" ]
                }
            ]
        }]
    }
}
)"_json;

extern const JsonObject createSetItemValueMulticastAction = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "SetItemValue",
        "then" : [
            {
                "blockOptions":{
                    "method":{
                        "args":{
                            "item":"item",
                            "value": "value"
                        },
                        "name":"setItemValue"
                    }
                },
                "blockType":"then",
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value" : "__ITEM_ID1__"
                    },
                    {
                        "name": "value",
                        "type": "int",
                        "value": 12345
                     }
                ]
            },
            {
                "blockOptions":{
                    "method":{
                        "args":{
                            "item":"item",
                            "value": "value"
                        },
                        "name":"setItemValue"
                    }
                },
                "blockType":"then",
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value" : "__ITEM_ID2__"
                    },
                    {
                        "name": "value",
                        "type": "int",
                        "value": 54321
                     }
                ]
            }
        ],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "daily"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "12:45", "23:17" ]
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createEmptyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "EmptyScene",
        "parent_id": "",
        "then" : [],
        "when" : []
    }
}
)"_json;

const JsonObject createHouseModeScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "HouseModes",
        "house_modes" : [
            "2",
            "4"
        ],
        "then" : [],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "daily"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "12:45", "23:17" ]
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createIsSunStateDailyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": null,
        "is_group": false,
        "name": "isSunState",
        "then" : [],
        "when" : [
            {
               "blockOptions":{
                  "method":{
                     "args":{
                        "sunstate":"__SUNSTATE__",
                        "range":"range",
                        "time":"time"
                     },
                     "name":"isSunState"
                  }
               },
               "blockType":"when",
               "fields":[
                {
                     "name":"__SUNSTATE__",
                     "type":"string",
                     "value":"__WHEN__"
                },
                {
                   "name":"time",
                   "type":"hms_interval",
                   "value":"__TIME__"
                },
                {
                   "name":"range",
                   "type":"token",
                   "value":"__RANGE_TYPE__"
                }
               ]
            }
        ]
    }
}
)"_json;

const JsonObject createIsSunStateWeeklyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": null,
        "is_group": false,
        "name": "isSunState",
        "then" : [],
        "when" : [
            {
               "blockOptions":{
                  "method":{
                     "args":{
                        "sunstate":"__SUNSTATE__",
                        "weekdays":"weekdays",
                        "time":"time"
                     },
                     "name":"isSunState"
                  }
               },
               "blockType":"when",
               "fields":[
                {
                     "name":"__SUNSTATE__",
                     "type":"string",
                     "value":"__WHEN__"
                },
                {
                     "name":"weekdays",
                     "type":"int_array",
                     "value": [ 1, 3, 4 ]
                },
                {
                   "name":"time",
                   "type":"hms_interval",
                   "value":"__TIME__"
                }
               ]
            }
        ]
    }
}
)"_json;

const JsonObject createIsSunStateMonthlyScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": null,
        "is_group": false,
        "name": "isSunState",
        "then" : [],
        "when" : [
            {
               "blockOptions":{
                  "method":{
                     "args":{
                        "sunstate":"__SUNSTATE__",
                        "days":"days",
                        "time":"time"
                     },
                     "name":"isSunState"
                  }
               },
               "blockType":"when",
               "fields":[
                {
                     "name":"__SUNSTATE__",
                     "type":"string",
                     "value":"__WHEN__"
                },
                {
                     "name":"days",
                     "type":"int_array",
                     "value": [ 1, 3, 4 ]
                },
                {
                   "name":"time",
                   "type":"hms_interval",
                   "value":"__TIME__"
                }
               ]
            }
        ]
    }
}
)"_json;

const JsonObject createNotificationScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "Notifications",
        "user_notifications": [
            "1111",
            "2222"
        ],
        "then" : [],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "type":"type",
                        "time":"time"
                    },
                    "name":"isDate"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "type",
                    "type": "string",
                    "value": "daily"
                },
                {
                    "name": "time",
                    "type": "24_hours_time_array",
                    "value": [ "12:45", "23:17" ]
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createIsOnceScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "IsOnce",
        "then" : [],
        "when" : [{
            "blockOptions":{
                "method":{
                    "args":{
                        "time": "time",
                        "day": "day",
                        "month": "month",
                        "year": "year"
                    },
                    "name":"isOnce"
                }
            },
            "blockType":"when",
            "fields":[
                {
                    "name": "time",
                    "type": "24_hours_time",
                    "value": "10:20"
                },
                {
                    "name": "day",
                    "type": "int",
                    "value": 5
                },
                {
                    "name": "month",
                    "type": "int",
                    "value": 12
                },
                {
                    "name": "year",
                    "type": "int",
                    "value": 2019
                }
            ]
        }]
    }
}
)"_json;

const JsonObject createActionDelay = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "ActionDelay",
        "then" : [
            {
                "blockOptions":{
                    "method":{
                        "args":{
                            "item":"item",
                            "value": "value"
                        },
                        "name":"setItemValue"
                    }
                },
                "delay" : {
                       "seconds": 3,
                       "minutes": 0,
                       "hours": 0,
                       "days": 0
                   },
                "blockType":"then",
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value" : "__ITEM_ID__"
                    },
                    {
                        "name": "value",
                        "type": "int",
                        "value": 12345
                     }
                ]
            }
        ],
        "when" : []
    }
}
)"_json;

const JsonObject createIsItemState = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "IsItemStateSimple",
        "then" : [],
        "when" : [
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value",
                        "armed":"armed"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID__"
                    },
                    {
                        "name":"value",
                        "type":"int",
                        "value": 10
                    },
                    {
                        "name":"armed",
                        "type":"bool",
                        "value":true
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject createIsItemStateMultipleItems = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "IsItemStateMultipleItems",
        "then" : [],
        "when" : [
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value",
                        "armed":"armed"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID1__"
                    },
                    {
                        "name":"value",
                        "type":"int",
                        "value": 10
                    },
                    {
                        "name":"armed",
                        "type":"bool",
                        "value":true
                    }
                ]
            },
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value",
                        "armed":"armed"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID2__"
                    },
                    {
                        "name":"value",
                        "type":"int",
                        "value": 10
                    },
                    {
                        "name":"armed",
                        "type":"bool",
                        "value":true
                    }
                ]
            },
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value",
                        "armed":"armed"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID3__"
                    },
                    {
                        "name":"value",
                        "type":"int",
                        "value": 10
                    },
                    {
                        "name":"armed",
                        "type":"bool",
                        "value":true
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject createIsItemStateNotArmedDevices = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "isItemStateNotArmedDevices",
        "then" : [],
        "when" : [
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID1__"
                    },
                    {
                        "name":"value",
                        "type":"bool",
                        "value": true
                    }
                ]
            },
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"isItemState",
                    "args":{
                        "item":"item",
                        "value":"value"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID2__"
                    },
                    {
                        "name":"value",
                        "type":"token",
                        "value": "single_user_code_deleted"
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject createCompareNumbersScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "CompareNumber",
        "then" : [],
        "when" : [
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"compareNumbers",
                    "args":{
                        "item":"item",
                        "comparator":"comparator",
                        "value":"value"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID1__"
                    },
                    {
                        "name":"comparator",
                        "type":"string",
                        "value": ">"
                    },
                    {
                        "name":"value",
                        "type":"int",
                        "value": 51
                    }
                ]
            },
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"compareNumbers",
                    "args":{
                        "item":"item",
                        "comparator":"comparator",
                        "value":"value"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID2__"
                    },
                    {
                        "name":"comparator",
                        "type":"string",
                        "value": ">"
                    },
                    {
                        "name":"value",
                        "type":"float",
                        "value": 35.55
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject createCompareNumbersScalableScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "CompareNumbersScalable",
        "then" : [],
        "when" : [
            {
                "blockType":"when",
                "blockOptions":{
                "method":{
                    "name":"compareNumbers",
                    "args":{
                        "item":"item",
                        "comparator":"comparator",
                        "value":"value"
                    }
                }
                },
                "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID1__"
                    },
                    {
                        "name":"comparator",
                        "type":"string",
                        "value": ">"
                    },
                    {
                        "name":"value",
                        "type":"__ITEM1_TYPE__",
                        "value": 11112345,
                        "scale": "__ITEM1_SCALE__"
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject createIsButtonStateScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "IsButtonState",
        "then" : [],
        "when" : [
            {
               "blockType":"when",
               "blockOptions":{
                  "method":{
                     "name":"isButtonState",
                     "args":{
                        "item":"item",
                        "value":"value"
                     }
                  }
               },
               "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID1__"
                    },
                    {
                        "name":"value",
                        "type":"token",
                        "value": "released"
                    }
               ]
            },
            {
               "blockType":"when",
               "blockOptions":{
                  "method":{
                     "name":"isButtonState",
                     "args":{
                        "item":"item",
                        "value":"value"
                     }
                  }
               },
               "fields":[
                    {
                        "name":"item",
                        "type":"item",
                        "value":"__ITEM_ID2__"
                    },
                    {
                        "name":"value",
                        "type":"token",
                        "value": "press_1_time"
                    }
               ]
            }
        ]
    }
}
)"_json;

const JsonObject editScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params" : {
        "_id": "__SCENE_ID__",
        "eo": "__NEW_SCENE__"
    }
}
)"_json;

const JsonObject getScenesList = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.list",
    "params": {}
}
)"_json;

const JsonObject getScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.get",
    "params": {
        "_id": "__SCENE_ID__"
    }
}
)"_json;

const JsonObject deleteScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.delete",
    "params": {
        "_id": "__SCENE_ID__"
    }
}
)"_json;

const JsonObject getTimeScenesList = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.time.list",
    "params": { }
}
)"_json;

const JsonObject getScenesBlocksList = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.blocks.list",
    "params": {
        "blockType": "when",
        "devices": [ "__DEVICE_ID__" ]
    }
}
)"_json;

const JsonObject getScenesTriggerDevicesList = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.trigger.devices.list",
    "params": {
    }
}
)"_json;

const JsonObject setSceneRoom = R"(
{
    "method": "hub.scenes.room.set",
    "id": "_ID_",
    "params": {
        "sceneId": "",
        "roomId": ""
    }
}
)"_json;

// Kharkiv City coordinates
const JsonObject setGeoCoordinates = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.coordinates.set",
    "params": {
          "latitude" :  49.9808100,
          "longitude" : 36.2527200
     }
}
)"_json;

const JsonObject runSceneManually = R"(
{
    "method": "hub.scenes.run",
    "id": "_ID_",
    "params": {
        "sceneId": "__ID__"
    }
}
)"_json;

const JsonObject addSceneNotification = R"(
{
    "method": "hub.scenes.notification.add",
    "id": "_ID_",
    "params": {
        "sceneId": "__SCENE_ID__",
        "userId": "3333"
    }
}
)"_json;

const JsonObject removeSceneNotification = R"(
{
    "method": "hub.scenes.notification.remove",
    "id": "_ID_",
    "params": {
        "sceneId": "__SCENE_ID__",
        "userId": "1111"
    }
}
)"_json;

const JsonObject enableScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.enabled.set",
    "params": {
        "_id": "__SCENE_ID__",
        "enabled" : true
    }
}
)"_json;

const JsonObject disableScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.enabled.set",
    "params": {
        "_id": "__SCENE_ID__",
        "enabled" : false
    }
}
)"_json;

const JsonObject EFWR_1561_createScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "house_modes": [
            "1",
            "2",
            "3",
            "4"
        ],
        "is_group": false,
        "name": "Test",
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
                        "value": "5d91f7280004000bc2297121"
                    },
                    {
                        "name": "value",
                        "type": "bool",
                        "value": true
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
                        "name": "setItemValue"
                    }
                },
                "blockType": "then",
                "delay": {
                    "days": 0,
                    "hours": 0,
                    "minutes": 0,
                    "seconds": 5
                },
                "fields": [
                    {
                        "name": "item",
                        "type": "item",
                        "value": "5d91f7280004000bc2297121"
                    },
                    {
                        "name": "value",
                        "type": "bool",
                        "value": false
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
                            "interval": "interval"
                        },
                        "name": "isInterval"
                    }
                },
                "blockType": "when",
                "fields": [
                    {
                        "name": "interval",
                        "type": "interval",
                        "value": "60s"
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject EFWR_1561_editScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params": {
        "_id": "__SCENE_ID__",
        "eo": {
            "enabled": true,
            "house_modes": [
                "1",
                "2",
                "3",
                "4"
            ],
            "is_group": false,
            "name": "Test",
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
                            "value": "5d91f7280004000bc2297121"
                        },
                        {
                            "name": "value",
                            "type": "bool",
                            "value": true
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
                            "name": "setItemValue"
                        }
                    },
                    "blockType": "then",
                    "fields": [
                        {
                            "name": "item",
                            "type": "item",
                            "value": "5d91fb360004000bc2297124"
                        },
                        {
                            "name": "value",
                            "type": "int",
                            "value": 0
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
                            "name": "setItemValue"
                        }
                    },
                    "blockType": "then",
                    "delay": {
                        "days": 0,
                        "hours": 0,
                        "minutes": 0,
                        "seconds": 5
                    },
                    "fields": [
                        {
                            "name": "item",
                            "type": "item",
                            "value": "5d91f7280004000bc2297121"
                        },
                        {
                            "name": "value",
                            "type": "bool",
                            "value": false
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
                            "name": "setItemValue"
                        }
                    },
                    "blockType": "then",
                    "delay": {
                        "days": 0,
                        "hours": 0,
                        "minutes": 0,
                        "seconds": 5
                    },
                    "fields": [
                        {
                            "name": "item",
                            "type": "item",
                            "value": "5d91fb360004000bc2297124"
                        },
                        {
                            "name": "value",
                            "type": "int",
                            "value": 0
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
                                "interval": "interval"
                            },
                            "name": "isInterval"
                        }
                    },
                    "blockType": "when",
                    "fields": [
                        {
                            "name": "interval",
                            "type": "interval",
                            "value": "60s"
                        }
                    ]
                }
            ]
        }
    }
}
)"_json;

const JsonObject EFWR_1580_createScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "__NAME__",
        "then" : [],
        "when" : []
    }
}
)"_json;

const JsonObject EFWR_1580_editScene = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params": {
        "_id": "__SCENE_ID__",
        "eo": {
            "enabled": true,
            "group_id": "",
            "is_group": false,
            "name": "__NAME__",
            "then" : [],
            "when" : []
        }
    }
}
)"_json;

const std::string EFWR_2195_editSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params": {
        "_id": "__SCENE_ID__",
        "eo": {
            "enabled": true,
            "group_id": "",
            "is_group": false,
            "name": "__NAME__",
            "then" : [
                __THEN_BLOCKS__
            ],
            "when" : []
        }
    }
}
)";

const JsonObject EFWR_3664_createScene = R"(
{
    "method": "hub.scenes.create",
    "id": "_ID_",
    "params": {
        "name": "efwr3664",
        "else": [],
        "enabled": false,
        "exceptions": [
            {
                "_id": "${autogen1}",
                "blockOptions": {
                    "method": {
                        "args": { "scene": "scene", "state": "state" },
                        "name": "isSceneState"
                    }
                },
                "blockType": "when",
                "fields": [
                    {
                        "name": "scene",
                        "type": "sceneId",
                        "value": "__SCENE_ID_3__"
                    },
                    {
                        "name": "state",
                        "type": "token",
                        "value": "finished"
                    }
                ]
            },
            {
                "_id": "${autogen2}",
                "blockOptions": {
                    "method": {
                        "args": { "scene": "scene", "state": "state" },
                        "name": "isSceneState"
                    }
                },
                "blockType": "when",
                "fields": [
                    {
                        "name": "scene",
                        "type": "sceneId",
                        "value": "__SCENE_ID_2__"
                    },
                    {
                        "name": "state",
                        "type": "token",
                        "value": "finished"
                    }
                ]
            }
        ],
        "is_group": false,
        "then": [
            {
                "blockOptions": {
                    "method": {
                        "args": { "sceneId": "sceneId" },
                        "name": "runScene"
                    }
                },
                "blockType": "then",
                "control": {
                    "exception": {
                        "blockId": "${autogen1}",
                        "type": "block_reference"
                    }
                },
                "fields": [
                    {
                        "name": "sceneId",
                        "type": "sceneId",
                        "value": "__SCENE_ID_2__"
                    },
                    {
                        "name": "block",
                        "type": "string",
                        "value": "then"
                    }
                ]
            },
            {
                "blockOptions": {
                    "method": {
                        "args": { "sceneId": "sceneId" },
                        "name": "runScene"
                    }
                },
                "blockType": "then",
                "control": {
                    "exception": {
                        "blockId": "${autogen2}",
                        "type": "block_reference"
                    }
                },
                "fields": [
                    {
                        "name": "sceneId",
                        "type": "sceneId",
                        "value": "__SCENE_ID_1__"
                    },
                    {
                        "name": "block",
                        "type": "string",
                        "value": "then"
                    }
                ]
            }
        ],
        "when": []
    }
}
)"_json;

extern const JsonObject createSameActionsDifferentDelays = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "is_group": false,
        "name": "SameActionsDifferentDelays",
        "parent_id": "",
        "exec_policy": "check_result",
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
                "delay": {
                    "days": 0,
                    "hours": 0,
                    "minutes": 0,
                    "seconds": 1
                },
                "fields": [
                    {
                        "name": "item",
                        "type": "item",
                        "value": "__ITEM_ID__"
                    },
                    {
                        "name": "value",
                        "type": "int",
                        "value": 5
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
                        "name": "setItemValue"
                    }
                },
                "blockType": "then",
                "delay": {
                    "days": 0,
                    "hours": 0,
                    "minutes": 0,
                    "seconds": 1
                },
                "fields": [
                    {
                        "name": "item",
                        "type": "item",
                        "value": "__ITEM_ID__"
                    },
                    {
                        "name": "value",
                        "type": "int",
                        "value": 4
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
                        "name": "setItemValue"
                    }
                },
                "blockType": "then",
                "delay": {
                    "days": 0,
                    "hours": 0,
                    "minutes": 0,
                    "seconds": 1
                },
                "fields": [
                    {
                        "name": "item",
                        "type": "item",
                        "value": "__ITEM_ID__"
                    },
                    {
                        "name": "value",
                        "type": "int",
                        "value": 3
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
                        "name": "setItemValue"
                    }
                },
                "delay": {
                    "days": 0,
                    "hours": 0,
                    "minutes": 0,
                    "seconds": 1
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
                        "type": "int",
                        "value": 2
                    }
                ]
            }
        ],
        "user_notifications": [],
        "when": []
    }
}
)"_json;

const std::string createSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "__NAME__",
        "exec_policy": "__EXEC_POLICY__",
        "icon": "__ICON__",
        "parent_id": "__PARENT_ID__",
        "then" : [
            __THEN_BLOCKS__
        ],
        "when" : [
            __WHEN_BLOCKS__
        ]
    }
}
)";

const std::string createSceneWithExceptionsRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.create",
    "params": {
        "enabled": true,
        "group_id": "",
        "is_group": false,
        "name": "__NAME__",
        "exec_policy": "__EXEC_POLICY__",
        "icon": "__ICON__",
        "parent_id": "__PARENT_ID__",
        "then" : [
            __THEN_BLOCKS__
        ],
        "when" : [
            __WHEN_BLOCKS__
        ],
        "exceptions" : [
            __EXCEPTION_BLOCKS__
        ]
    }
}
)";

const std::string deleteSceneRequest = R"(
{
    "id": "__ID__",
    "jsonrpc": "2.0",
    "api":"1.0",
    "method": "hub.scenes.delete",
    "params": {
        "_id": "__ID__"
    }
}
)";

const std::string setEnableSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.enabled.set",
    "params": {
        "_id": "__ID__",
        "enabled" : __ENABLED__
    }
}
)";

const std::string runSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api":"1.0",
    "method": "hub.scenes.run",
    "params": {
        "sceneId": "__ID__"
    }
}
)";

const std::string stopSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api":"1.0",
    "method": "hub.scenes.stop",
    "params": {
        "sceneId": "__ID__",
        "block": "__BLOCK__",
        "group": "__GROUP__"
    }
}
)";

const std::string editScenesRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params" : {
        "_id": "__ID__",
        "eo": {
            "enabled": true,
            "group_id": "",
            "is_group": false,
            "name": "__NAME__",
            "exec_policy": "__EXEC_POLICY__",
            "icon": "__ICON__",
            "parent_id": "__PARENT_ID__",
            "then" : [
                __THEN_BLOCKS__
            ],
            "when" : [
                __WHEN_BLOCKS__
            ]
        }
    }
}
)";

const std::string editSceneWithExceptionsRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.edit",
    "params" : {
        "_id": "__ID__",
        "eo": {
            "enabled": true,
            "group_id": "",
            "is_group": false,
            "name": "__NAME__",
            "exec_policy": "__EXEC_POLICY__",
            "icon": "__ICON__",
            "parent_id": "__PARENT_ID__",
            "then" : [
                __THEN_BLOCKS__
            ],
            "when" : [
                __WHEN_BLOCKS__
            ],
            "exceptions" : [
                __EXCEPTION_BLOCKS__
            ]
        }
    }
}
)";

const std::string setIconRequest = R"(
{
   "method": "hub.scenes.icon.set",
   "id": "_ID_",
   "params": {
       "_id": "__SCENE_ID__",
       "icon": "__ICON__"
   }
}
)";

const std::string setNameRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.name.set",
    "params": {
        "_id": "__SCENE_ID__",
        "name" : "__NAME__"
    }
}
)";

const std::string setMetaRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.meta.set",
    "params": {
        "sceneId": "__SCENE_ID__",
        "meta": {"key": "value"}
    }
}
)";

const std::string setBlockMetaRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.block.meta.set",
    "params": {
        "sceneId": "__SCENE_ID__",
        "blockId": "__BLOCK_ID__",
        "meta": {"key": "value"}
    }
}
)";

const std::string addScriptRequest = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.scenes.scripts.add",
   "params":{
      "name":"__NAME__",
      "code": "__CODE__"
   }
}
)";

const std::string deleteScriptRequest = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.scenes.scripts.delete",
   "params":{
      "_id":"__ID__"
   }
}
)";

const std::string scenesScriptsGetRequest = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.scenes.scripts.get",
   "params":{
      "_id":"__ID__"
   }
}
)";

const std::string setHouseModesRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.house_modes.set",
    "params": {
        "_id": "__SCENE_ID__",
        "houseModes" : __HOUSE_MODES__
    }
}
)";

const std::string setBlockEnabledSetRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.block.enabled.set",
    "params": {
        "sceneId": "__SCENE_ID__",
        "blockId": "__BLOCK_ID__",
        "enabled" : __ENABLED__
    }
}
)";

const std::string testActionBlock = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.action.block.test",
    "params": {
        "block": __BLOCK__
    }
}
)";

const std::string isItemStateWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
    "method":{
        "name":"isItemState",
        "args":{
            "item":"item",
            "value":"value"
        }
    }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string isItemStateWithSourceWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"isItemState",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "value":"value"
            }
        }
    },
    "fields":[
        { "name":"__SOURCE_NAME__", "type":"__SOURCE_TYPE__", "value": __SOURCE_VALUE__  },
        { "name":"value", "type":"__TYPE__", "value": __VALUE__ }
    ]
}
)";

const std::string isDeviceStateWithSourceWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"isDeviceState",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "armed":"armed",
                "reachable":"reachable",
                "device":"device"
            }
        }
    },
    "fields":[
        { "name":"__SOURCE_NAME__", "type":"__SOURCE_TYPE__", "value": __SOURCE_VALUE__  },
        { "name":"armed", "type":"bool", "value": __ARMED__ },
        { "name":"reachable", "type":"bool", "value": __REACHABLE__ }
    ]
}
)";

const std::string isItemStateChangedWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"isItemStateChanged",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "start":"start",
                "finish":"finish"
            }
        }
    },
    "fields":[
        {
            "name":"__SOURCE_NAME__",
            "type":"__SOURCE_TYPE__",
            "value": __SOURCE_VALUE__
        },
        {
            "name":"start",
            "type":"__START_TYPE__",
            "value":__START_VALUE__
        },
        {
            "name":"finish",
            "type":"__FINISH_TYPE__",
            "value":__FINISH_VALUE__
        }
    ]
}
)";

const std::string isItemStateOptionsWhenBlock = R"(
{
    "blockId":"__BLOCK_ID__",
    "blockType":"when",
    "blockOptions":{
    "method":{
        "name":"isItemState",
        "args":{
            "item":"item",
            "value":"value"
        }
    }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"value",
            "options": __OPTIONS__,
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string isButtonStateWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "item":"item",
            "value":"value"
         },
         "name":"isButtonState"
      }
   },
   "blockType":"when",
   "fields":[
      {
          "name":"item",
          "type":"item",
          "value" : "__ITEM_ID__"
      },
      {
          "name":"value",
          "type":"token",
          "value":"__VALUE__"
      }
   ]
}
)";

const std::string compareNumbersWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"compareNumbers",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "comparator":"comparator",
                "value":"value"
            }
        }
    },
    "fields":[
        {
            "name":"__SOURCE_NAME__",
            "type":"__SOURCE_TYPE__",
            "value": __SOURCE_VALUE__
        },
        {
            "name":"comparator",
            "type":"string",
            "value": "__COMPARATOR__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string sceneEventFieldsSource = R"(
{
    "name":"__SOURCE_NAME__",
    "type":"__SOURCE_TYPE__",
    "value": __SOURCE_VALUE__
})";

const std::string compareStringsWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"compareStrings",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "comparator":"comparator",
                "value":"value"
            }
        }
    },
    "fields":[
        {
            "name":"__SOURCE_NAME__",
            "type":"__SOURCE_TYPE__",
            "value": __SOURCE_VALUE__
        },
        {
            "name":"comparator",
            "type":"string",
            "value": "__COMPARATOR__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": "__VALUE__"
        }
    ]
}
)";

const std::string itemOperationWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"__METHOD__",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "operation":"operation",
                "value":"value"
            }
        }
    },
    "fields":[
        {
            "name":"__SOURCE_NAME__",
            "type":"__SOURCE_TYPE__",
            "value": __SOURCE_VALUE__
        },
        {
            "name":"operation",
            "type":"string",
            "value":"__OPERATION__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string compareValuesWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"compareValues",
            "args":{
                "__SOURCE_NAME__":"__SOURCE_NAME__",
                "comparator":"comparator",
                "valueType":"valueType",
                "value":"value"
            }
        }
    },
    "fields":[
        { "name":"__SOURCE_NAME__", "type":"__SOURCE_TYPE__", "value": __SOURCE_VALUE__  },
        { "name":"comparator", "type":"string", "value": "__COMPARATOR__" },
        { "name":"valueType", "type":"string", "value": "__VALUE_TYPE_VALUE__" },
        { "name":"value", "type":"__TYPE__", "value": __UNQUOTED_VALUE__ }
    ]
}
)";

const std::string isOnceSceneWhenBlock = R"(
{
    "blockOptions":{
        "method":{
            "args":{
                "time": "time",
                "day": "day",
                "month": "month",
                "year": "year"
            },
            "name":"isOnce"
        }
    },
    "blockType":"when",
    "fields":[
        {
            "name": "time",
            "type": "24_hours_time",
            "value": "__TIME__"
        },
        {
            "name": "day",
            "type": "int",
            "value": __DAY__
        },
        {
            "name": "month",
            "type": "int",
            "value": __MONTH__
        },
        {
            "name": "year",
            "type": "int",
            "value": __YEAR__
        }
    ]
}
)";

const std::string isDateDailyWhenBlock = R"(
{
    "blockOptions":{
        "method":{
            "args":{
                "type":"type",
                "time":"time"
            },
            "name":"isDate"
        }
    },
    "blockType":"when",
    "fields":[
        {
            "name": "type",
            "type": "string",
            "value": "daily"
        },
        {
            "name": "time",
            "type": "24_hours_time_array",
            "value": [ __TIME_ARRAY__ ]
        }
    ]
}
)";

const std::string isDateWithTimeWhenBlock = R"(
{
    "blockOptions": {
        "method": {
            "name": "isDate",
            "args": {
                "type": "type",
                "time": "time",
                "__DAYS_ARRAY_FIELD__": "__DAYS_ARRAY_FIELD__"
            }
        }
    },
    "blockType": "when",
    "fields": [
        {
            "name": "type",
            "type": "string",
            "value": "__TYPE__"
        },
        {
            "name": "time",
            "type": "24_hours_time_array",
            "value": __TIME_ARRAY__
        },
        {
            "name": "__DAYS_ARRAY_FIELD__",
            "type": "int_array",
            "value": __DAYS_ARRAY__
        }
    ]
}
)";

const std::string isDateWeeksWhenBlockTemplate = R"(
{
   "blockOptions": {
      "method": {
         "name": "isDate",
         "args": {
             "type": "type",
             "weeks": "weeks"
          }
      }
   },
   "blockType": "when",
   "fields": [
        {
            "name": "type",
            "type": "string",
            "value": __WEEKS_TYPE__
        },
        {
            "name": "weeks",
            "type": "int_array",
            "value": __WEEKS_ARRAY__
        }
   ]
}
)";

const std::string isIntervalWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "interval":"interval"
         },
         "name":"isInterval"
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"interval",
         "type":"interval",
         "value":"__INTERVAL__"
      }
   ]
}
)";

const std::string isSunStateWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "sunstate":"sunrise",
            "time":"time"
         },
         "name":"isSunState"
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"sunrise",
         "type":"string",
         "value":"__TYPE__"
      },
      {
         "name":"time",
         "type":"hms_interval",
         "value":"__TIME__"
      }
   ]
}
)";

const std::string isUserLockOperationWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"isUserLockOperation",
            "args":{
                "item":"item",
                "state":"state",
                "user":"user"
            }
        }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"state",
            "type":"token",
            "value":"lock"
        },
        {
            "name":"user",
            "type":"dictionaryId",
            "value":"__USER_ID__"
        }
    ]
}
)";

const std::string isUserLockOperationWhenBlockWithUserCodeAction = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"isUserLockOperation",
            "args":{
                "item":"item",
                "state":"state",
                "user":"user"
            }
        }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"state",
            "type":"token",
            "value":"user_code_entered"
        },
        {
            "name":"user",
            "type":"dictionaryId",
            "value":"__USER_ID__"
        }
    ]
}
)";

const std::string isDateRangeWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"isDateRange",
         "args":{
            "startTime":"startTime",
            "startDay":"startDay",
            "startMonth":"startMonth",
            "startYear":"startYear",
            "endTime":"endTime",
            "endDay":"endDay",
            "endMonth":"endMonth",
            "endYear":"endYear"
         }
      }
   },
   "blockType":"when",
   "fields":[
        {
            "name": "startTime",
            "type": "24_hours_time",
            "value": "__START_TIME__"
        },
        {
            "name": "startDay",
            "type": "int",
            "value": __START_DAY__
        },
        {
            "name": "startMonth",
            "type": "int",
            "value": __START_MONTH__
        },
        {
            "name": "startYear",
            "type": "int",
            "value": __START_YEAR__
        },
        {
            "name": "endTime",
            "type": "24_hours_time",
            "value": "__END_TIME__"
        },
        {
            "name": "endDay",
            "type": "int",
            "value": __END_DAY__
        },
        {
            "name": "endMonth",
            "type": "int",
            "value": __END_MONTH__
        },
        {
            "name": "endYear",
            "type": "int",
            "value": __END_YEAR__
        }
   ]
}
)";

const std::string isHouseModeChangedToWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"isHouseModeChangedTo",
         "args":{
            "houseMode":"houseMode"
         }
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"houseMode",
         "type":"houseModeId_array",
         "value": [ __HOUSE_MODES__ ]
      }
   ]
}
)";

const std::string isHouseModeAlarmedRangeWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"isHouseModeAlarmPhaseRange",
         "args":{
            "phase":"phase"
         }
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"phase",
         "type":"string",
         "value":"__PHASE__"
      }
   ]
}
)";

const std::string isHouseModeSwitchToRangeWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"isHouseModeSwitchToRange",
         "args":{
         }
      }
   },
   "blockType":"when",
   "fields":[
   ]
}
)";

const std::string isHouseModeChangedFromWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"isHouseModeChangedFrom",
         "args":{
            "houseMode":"houseMode"
         }
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"houseMode",
         "type":"houseModeId_array",
         "value": [ __HOUSE_MODES__ ]
      }
   ]
}
)";

const std::string isNetworkStateWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"isNetworkState",
         "args":{
              "interface":"id",
              "state":"state"
         }
      }
   },
   "fields":[
        {
           "name":"id",
           "type":"networkId",
           "value":"__NETWORK_ID__"
        },
        {
           "name":"state",
           "type":"token",
           "value":"__STATE__"
        }
   ]
}
)";

const std::string hasAtLeastOneDictionaryValueWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"hasAtLeastOneDictionaryValue",
            "args":{
                "item":"item",
                "value":"value"
            }
        }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"token",
            "value":"__TOKEN__"
        }
    ]
}
)";

const std::string isSceneStateWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"isSceneState",
         "args":{
            "scene":"scene",
            "state":"state"
         }
      }
   },
   "fields":[
      {
         "name":"scene",
         "type":"sceneId",
         "value": "__SCENE_ID__"
      },
      {
         "name":"state",
         "type":"token",
         "value": "__STATE__"
      }
   ]
}
)";

const std::string isCloudStateWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"isCloudState",
         "args":{
             "state":"state"
         }
      }
   },
   "fields":[
        {
           "name":"state",
           "type":"token",
           "value":"__STATE__"
        }
   ]
}
)";

const std::string andOperatorWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"and",
         "args":{
            "blocks":"blocks"
         }
      }
   },
   "fields":[
      {
         "name":"blocks",
         "type":"blocks",
         "value": [
            __BLOCKS__
         ]
      }
   ]
}
)";

const std::string notOperatorWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"not",
         "args":{
            "block":"block"
         }
      }
   },
   "fields":[
      {
         "name":"block",
         "type":"block",
         "value": __BLOCK__
      }
   ]
}
)";

const std::string orOperatorWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"or",
         "args":{
            "blocks":"blocks"
         }
      }
   },
   "fields":[
      {
         "name":"blocks",
         "type":"blocks",
         "value": [
            __BLOCKS__
         ]
      }
   ]
}
)";

const std::string xorOperatorWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"xor",
         "args":{
            "blocks":"blocks"
         }
      }
   },
   "fields":[
      {
         "name":"blocks",
         "type":"blocks",
         "value": [
            __BLOCKS__
         ]
      }
   ]
}
)";

const std::string sendCloudAbstractCommandThenBlock = R"(
{
   "exec_policy": "__EXEC_POLICY__",
   "blockOptions":{
      "method":{
         "name":"sendCloudAbstractCommand",
         "args":{
            "abstractId":"abstract",
            "capability":"capability",
            "command":"command",
            "parameters":"parameters"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"abstract",
         "type":"string",
         "value": "asdasdn-2342342-23423"
      },
      {
         "name":"capability",
         "type":"string",
         "value": "text_command"
      },
      {
         "name":"command",
         "type":"string",
         "value": "send"
      },
      {
         "name":"parameters",
         "type":"object",
         "value": {
             "text": "turn on the light"
         }
      }
   ]
}
)";

const std::string setItemValueThenBlock = R"(
{
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions":{
        "method":{
            "args":{
            "item":"item",
            "value":"value"
            },
            "name":"setItemValue"
        }
    },
    "blockType":"then",
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value" : "__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string setItemValueThenBlockWithScale = R"(
{
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions":{
        "method":{
            "args":{
            "item":"item",
            "value":"value"
            },
            "name":"setItemValue"
        }
    },
    "blockType":"then",
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value" : "__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "scale":"__SCALE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string groupSetItemValueThenBlock = R"(
{
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions":{
        "method":{
            "args":{
            "deviceGroup": "deviceGroup",
            "itemGroup": "itemGroup",
            "value": "value"
            },
            "name": "groupSetItemValue"
        }
    },
    "blockType":"then",
    "fields":[
         {
         "name": "deviceGroup",
         "type": "device_group",
         "value": "__GROUP_DEVICE_ID__"
          },
          {
             "name": "itemGroup",
             "type": "item_group",
             "value": "__GROUP_ITEM_ID__"
          },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string groupSetDeviceArmedThenBlock = R"(
{
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions": {
       "method":{
          "args":{
             "device":"deviceGroup",
             "deviceFlag":"deviceFlag"
          },
          "name":"groupSetDeviceArmed"
       }
    },
    "blockType":"then",
    "fields":[
       {
          "name":"deviceGroup",
          "type":"device_group",
          "value" : "__DEVICE_GROUP_ID__"
       },
       {
          "name":"deviceFlag",
          "type":"bool",
          "value" : __VALUE__
       }
    ]
 }
)";

const std::string groupToggleValueThenBlock = R"(
{
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions":{
        "method":{
            "args":{
            "deviceGroup": "deviceGroup",
            "itemGroup": "itemGroup"
            },
            "name": "groupToggleValue"
        }
    },
    "blockType":"then",
    "fields":[
         {
         "name": "deviceGroup",
         "type": "device_group",
         "value": "__GROUP_DEVICE_ID__"
          },
          {
             "name": "itemGroup",
             "type": "item_group",
             "value": "__GROUP_ITEM_ID__"
          }
    ]
}
)";

const std::string switchHouseModeThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"switchHouseMode",
         "args":{
            "houseMode":"houseMode"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"houseMode",
         "type":"houseModeId",
         "value": "__HOUSE_MODE_ID__"
      }
   ]
}
)";

const std::string sendHttpRequestThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"sendHttpRequest",
         "args":{
            "url":"url",
            "credential":"credential",
            "request":"request",
            "contentType":"contentType",
            "content":"content",
            "headers":"headers",
            "skipSecurity":"skipSecurity"
         }
      }
   },
   "blockType":"then",
   "exec_policy": "__EXEC_POLICY__",
   "fields":[
      {
         "name":"url",
         "type":"string",
         "value": "__URL__"
      },
      {
         "name":"credential",
         "type":"credential",
         "value": {
             "user":"__USER__",
             "password":"__PASSWORD__"
         }
      },
      {
         "name":"request",
         "type":"string",
         "value": "__REQUEST_TYPE__"
      },
      {
         "name":"contentType",
         "type":"string",
         "value": "__CONTENT_TYPE__"
      },
      {
         "name":"content",
         "type":"string",
         "value": "__CONTENT__"
      },
      {
         "name":"headers",
         "type":"dictionary",
         "value": {
              __HEADERS__
         }
      },
      {
         "name":"skipSecurity",
         "type":"bool",
         "value": __SKIP_SECURITY__
      }
   ]
}
)";

const std::string sendHttpRequestSaveResultThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"sendHttpRequest",
         "args":{
            "url":"url",
            "credential":"credential",
            "request":"request",
            "contentType":"contentType",
            "content":"content",
            "headers":"headers",
            "skipSecurity":"skipSecurity"
         }
      }
   },
    "saveResult": {
            "method": "setVariable",
            "args": {
                "name": "__GLOBAL_VARIABLE_NAME__",
                "valueType": "int"
            }
       },
   "blockType":"then",
   "exec_policy": "__EXEC_POLICY__",
   "fields":[
      {
         "name":"url",
         "type":"string",
         "value": "__URL__"
      },
      {
         "name":"credential",
         "type":"credential",
         "value": {
             "user":"__USER__",
             "password":"__PASSWORD__"
         }
      },
      {
         "name":"request",
         "type":"string",
         "value": "__REQUEST_TYPE__"
      },
      {
         "name":"contentType",
         "type":"string",
         "value": "__CONTENT_TYPE__"
      },
      {
         "name":"content",
         "type":"string",
         "value": "__CONTENT__"
      },
      {
         "name":"headers",
         "type":"dictionary",
         "value": {
              __HEADERS__
         }
      },
      {
         "name":"skipSecurity",
         "type":"bool",
         "value": __SKIP_SECURITY__
      }
   ]
}
)";

const std::string runSceneThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"runScene",
         "args":{
            "sceneId":"sceneId"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"sceneId",
         "value": "__SCENE_ID__"
      }
   ]
}
)";

const std::string stopSceneThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"stopScene",
         "args":{
            "sceneId":"sceneId"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"sceneId",
         "value":"__SCENE_ID__"
      }
   ]
}
)";

const std::string stopSceneElseBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"stopScene",
         "args":{
            "sceneId":"sceneId",
            "block": "block"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"sceneId",
         "value":"__SCENE_ID__"
      },
      {
         "name":"block",
         "type":"string",
         "value": "else"
      }
   ]
}
)";

const std::string runSceneThenGroupsBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"runScene",
         "args":{
            "sceneId":"sceneId",
            "block":"block",
            "group":"group"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"sceneId",
         "value": "__SCENE_ID__"
      },
      {
         "name":"block",
         "type":"string",
         "value": "thenGroups"
      },
      {
         "name":"group",
         "type":"string",
         "value": "__GROUP__"
      }
   ]
}
)";

const std::string setSceneStateThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"setSceneState",
         "args":{
            "sceneId":"sceneId",
            "enabled":"enabled"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"sceneId",
         "value": "__SCENE_ID__"
      },
      {
         "name":"enabled",
         "type":"bool",
         "value": __ENABLED__
      }
   ]
}
)";

const std::string resetLatchThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"resetLatch",
         "args":{
            "sceneId":"sceneId",
            "blockId":"blockId"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"string",
         "value":"__SCENE_ID__"
      },
      {
         "name":"blockId",
         "type":"string",
         "value":"__BLOCK_ID__"
      }
   ]
}
)";

const std::string resetSceneLatchesThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"resetSceneLatches",
         "args":{ "sceneId":"sceneId" }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"sceneId",
         "type":"string",
         "value":"__SCENE_ID__"
      }
   ]
}
)";

/**
 * @param __SCRIPT_ID__
 * @param __UNQUOTED_OBJECT__
 * */
const std::string runPluginScriptThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"runPluginScript",
         "args":{
            "script":"script",
            "params":"params"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"script",
         "type":"string",
         "value":"__SCRIPT_ID__"
      },
      {
         "name":"params",
         "type":"object",
         "value": __UNQUOTED_OBJECT__
      }
   ]
}
)";

const std::string blockReferenceExceptionBlock = R"(
{
    "exception": {
        "type": "block_reference",
        "blockId": "__BLOCK_ID__"
    }
}
)";

const std::string externalBlockReferenceExceptionBlock = R"(
{
    "exception": {
        "type": "block_reference",
        "sceneId": "__SCENE_ID__",
        "blockId": "__BLOCK_ID__"
    }
}
)";

const std::string expressionReferenceExceptionBlock = R"(
{
    "exception": {
        "type": "expression_reference",
        "name": "__NAME__"
    }
}
)";

const std::string setExpressionCommand = R"(
{
    "method": "hub.scenes.expressions.set",
    "params": {
        "name": "__NAME__",
        "params": {},
        "code": "__CODE__",
        "valueType": "__VALUE_TYPE__",
        "metadata": null,
        "variable": false
    }
}
)";

const std::string createGlobalSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.global.create",
    "params": {
        "_id": "__ID__",
        "enabled": true,
        "tag": "__TAG__",
        "exec_policy": "__EXEC_POLICY__",
        "then" : [
            __THEN_BLOCKS__
        ],
        "when" : [
            __WHEN_BLOCKS__
        ]
    }
}
)";

const std::string deleteGlobalSceneRequest = R"(
{
    "id": "__ID__",
    "jsonrpc": "2.0",
    "api":"1.0",
    "method": "hub.scenes.global.delete",
    "params": {
        "_id": "__ID__"
    }
}
)";

const std::string getGlobalScenesListRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.global.list",
    "params": {
        "onlyId": __ONLY_ID__
    }
}
)";

const std::string editGlobalScenesRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.global.edit",
    "params" : {
        "_id": "__ID__",
        "scene": {
            "enabled": __ENABLED__,
            "tag": "__TAG__",
            "exec_policy": "__EXEC_POLICY__",
            "then" : [
                __THEN_BLOCKS__
            ],
            "when" : [
                __WHEN_BLOCKS__
            ]
        }
    }
}
)";

const std::string enableGlobalSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.global.enabled.set",
    "params": {
        "_id": "__ID__",
        "enabled" : __ENABLED__
    }
}
)";

const std::string runGlobalSceneRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api":"1.0",
    "method": "hub.scenes.global.run",
    "params": {
        "_id": "__ID__"
    }
}
)";

const std::string globalIsItemStateWhenBlock = R"(
{
    "controllerId":"__CONTROLLER_ID__",
    "blockType":"when",
    "blockOptions":{
    "method":{
        "name":"isItemState",
        "args":{
            "item":"item",
            "value":"value"
        }
    }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string globalIsItemArmedStateWhenBlock = R"(
{
    "controllerId":"__CONTROLLER_ID__",
    "blockType":"when",
    "blockOptions":{
    "method":{
        "name":"isItemState",
        "args":{
            "item":"item",
            "value":"value",
            "armed":"armed"
        }
    }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        },
        {
            "name":"armed",
            "type":"bool",
            "value":__ARMED__
        }
    ]
}
)";

const std::string globalIsButtonStateWhenBlock = R"(
{
    "controllerId":"__CONTROLLER_ID__",
    "blockOptions":{
      "method":{
         "args":{
            "item":"item",
            "value":"value"
         },
         "name":"isButtonState"
      }
    },
    "blockType":"when",
    "fields":[
      {
          "name":"item",
          "type":"item",
          "value" : "__ITEM_ID__"
      },
      {
          "name":"value",
          "type":"token",
          "value":"__VALUE__"
      }
    ]
}
)";

const std::string globalHasAtLeastOneDictionaryValueWhenBlock = R"(
{
    "controllerId":"__CONTROLLER_ID__",
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"hasAtLeastOneDictionaryValue",
            "args":{
                "item":"item",
                "value":"value"
            }
        }
    },
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value":"__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"token",
            "value":"__TOKEN__"
        }
    ]
}
)";

const std::string globalSetItemValueThenBlock = R"(
{
    "controllerId": "__CONTROLLER_ID__",
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions":{
        "method":{
            "args":{
            "item":"item",
            "value":"value"
            },
            "name":"setItemValue"
        }
    },
    "blockType":"then",
    "fields":[
        {
            "name":"item",
            "type":"item",
            "value" : "__ITEM_ID__"
        },
        {
            "name":"value",
            "type":"__TYPE__",
            "value": __VALUE__
        }
    ]
}
)";

const std::string globalSetDeviceArmed = R"(
{
    "controllerId": "__CONTROLLER_ID__",
    "exec_policy": "__EXEC_POLICY__",
    "blockOptions": {
       "method":{
          "args":{
             "device":"device",
             "deviceFlag":"deviceFlag"
          },
          "name":"setDeviceArmed"
       }
    },
    "blockType":"then",
    "fields":[
       {
          "name":"device",
          "type":"device",
          "value" : "__DEVICE_ID__"
       },
       {
          "name":"deviceFlag",
          "type":"bool",
          "value" : __VALUE__
       }
    ]
 }
)";

const std::string addCustomScriptRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.scripts.add",
    "params": {
        "name": "__NAME__",
        "code": "__CODE__"
    }
}
)";

const std::string deleteCustomScriptRequest = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.scenes.scripts.delete",
   "params":{
      "_id":"__SCRIPT_ID__"
   }
}
)";

const std::string runCustomScriptThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "name":"runCustomScript",
         "args":{
            "script":"script"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"script",
         "type":"scriptId",
         "value": "__SCRIPT_ID__"
      }
   ]
}
)";

const std::string checkBatteryConditionBlock = R"(
{
  "blockType":"when",
  "blockOptions":{
     "method":{
        "name":"isBatteryState",
        "args":{
             "state":"state"
        }
     }
  },
  "fields":[
       {
          "name":"state",
          "type":"token",
          "value":"__STATE__"
       }
  ]
}
)";

const std::string checkBatteryLevelConditionBlock = R"(
{
    "blockType":"when",
     "blockOptions":{
        "method":{
            "name":"isBatteryLevel",
            "args":{
                  "comparator":"comparator",
                  "value":"value"
            }
        }
    },
    "fields":[
       {
          "name":"value",
          "type":"int",
          "value":20
       },
       {
          "name":"comparator",
          "type":"string",
          "value":"__COMP__"
       }
  ]
}
)";

const std::string isFeatureStateWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"isFeatureState",
         "args":{
            "name":"name",
            "status":"status",
            "version":"version",
            "comparator":"comparator"
         }
      }
   },
   "fields":[
      {
         "name":"name",
         "type":"string",
         "value":"__NAME__"
      },
      {
         "name":"status",
         "type":"bool",
         "value":__STATUS__
      },
      {
         "name":"version",
         "type":"string",
         "value":"__VERSION__"
      },
      {
         "name":"comparator",
         "type":"string",
         "value":"__COMPARATOR__"
      }
   ]
}
)";

const std::string isFeatureStateWithoutStatusWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"isFeatureState",
         "args":{
            "name":"name",
            "version":"version",
            "comparator":"comparator"
         }
      }
   },
   "fields":[
      {
         "name":"name",
         "type":"string",
         "value":"__NAME__"
      },
      {
         "name":"version",
         "type":"string",
         "value":"__VERSION__"
      },
      {
         "name":"comparator",
         "type":"string",
         "value":"__COMPARATOR__"
      }
   ]
}
)";

const std::string isFeatureStateWithoutVersionWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"isFeatureState",
         "args":{
            "name":"name",
            "status":"status"
         }
      }
   },
   "fields":[
      {
         "name":"name",
         "type":"string",
         "value":"__NAME__"
      },
      {
         "name":"status",
         "type":"bool",
         "value":__STATUS__
      }
   ]
}
)";

const std::string compareNumbersRangeConditionBlock = R"(
{
  "blockType":"when",
  "blockOptions":{
     "method":{
        "name":"compareNumberRange",
        "args":{
           "__SOURCE_NAME__":"__SOURCE_NAME__",
           "comparator":"comparator",
           "startValue":"startValue",
           "endValue":"endValue"
        }
     }
  },
  "fields":[
     {
        "name":"__SOURCE_NAME__",
        "type":"__SOURCE_TYPE__",
        "value": __SOURCE_VALUE__
     },
     {
        "name":"comparator",
        "type":"string",
        "value":"__COMP__"
     },
     {
        "name":"startValue",
        "type":"int",
        "value":10
     },
     {
        "name":"endValue",
        "type":"int",
        "value":50
     }
  ]
}
)";

const std::string isDeviceItemGroupEvent = R"(
{
    "blockType":"when",
    "blockOptions":{
        "method":{
            "name":"isDeviceItemGroup",
            "args":{
                "deviceGroup":"deviceGroup",
                "itemGroup":"itemGroup"
            }
        }
    },
    "fields":[
        {
            "name":"deviceGroup",
            "type":"device_group",
            "value":"__DEVICE_GROUP__"
        },
        {
            "name":"itemGroup",
            "type":"item_group",
            "value":"__ITEM_GROUP__"
        }
    ]
}
)";

const std::string softwareUpdateConditionBlock = R"(
{
  "blockType":"when",
  "blockOptions":{
     "method":{
        "name":"isFirmwareUpdateState",
        "args":{
           "state":"state"
        }
     }
  },
  "fields":[
     {
        "name":"state",
        "type":"token",
        "value":"__STATE__"
     }
  ]
}
)";

const std::string isGroupStateWhenBlock = R"(
{
    "blockType":"when",
    "blockOptions":{
    "method":{
        "name":"isGroupState",
        "args":{
            "scene":"scene",
            "group":"group",
            "state":"state"
        }
    }
    },
    "fields":[
        {
            "name":"scene",
            "type":"sceneId",
            "value":"__SCENE__"
        },
        {
            "name":"group",
            "type":"string",
            "value":"__GROUP__"
        },
        {
            "name":"state",
            "type":"string",
            "value":"__STATE__"
        }
    ]
}
)";

const std::string isDeviceStateWhenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "device":"device",
            "armed":"armed",
            "reachable":"reachable"
         },
         "name":"isDeviceState"
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"device",
         "type":"device",
         "value":"__DEVICE_ID__"
      },
      {
         "name":"armed",
         "type":"bool",
         "value":__ARMED__
      },
      {
         "name":"reachable",
         "type":"bool",
         "value":__REACHABLE__
      }
   ]
}
)";

const std::string isDeviceStateWhenBlockArmed = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "device":"device",
            "armed":"armed"
         },
         "name":"isDeviceState"
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"device",
         "type":"device",
         "value":"__DEVICE_ID__"
      },
      {
         "name":"armed",
         "type":"bool",
         "value":__ARMED__
      }
   ]
}
)";

const std::string isDeviceStateWhenBlockReachable = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "device":"device",
            "reachable":"reachable"
         },
         "name":"isDeviceState"
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"device",
         "type":"device",
         "value":"__DEVICE_ID__"
      },
      {
         "name":"reachable",
         "type":"bool",
         "value":__REACHABLE__
      }
   ]
}
)";

const std::string isDeviceStateWhenBlockFailed = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "device":"device"
         },
         "name":"isDeviceState"
      }
   },
   "blockType":"when",
   "fields":[
      {
         "name":"device",
         "type":"device",
         "value":__DEVICE_ID__
      }
   ]
}
)";

const std::string isDeviceStateWhenBlockFailedDevice = R"(
{
   "blockOptions":{
      "method":{
         "args":{
         },
         "name":"isDeviceState"
      }
   },
   "blockType":"when",
   "fields":[
   ]
}
)";

const std::string cloudAPIActionSaveResultThenBlock = R"(
{
   "blockType":"then",
   "blockOptions":{
      "method":{
         "name":"cloudAPI",
         "args":{
            "method": "cloudApiName",
            "version": "version",
            "params": "params"
         }
      }
   },
  "saveResult": {
            "method": "setVariable",
            "args": {
                "name": "setGlobalVariableActionCloud",
                "valueType": "string"
            }
       },
   "fields":[
      {
         "name": "cloudApiName",
         "type": "string",
         "value": "__CALL__"
      },

      {
         "name":"version",
         "type":"int",
         "value": 1
      },

      {
         "name":"params",
         "type":"string",
         "value": "{\"a\":\"b\"}"
      }
   ]
}
)";

const std::string cloudAPIAction = R"(
{
   "blockType":"then",
   "blockOptions":{
      "method":{
         "name":"cloudAPI",
         "args":{
            "method": "cloudApiName",
            "version": "version",
            "params": "params"
         }
      }
   },
   "fields":[
      {
         "name": "cloudApiName",
         "type": "string",
         "value": "some_api_call"
      },

      {
         "name":"version",
         "type":"int",
         "value": 1
      },

      {
         "name":"params",
         "type":"string",
         "value": "{\"a\":\"b\"}"
      }
   ]
}
)";

const std::string isDictionaryChangedWhenBlockTemplate = R"(
{
    "blockType": "when",
    "blockOptions": {
        "method": {
            "name": "isDictionaryChanged",
            "args": {
                "item": "item",
                "key": "key",
                "operation": "operation"
            }
        }
    },
    "fields": [
        {
            "name": "item",
            "type": "item",
            "value": "__ITEM_ID__"
        },
        {
            "name": "key",
            "type": "token",
            "value": "__KEY__"
        },
        {
            "name": "operation",
            "type": "token",
            "value": "__OPERATION__"
        }
    ]
})";

const std::string functionOperatorWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"function",
         "args":{
            "blocks":"blocks"
         }
      },
      "function": {
          __FUNCTION__
      }
   },
   "fields":[
      {
         "name":"blocks",
         "type":"blocks",
         "value": [
            __BLOCKS__
         ]
      }
   ]
}
)";

const std::string setExpression = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "name":"name",
            "type":"type",
            "code":"code",
            "valueType":"valueType",
            "metadata":"metadata"
         },
         "name":"setExpression"
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"name",
         "type":"string",
         "value":"setExpressionAction"
      },
      {
         "name":"type",
         "type":"string",
         "value":"code"
      },
      {
         "name":"code",
         "type":"string",
         "value":"return 2 + 2 * 2.1"
      },
      {
         "name":"valueType",
         "type":"string",
         "value":"float"
      },
      {
         "name":"metadata",
         "type":"object",
         "value":{

         }
      }
   ]
}
)";

const std::string setExpressionWithParams = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "name":"name",
            "type":"type",
            "code":"code",
            "valueType":"valueType",
            "metadata":"metadata",
            "params": "params"
         },
         "name":"setExpression"
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"name",
         "type":"string",
         "value":"setExprAction"
      },
      {
         "name":"type",
         "type":"string",
         "value":"code"
      },
      {
         "name":"code",
         "type":"string",
         "value":"return 2 + 2 * 2.1"
      },
      {
         "name":"valueType",
         "type":"string",
         "value":"float"
      },
      {
         "name":"metadata",
         "type":"object",
         "value":{

         }
      },
      {
        "name": "params",
        "type": "object",
        "value": {
            "items": [
                {
                    "name": "sound_volume_to_ten",
                    "_id": "__ID__"
                }
            ]
        }
    }
   ]
}
)";

const std::string isDetectedInHotzoneWhenBlockTemplate = R"(
{
    "blockType": "when",
    "blockOptions": {
        "method": {
            "name": "isDetectedInHotzone",
            "args": {
                "item": "item",
                "hotzones": "hotzones",
                "match": "match",
                "filters": "filters"
            }
        }
    },
    "fields": [
        {
            "name": "item",
            "type": "item",
            "value": "__ITEM_ID__"
        },
        {
            "name": "hotzones",
            "type": "tokenArray",
            "value": __HOTZONES__
        },
        {
            "name": "match",
            "type": "token",
            "value": "__MATCH__"
        },
        {
            "name": "filters",
            "type": "dictionary.hotzone_match",
            "value": __FILTERS__
        }
    ]
})";

const std::string setValueActonBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "name":"name",
            "type":"type",
            "value":"value",
            "valueType":"valueType",
            "metadata":"metadata"
         },
         "name":"setVariable"
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"name",
         "type":"string",
         "value":"__NAME__"
      },
      {
         "name":"type",
         "type":"string",
         "value":"value"
      },
      {
         "name":"value",
         "type":"int",
         "value":123
      },
      {
         "name":"valueType",
         "type":"string",
         "value":"int"
      },
      {
         "name":"metadata",
         "type":"object",
         "value":{
            "key":"value"
         }
      }
   ]
}
)";

const std::string toggleExpressionValueThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "source":"expression"
         },
         "name":"toggleValue"
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"expression",
         "type":"expression",
         "value":"__VALUE__"
      }
   ]
}
)";

const std::string toggleItemValueThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "source":"item"
         },
         "name":"toggleValue"
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"item",
         "type":"item",
         "value":"__VALUE__"
      }
   ]
}
)";

const std::string toggleItemValueDelayedThenBlock = R"(
{
   "exec_policy": "__EXEC_POLICY__",
   "blockOptions":{
      "method":{
         "args":{
            "source":"item"
         },
         "name":"toggleValue"
      }
   },
    "delay" : {
           "seconds": 2,
           "minutes": 0,
           "hours": 0,
           "days": 0
       },
   "blockType":"then",
   "fields":[
      {
         "name":"item",
         "type":"item",
         "value":"__VALUE__"
      }
   ]
}
)";

const std::string getSceneStatusRequest = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.status.get",
    "params": {
        "sceneId": "__SCENE_ID__"
    }
}
)";

const std::string scenesDeviceGroupCreate = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.device.group.create",
   "params":{
        "name": "_DEVICE_GROUP_NAME_",
        "categories": [
            { "category": "security_sensor", "subcategories": [ "co2", "leak" ]  },
            { "category": "scene_controller", "exceptions": [ "panic" ] }
        ],
        "devices": [],
        "exceptions": [],
        "entryDelay": "none",
        "role": "user",
        "packageId" : ""
    }
}
)";

const std::string scenesDeviceGroupCreateWithDevices = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.device.group.create",
   "params":{
        "name": "_DEVICE_GROUP_NAME_",
        "devices": __DEVICES__,
        "exceptions": __EXCEPTIONS__
    }
}
)";

const std::string scenesDeviceGroupCreateWithDevicesAndDelay = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.device.group.create",
   "params":{
        "name": "_DEVICE_GROUP_NAME_",
        "categories": [],
        "devices": __DEVICES__,
        "exceptions": __EXCEPTIONS__,
        "entryDelay": "__ENTRY_DELAY__"
    }

}
)";

const std::string scenesItemGroupCreate = R"(
{
    "id":"_ID_",
    "jsonrpc":"2.0",
    "api": "1.0",
    "method":"hub.item.group.create",
    "params":{
        "name": "__NAME__",
        "itemNames": [ "__ITEM_NAME__" ],
        "hasGetter": true,
        "hasSetter": false,
        "valueType": "__VALUE_TYPE__",
        "enum": [],
        "persistent": false
    }
}
)";

const std::string scenesItemGroupCreateWithMultipleItemNames = R"(
{
    "id":"_ID_",
    "jsonrpc":"2.0",
    "api": "1.0",
    "method":"hub.item.group.create",
    "params":{
        "name": "__NAME__",
        "itemNames": [ "__ITEM_NAME1__", "__ITEM_NAME2__", "__ITEM_NAME3__", "__ITEM_NAME4__" ],
        "hasGetter": true,
        "hasSetter": false,
        "valueType": "__VALUE_TYPE__",
        "enum": [],
        "persistent": false
    }
}
)";

const std::string scenesItemGroupDelete = R"(
{
    "id":"_ID_",
    "jsonrpc":"2.0",
    "api": "1.0",
    "method":"hub.item.group.delete",
    "params":{
        "id": "__GROUP_ID__",
        "force": __FORCE__
    }
}
)";

const std::string scenesItemGroupUpdate = R"(
{
    "id":"_ID_",
    "jsonrpc":"2.0",
    "api": "1.0",
    "method":"hub.item.group.update",
    "params":{
        "id": "__GROUP_ID__",
        "name": "__NAME__",
        "itemNames": [ "test_token" ],
        "hasGetter": true,
        "hasSetter": false,
        "valueType": "token",
        "enum": [],
        "persistent": false
    }
}
)";

const std::string scenesPersistentDeviceGroupCreate = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.device.group.create",
   "params":{
        "name": "_DEVICE_GROUP_NAME_",
        "categories": [
            { "category": "security_sensor", "subcategories": [ "co2", "leak" ]  },
            { "category": "scene_controller", "exceptions": [ "panic" ] }
        ],
        "devices": [],
        "exceptions": [],
        "persistent": __PERSISTENT__
    }
}
)";

const std::string scenesPersistentItemGroupCreate = R"(
{
    "id":"_ID_",
    "jsonrpc":"2.0",
    "api": "1.0",
    "method":"hub.item.group.create",
    "params":{
        "name": "__NAME__",
        "itemNames": [ "test_token" ],
        "hasGetter": true,
        "hasSetter": false,
        "valueType": "token",
        "enum": [],
        "persistent": __PERSISTENT__
    }
}
)";

const std::string scenesDeviceGroupDelete = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.device.group.delete",
   "params":{
        "id": "__GROUP_ID__",
        "force": __FORCE__
    }
}
)";

const std::string scenesDeviceGroupUpdate = R"(
{
   "id":"_ID_",
   "jsonrpc":"2.0",
   "method":"hub.device.group.update",
   "params":{
        "id": "__GROUP_ID__",
        "name": "__DEVICE_GROUP_NAME__",
        "categories": [
            { "category": "security_sensor", "subcategories": [ "co", "leak" ]  },
            { "category": "scene_controller", "exceptions": [ "panic" ] }
        ],
        "devices": [],
        "exceptions": []
    }
})";

const std::string deviceGroupsList = R"(
{
    "id": "__ID__",
    "api": "1.0",
    "method": "hub.device.groups.list",
    "params": {
        "version": "__VERSION__"
    }
})";

const std::string deviceGroupGet = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api": "1.0",
    "method": "hub.device.group.get",
    "params": {
        "id": "__GROUP_ID__"
}
})";

const std::string deviceGroupFind = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api": "1.0",
    "method": "hub.device.group.find",
    "params": {
        "deviceGroupId": "__GROUP_ID__"
}
})";

const std::string itemGroupsList = R"(
{
    "id": "__ID__",
    "api": "1.0",
    "method": "hub.item.groups.list",
    "params": {
        "version": "__VERSION__"
    }
})";

const std::string itemGroupGet = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api": "1.0",
    "method": "hub.item.group.get",
    "params": {
        "id": "__GROUP_ID__"
}
})";

const std::string deviceGroupExpand = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api": "1.0",
    "method": "hub.device.item.group.expand",
    "params": {
        "deviceGroupId": "__DEVICE_GROUP_ID__",
        "showItems": __FLAG__,
        "version": "__VERSION__"
}
})";

const std::string deviceItemGroupExpand = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "api": "1.0",
    "method": "hub.device.item.group.expand",
    "params": {
        "deviceGroupId": "__DEVICE_GROUP_ID__",
        "itemGroupId": "__ITEM_GROUP_ID__"
}
})";

const JsonObject cloudAccessKeysSyncResponseTemplate = R"(
{
    "error": null,
    "method": "cloud.access_keys_controller_sync",
    "id": "_ID_",
    "result": {
        "clients": {},
        "peers": {},
        "current": [],
        "version": 11
    }
}
)"_json;

const JsonObject cloneSceneJson = R"(
{
    "id": "_ID_",
    "jsonrpc": "2.0",
    "method": "hub.scenes.clone",
    "params": {
        "_id": "__ID__",
        "name": "__NAME__"
    }
}
)"_json;

const std::string forwardNotificationThenBlock = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "notificationTemplate": "notificationTemplate",
            "targets": "targets",
            "device": "device",
            "item": "item",
            "notificationBlock": "notificationBlock"
         },
         "name":"forwardNotifications"
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"notificationTemplate",
         "type":"templateId",
         "value":"__TEMPLATE__"
      },
      {
         "name":"targets",
         "type":"array",
         "value":__TARGETS__
      },
      {
         "name":"device",
         "type":"string",
         "value":"__DEVICE_ID__"
      },
      {
         "name":"item",
         "type":"string",
         "value":"__ITEM_ID__"
      },
      {
         "name":"notificationBlock",
         "type":"object",
         "value":{"key":"value"}
      }
   ]
}
)";

}  // namespace Tests
}  // namespace HubPlatform
