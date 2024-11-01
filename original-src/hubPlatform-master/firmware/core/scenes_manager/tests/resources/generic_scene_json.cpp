#include "scenes_manager/tests/resources/generic_scene_json.h"

namespace SceneEngine {
namespace UT {

JsonObject emptyScene = R"(
{
    "enabled": true,
    "group_id": "",
    "is_group": false,
    "name": "EmptyName",
    "parent_id": "",
    "then" : [],
    "when" : []
}
)"_json;

JsonObject sceneWithExceptionsInActions = R"(
{
        "name": "sceneWithExceptionsInActions",
        "else": [],
        "enabled": false,
        "exceptions": [
            {
                "_id": "${block1}",
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
                        "value": "__SCENE_ID_1__"
                    },
                    {
                        "name": "state",
                        "type": "token",
                        "value": "finished"
                    }
                ]
            },
            {
                "_id": "${block2}",
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
                        "blockId": "${block1}",
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
            }],
        "else":[
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
                        "blockId": "${block2}",
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
)"_json;

JsonObject sceneWithInnerRelationsInActions = R"(
{
   "_id":"65098981000000063f021d53",
   "enabled":false,
   "is_group":false,
   "name":"sceneWithInnerRelationsInActions",
   "parent_id":"",
   "syncNotification":true,
   "then":[
   ],
   "else":[
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
               "value":"__SCENE_ID__"
            }
         ]
      }
   ],
   "when":[
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
)"_json;

JsonObject editedSceneWithInnerRelationsInActions = R"(
{
    "_id": "__SCENE_ID__",
    "eo": {
          "_id":"__SCENE_ID__",
           "enabled":false,
           "is_group":false,
           "name":"sceneWithInnerRelationsInActions",
           "parent_id":"",
           "syncNotification":true,
           "then":[
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
           ],
           "else":[
           {
               "blockOptions": {
                  "method": {
                     "args": {
                        "sceneId": "sceneId"
                     },
                     "name": "resetSceneLatches"
                  }
               },
               "blockType": "then",
               "fields": [
                  {
                     "name": "sceneId",
                     "type": "string",
                     "value": "__SCENE_ID__"
                  }
               ]
           }
           ],
           "when":[
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
                "_id":"__BLOCK_ID__",
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

JsonObject intervalCondition = R"(
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
         "value":"212s"
      }
   ]
}
)"_json;

JsonObject sunstateCondition = R"(
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
         "value":"before"
      },
      {
         "name":"time",
         "type":"hms_interval",
         "value":"10:30"
      }
   ]
}
)"_json;

JsonObject sunstateConditionRange = R"(
{
    "blockOptions": {
        "method": {
            "args": {
                "sunstate": "sunrise",
                "time": "time",
                "range": "range"
            },
            "name": "isSunState"
        }
    },
    "blockType": "when",
    "fields": [
        {
            "name": "sunrise",
            "type": "string",
            "value": "before"
        },
        {
            "name": "time",
            "type": "hms_interval",
            "value": "10:00"
        },
        {
            "name": "range",
            "type": "token",
            "value": "after"
        }
    ]
}
)"_json;

JsonObject dateCondition = R"(
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
            "value": [ "10:20", "12:40", "13:32" ]
        }
    ]
}
)"_json;

JsonObject onceCondition = R"(
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
}
)"_json;

JsonArray wrongWhenBlockArray = R"(
{
    "blockOptions":{
        "method":{
            "args":{},
            "name":"wrongMethod"
        }
    },
    "blockType":"when",
    "fields":[]
}
)"_json;

JsonObject itemEventExpressionCondition = R"(
{
   "blockOptions": {
      "method": {
         "args": { "item": "item", "value": "value" },
         "name": "isItemState"
      }
   },
   "blockType": "when",
   "fields": [
        { "name": "item", "type": "item", "value": "__ITEM_ID__" },
        { "name": "value", "type": "expression", "value": "__EXPRESSION_NAME__" }
   ]
}
)"_json;

JsonObject compareNumbersEventExpressionCondition = R"(
{
   "blockOptions": {
      "method": {
         "name": "compareNumbers",
         "args": { "item": "item", "value": "value", "comparator": "comparator" }
      }
   },
   "blockType": "when",
   "fields": [
        { "name": "item", "type": "item", "value": "__ITEM_ID__" },
        { "name": "comparator", "type": "string", "value": "==" },
        { "name": "value", "type": "expression", "value": "__EXPRESSION_NAME__" }
   ]
}
)"_json;

std::string compareOperatorWhenBlockTemplate = R"(
{
   "blockOptions": {
      "method": {
         "name": "__COMPARE_METHOD__",
         "args": { "item": "item", "value": "value", "comparator": "comparator" }
      }
   },
   "blockType": "when",
   "fields": [
        { "name": "item", "type": "item", "value": "__ITEM_ID__" },
        { "name": "comparator", "type": "string", "value": "__COMPARISON__" },
        { "name": "value", "type": "__TYPE__", "value": "__VALUE__" }
   ]
}
)";

std::string compareValuesWhenBlockTemplate = R"(
{
   "blockOptions": {
      "method": {
         "name": "compareValues",
         "args": { "item": "item", "value": "value", "valueType": "valueType", "comparator": "comparator" }
      }
   },
   "blockType": "when",
   "fields": [
        { "name": "valueType", "type": "string", "value": "__VALUE_TYPE__" },
        { "name": "item", "type": "item", "value": "__ITEM_ID__" },
        { "name": "comparator", "type": "string", "value": "__COMPARISON__" },
        { "name": "value", "type": "__TYPE__", "value": __UNQUOTED_VALUE__ }
   ]
}
)";

std::string itemOperationWhenBlockTemplate = R"(
{
   "blockOptions": {
      "method": {
         "name": "__METHOD__",
         "args": { "item": "item", "value": "value", "operation": "operation" }
      }
   },
   "blockType": "when",
   "fields": [
        { "name": "item", "type": "item", "value": "__ITEM_ID__" },
        { "name": "operation", "type": "string", "value": "__OPERATION__" },
        { "name": "value", "type": "__TYPE__", "value": __VALUE__ }
   ]
}
)";

std::string isItemStateChangedWhenBlockTemplate = R"(
{
    "blockOptions":{
        "method":{
            "name":"isItemStateChanged",
            "args":{ "item": "item", "start": "start", "finish": "finish" }
        }
    },
    "fields": [
        { "name": "item", "type": "item", "value": "__ITEM_ID__" },
        { "name": "start", "type": "int", "value": __START_VALUE__ },
        { "name": "finish", "type": "int", "value": __FINISH_VALUE__ }
    ]
}
)";

std::string isDateWeeksWhenBlockTemplate = R"({
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
})";

std::string isDateWithTimeBlockTemplate = R"({
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
})";

JsonArray whenBlockArray = { intervalCondition, sunstateCondition, dateCondition, onceCondition };

const JsonObject hubBackupRunAction = R"(
{
   "blockOptions":{
      "method":{
         "name":"runHubBackup",
         "args":{
            "action":"action",
            "backupId":"backupId"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"action",
         "type":"token",
         "value": "restore"
      },
      {
         "name":"backupId",
         "type":"dictionary.string",
         "value":{
                "key": "RnEobYMsxOkMwQg8zOGdrBdS1I8VVwSoGx0WvnikM7RWbLOIC4U7uNd",
                "uuid": "ba85b840-0687-11ec-bee4-a156a78fb21b"
                }
      }
   ]
}
)"_json;

const JsonObject setItemValueAction = R"(
{
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
   "delay" : {
       "seconds": 12,
       "minutes": 30,
       "hours": 1,
       "days": 0
   },
   "fields":[
      {
         "name":"item",
         "type":"item",
         "value" : "897607_32771_1"
      },
      {
         "name":"value",
         "type":"int",
         "value": 10
      }
   ]
}
)"_json;

const JsonObject setItemValueActionForTemperature = R"(
{
   "blockOptions":{
      "method":{
         "args":{ "item":"item", "value":"value" },
         "name":"setItemValue"
      }
   },
   "blockType":"then",
   "fields":[
      { "name":"item", "type":"item", "value" : "897607_32771_2" },
      { "name":"value", "type":"float", "value": 78.7, "scale": "fahrenheit" }
   ]
}
)"_json;

const JsonObject setDeviceArmedAction = R"(
{
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
         "value" : "76asdasd76yasf89"
      },
      {
         "name":"deviceFlag",
         "type":"bool",
         "value" : false
      }
   ]
}
)"_json;

// TODO: efwr-2396 append setItemValueActionForTemperature to the array when it starts working
const JsonArray thenBlockArray = { setItemValueAction, setDeviceArmedAction };

extern const JsonArray thenBlockEmptyBlockOptions = R"(
{
   "blockType":"then",
   "fields":[
      {
         "name":"item",
         "type":"item",
         "value" : "897607_32771_1"
      },
      {
         "name":"value",
         "type":"int",
         "value": 10
      }
   ]
}
)"_json;

const JsonArray thenBlockEmptyMethod = R"(
{
   "blockOptions":{
   },
   "blockType":"then",
   "fields":[
      {
         "name":"item",
         "type":"item",
         "value" : "897607_32771_1"
      },
      {
         "name":"value",
         "type":"int",
         "value": 10
      }
   ]
}
)"_json;

const JsonArray thenBlockEmptyName = R"(
{
   "blockOptions":{
      "method":{
         "args":{
            "item":"item",
            "value":"value"
         }
      }
   },
   "blockType":"then",
   "fields":[
      {
         "name":"item",
         "type":"item",
         "value" : "897607_32771_1"
      },
      {
         "name":"value",
         "type":"int",
         "value": 10
      }
   ]
}
)"_json;

const JsonObject sceneBlocksTemplates = R"(
{
    "version":"1.0",
    "when":[
        {
            "item_filter": {
                 "names":["dimmer", "meter", "lock_operation"]
            },
            "label": {
                  "lang_tag": "ui7_token",
                  "text": "English string"
            },
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
                    "value":"value"
                },
                {
                    "name":"value",
                    "type":"token",
                    "value": "manual_unlock_operation",
                    "options":[
                        {
                            "value":"manual_lock_operation",
                            "label":{
                                "lang_tag":"ui7_door_manually_locked",
                                "text":"Door manually locked"
                            }
                        },
                        {
                            "value":"manual_unlock_operation",
                            "label":{
                                "lang_tag":"ui7_door_manually_unlocked",
                                "text":"Door manually unlocked"
                            }
                        },
                        {
                            "value":"rf_lock_operation",
                            "label":{
                                "lang_tag":"ui7_door_rf_locked",
                                "text":"Door remotely locked"
                            }
                        },
                        {
                            "value":"rf_unlock_operation",
                            "label":{
                                "lang_tag":"ui7_door_rf_unlocked",
                                "text":"Door remotely unlocked"
                            }
                        },
                        {
                            "value":"keypad_lock_operation",
                            "label":{
                                "lang_tag":"ui7_door_keypad_locked",
                                "text":"Door locked from keypad"
                            }
                        },
                        {
                            "value":"keypad_unlock_operation",
                            "label":{
                                "lang_tag":"ui7_door_keypad_unlocked",
                                "text":"Door unlocked from keypad"
                            }
                        },
                        {
                            "value":"manual_not_fully_locked_operation",
                            "label":{
                                "lang_tag":"manual_not_fully_locked_operation",
                                "text":"Manually partially locked"
                            }
                        },
                        {
                            "value":"rf_not_fully_locked_operation",
                            "label":{
                                "lang_tag":"rf_not_fully_locked_operation",
                                "text":"Remotely partially locked"
                            }
                        },
                        {
                            "value":"auto_lock_not_fully_locked_operation",
                            "label":{
                                "lang_tag":"auto_lock_not_fully_locked_operation",
                                "text":"Automatically partially locked"
                            }
                        },
                        {
                            "value":"unlock_by_rf_with_invalid_user_code",
                            "label":{
                                "lang_tag":"unlock_by_rf_with_invalid_user_code",
                                "text":"Remotely unlocked with invalid user code"
                            }
                        },
                        {
                            "value":"lock_by_rf_with_invalid_user_code",
                            "label":{
                                "lang_tag":"lock_by_rf_with_invalid_user_code",
                                "text":"Remotely locked with invalid user code"
                            }
                        },
                        {
                            "value":"lock_jammed",
                            "label":{
                                "lang_tag":"lock_jammed",
                                "text":"Locking jammed"
                            }
                        }
                    ]
                }
            ]
        },
        {
            "item_filter": {
                 "names":["dimmer", "meter"]
            },
            "label":{
                "lang_tag":"ui7_token",
                "text":"English string"
            },
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
                    "value":"value"
                },
                {
                    "name":"comparator",
                    "type":"string"
                },
                {
                    "name":"value",
                    "editable":true,
                    "type":"int",
                    "value":80
                }
            ]
        },
        {
            "item_filter": {
                 "names":["switch", "camera"]
            },
           "label": {
                "lang_tag": "ui7_token",
                "text": "English string"
           },
           "blockType":"when",
           "blockOptions":{
              "method":{
                 "name":"isItemState1",
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
                 "value":"value"
              },
              {
                 "name":"value",
                 "type":"bool",
                 "value": true
              }
           ]
        },
        {
            "blockId":"0001-000000c6",
            "item_filter":{
                "names":[
                    "user_lock_operation"
                ]
            },
            "label":{
                "lang_tag":"user_lock_operation_state",
                "text":"A PIN Code is entered"
            },
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
                    "type":"item"
                },
                {
                    "name":"state",
                    "type":"token",
                    "value":"lock"
                },
                {
                    "name":"user",
                    "type":"dictionaryId",
                    "optionItemValue": {
                        "itemFinder":{
                            "item": "user_codes",
                            "resultField":"itemId"
                        }
                    },
                    "value":""
                }
            ]
        },
        {
            "blockId":"0001-000000c7",
            "item_filter":{
                "names":[
                    "user_lock_operation1"
                ]
            },
            "label":{
                "lang_tag":"user_lock_operation_state",
                "text":"A PIN Code is entered"
            },
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
                    "type":"item"
                },
                {
                    "name":"state",
                    "type":"token",
                    "value":"lock"
                },
                {
                    "name":"user",
                    "type":"dictionaryId",
                    "optionItemValue": {
                        "itemFinder":{
                            "item": "user_codes1",
                            "resultField":"itemId"
                        }
                    },
                    "value":""
                }
            ]
        },
        {
            "blockId":"0001-000000c8",
            "item_filter":{
                "names":[
                    "switch"
                ]
            },
            "device_filter":{
                "category":[
                    {"switch":["valve"]}
                ]
            },
            "label":{
                "lang_tag":"when_valve",
                "text":"Whenever valve state changed"
            },
            "blockType":"when",
            "blockOptions":{
                "method":{
                    "name":"isItemState2",
                    "args":{
                        "item":"item",
                        "value":"value"
                    }
                }
            },
            "fields":[
                {
                    "name":"item",
                    "type":"item"
                },
                {
                    "name":"value",
                    "type":"bool",
                    "options":[
                        {
                            "value":true,
                            "label":{
                                "lang_tag":"ui7_mode_open",
                                "text":"Open"
                            }
                        },
                        {
                            "value":false,
                            "label":{
                                "lang_tag":"ui7_mode_closed",
                                "text":"Closed"
                            }
                        }
                    ],
                    "value":true
                }
            ]
        },
        {
            "blockId":"0001-000000c9",
            "item_filter":{
                "names":[
                    "switch"
                ]
            },
            "device_filter":{
                "not":{
                    "category":[
                        {"switch":["valve"]}
                    ]
                }
            },
            "label":{
                "lang_tag":"when_switch",
                "text":"Whenever switch state changed"
            },
            "blockType":"when",
            "blockOptions":{
                "method":{
                    "name":"isItemState3",
                    "args":{
                        "item":"item",
                        "value":"value"
                    }
                }
            },
            "fields":[
                {
                    "name":"item",
                    "type":"item"
                },
                {
                    "name":"value",
                    "type":"bool",
                    "options":[
                        {
                            "value":true,
                            "label":{
                                "lang_tag":"ui7_on",
                                "text":"On"
                            }
                        },
                        {
                            "value":false,
                            "label":{
                                "lang_tag":"ui7_mode_off",
                                "text":"Off"
                            }
                        }
                    ],
                    "value":true
                }
            ]
        },
        {
            "blockId":"0001-0000002a",
            "item_filter":{
                "names":[
                    "motion"
                ]
            },
            "parent_device_filter":{
                "not":{
                    "category":[
                        { "camera":[ "indoor_cam", "outdoor_cam", "doorbell_cam" ] }
                    ]
                }
            },
            "label":{
                "lang_tag":"ui7_an_armed_sensor_is_tripped",
                "text":"Whenever _DEVICE_NAME_ is armed and detects motion"
            },
            "blockType":"when",
            "blockOptions":{
                "method":{
                    "name":"isItemState0000002a",
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
                    "type":"item"
                },
                {
                    "name":"value",
                    "type":"bool",
                    "value":true
                },
                {
                    "name":"armed",
                    "type":"bool",
                    "value":true
                }
            ]
        },
        {
            "blockId":"0001-000000E0",
            "item_filter":{
                "names":[
                    "motion"
                ]
            },
            "label":{
                "lang_tag":"when_motion_state",
                "text":"Whenever _DEVICE_NAME_ state is changed"
            },
            "blockType":"when",
            "blockOptions":{
                "method":{
                    "name":"isItemState000000E0",
                    "args":{
                        "item":"item",
                        "value":"value"
                    }
                }
            },
            "fields":[
                {
                    "name":"item",
                    "type":"item"
                },
                {
                    "name":"value",
                    "type":"bool",
                    "options":[
                        {
                            "value":true,
                            "label":{
                                "lang_tag":"ui7_motion_detected",
                                "text":"Detected"
                            }
                        },
                        {
                            "value":false,
                            "label":{
                                "lang_tag":"ui7_mode_not_detected",
                                "text":"Not detected"
                            }
                        }
                    ],
                    "value":true
                }
            ]
        }
    ]
}
)"_json;

const std::vector< JsonObject > itemsList = {
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c56",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": false,
        "valueFormatted": "false",
        "valueType": "bool",
        "deviceCategory": "switch",
        "deviceSubcategory": "interior_plugin"
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c57",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "hasGetter": true,
        "hasSetter": false,
        "name": "dimmer",
        "show": true,
        "value": 0.00499999988824129,
        "valueFormatted": "0.005",
        "valueType": "float",
        "deviceCategory": "dimmable_light",
        "deviceSubcategory": "dimmable_bulb"
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c58",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "hasGetter": true,
        "hasSetter": false,
        "name": "dimmer",
        "show": true,
        "value": 0,
        "valueFormatted": "0",
        "valueType": "float",
        "deviceCategory": "dimmable_light",
        "deviceSubcategory": "dimmable_bulb"
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c59",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "hasGetter": false,
        "hasSetter": true,
        "name": "meter",
        "show": true,
        "value": 0,
        "valueFormatted": "0",
        "valueType": "int",
        "deviceCategory": "power_meter",
        "deviceSubcategory": ""
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c5a",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "hasGetter": true,
        "hasSetter": false,
        "name": "meter",
        "show": true,
        "value": 124.670997619629,
        "valueFormatted": "124.671",
        "valueType": "float",
        "deviceCategory": "power_meter",
        "deviceSubcategory": ""
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c5b",
        "deviceId": "5dd2a8eebfb5be6d20008c55",
        "hasGetter": true,
        "hasSetter": false,
        "name": "meter",
        "show": true,
        "value": 0,
        "valueFormatted": "0",
        "valueType": "float",
        "deviceCategory": "power_meter",
        "deviceSubcategory": ""
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008c60",
        "deviceId": "5dd2a8eebfb5be6d20008c56",
        "hasGetter": false,
        "hasSetter": true,
        "name": "camera",
        "show": true,
        "value": 0,
        "valueFormatted": "0",
        "valueType": "int",
        "deviceCategory": "camera",
        "deviceSubcategory": "indoor_cam"
    }
    )"_json,
    R"(
    {
        "_id": "5df8c51f7f000004b1eb358c",
        "deviceId": "5df8c51f7f000004b1eb3582",
        "hasGetter": true,
        "hasSetter": false,
        "name": "electric_meter_amper",
        "show": true,
        "value": 0.0149999996647239,
        "valueFormatted": "0.015",
        "valueType": "float",
        "deviceCategory": "power_meter",
        "deviceSubcategory": ""
    }
    )"_json,
};

const std::vector< JsonObject > tokenItemsList = {
    R"(
    {
        "_id": "5e4178147f000040a93369b1",
        "deviceId": "5e4178147f000040a93369bc",
        "enum": [
            "manual_lock_operation",
            "manual_unlock_operation",
            "rf_lock_operation",
            "rf_unlock_operation",
            "keypad_lock_operation",
            "keypad_unlock_operation",
            "manual_not_fully_locked_operation",
            "rf_not_fully_locked_operation",
            "auto_lock_not_fully_locked_operation",
            "unlock_by_rf_with_invalid_user_code",
            "lock_by_rf_with_invalid_user_code",
            "lock_jammed"
        ],
        "hasGetter": true,
        "hasSetter": false,
        "name": "lock_operation",
        "show": true,
        "value": "",
        "valueFormatted": "token",
        "type": "token"
    }
    )"_json,
    R"(
    {
        "_id": "5e4178147f000040a93369b2",
        "deviceId": "5e4178147f000040a93369bc",
        "enum": [
            "manual_lock_operation",
            "manual_unlock_operation",
            "manual_not_fully_locked_operation",
            "rf_not_fully_locked_operation",
            "auto_lock_not_fully_locked_operation",
            "unlock_by_rf_with_invalid_user_code",
            "lock_jammed"
        ],
        "hasGetter": true,
        "hasSetter": false,
        "name": "lock_operation",
        "show": true,
        "value": "",
        "valueFormatted": "token",
        "type": "token"
    }
    )"_json,
    R"(
    {
        "_id": "5e4178147f000040a93369b3",
        "deviceId": "5e4178147f000040a93369bc",
        "enum": [
            "rf_lock_operation",
            "rf_unlock_operation",
            "keypad_lock_operation",
            "keypad_unlock_operation",
            "manual_not_fully_locked_operation",
            "rf_not_fully_locked_operation",
            "lock_jammed"
        ],
        "hasGetter": true,
        "hasSetter": false,
        "name": "lock_operation",
        "show": true,
        "value": "",
        "valueFormatted": "token",
        "type": "token"
    }
    )"_json,
    R"(
    {
        "_id": "5e4178147f000040a93369b4",
        "deviceId": "5e4178147f000040a93369bc",
        "enum": [
            "manual_lock_operation",
            "lock_by_rf_with_invalid_user_code"
        ],
        "hasGetter": true,
        "hasSetter": false,
        "name": "lock_operation",
        "show": true,
        "value": "",
        "valueFormatted": "token",
        "type": "token"
    }
    )"_json,
    R"(
    {
        "_id": "5e4178147f000040a93369b5",
        "deviceId": "5e4178147f000040a93369bc",
        "enum": [
            "manual_lock_operation",
            "manual_unlock_operation",
            "rf_lock_operation",
            "rf_unlock_operation"
        ],
        "hasGetter": true,
        "hasSetter": false,
        "name": "lock_operation",
        "show": true,
        "value": "",
        "valueFormatted": "token",
        "type": "token"
    }
    )"_json
};

const std::vector< JsonObject > optionItemsList = {
    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9b5",
        "deviceId": "5e4a5fef7f0000707ff6e9b2",
        "hasGetter": true,
        "hasSetter": false,
        "name": "user_lock_operation",
        "show": true,
        "value": {
            "action": "unknown",
            "userId": -1
        },
        "valueFormatted": "",
        "valueType": "user_lock_operation"
    }
    )"_json,
    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9bd",
        "deviceId": "5e4a5fef7f0000707ff6e9b2",
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes",
        "show": true,
        "userCodeRestriction": "^\\d{4,10}$",
        "value": {
            "1": {
                "code": "1111",
                "name": "Code 1"
            }
        },
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json,

    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9b6",
        "deviceId": "5e4a5fef7f0000707ff6e9b3",
        "hasGetter": true,
        "hasSetter": false,
        "name": "user_lock_operation",
        "show": true,
        "value": {
            "action": "unknown",
            "userId": -1
        },
        "valueFormatted": "",
        "valueType": "user_lock_operation"
    }
    )"_json,
    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9be",
        "deviceId": "5e4a5fef7f0000707ff6e9b3",
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes",
        "show": true,
        "userCodeRestriction": "^\\d{4,10}$",
        "value": {
            "1": {
                "code": "1111",
                "name": "Code 1"
            }
        },
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json,

    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9b7",
        "deviceId": "5e4a5fef7f0000707ff6e9b4",
        "hasGetter": true,
        "hasSetter": false,
        "name": "user_lock_operation",
        "show": true,
        "value": {
            "action": "unknown",
            "userId": -1
        },
        "valueFormatted": "",
        "valueType": "user_lock_operation"
    }
    )"_json,
    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9bf",
        "deviceId": "5e4a5fef7f0000707ff6e9b4",
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes",
        "show": true,
        "userCodeRestriction": "^\\d{4,10}$",
        "value": {
            "1": {
                "code": "1111",
                "name": "Code 1"
            }
        },
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json,

    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9b8",
        "deviceId": "5e4a5fef7f0000707ff6e9b3",
        "hasGetter": true,
        "hasSetter": false,
        "name": "user_lock_operation1",
        "show": true,
        "value": {
            "action": "unknown",
            "userId": -1
        },
        "valueFormatted": "",
        "valueType": "user_lock_operation"
    }
    )"_json,
    R"(
    {
        "_id": "5e4a5fef7f0000707ff6e9c0",
        "deviceId": "5e4a5fef7f0000707ff6e9b3",
        "elementType": "userCode",
        "elementsMaxNumber": 250,
        "hasGetter": true,
        "hasSetter": true,
        "name": "user_codes1",
        "show": true,
        "userCodeRestriction": "^\\d{4,10}$",
        "value": {
            "1": {
                "code": "1111",
                "name": "Code 1"
            }
        },
        "valueFormatted": "",
        "valueType": "dictionary"
    }
    )"_json
};

const std::vector< JsonObject > switchItemsList = {
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008ca1",
        "deviceId": "5dd2a8eebfb5be6d20008ca1",
        "enum": [
            "On",
            "Off"
        ],
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": "On",
        "valueFormatted": "token",
        "valueType": "token",
        "deviceCategory": "switch",
        "deviceSubcategory": "valve"
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008ca2",
        "deviceId": "5dd2a8eebfb5be6d20008ca1",
        "enum": [
            "Open",
            "Closed"
        ],
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": "Open",
        "valueFormatted": "token",
        "valueType": "token",
        "deviceCategory": "switch",
        "deviceSubcategory": "interior_plugin"
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008ca3",
        "deviceId": "5dd2a8eebfb5be6d20008ca1",
        "enum": [
            "Open",
            "Closed"
        ],
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": "Open",
        "valueFormatted": "token",
        "valueType": "token",
        "deviceCategory": "switch",
        "deviceSubcategory": ""
    }
    )"_json,
    R"(
    {
        "_id": "5dd2a8efc1b5be6d20008ca4",
        "deviceId": "5dd2a8eebfb5be6d20008ca1",
        "enum": [
            "Open",
            "Closed"
        ],
        "hasGetter": true,
        "hasSetter": true,
        "name": "switch",
        "show": true,
        "value": "Open",
        "valueFormatted": "token",
        "valueType": "token",
        "deviceCategory": "",
        "deviceSubcategory": ""
    }
    )"_json
};

extern const std::vector< JsonObject > motionItemsList = {
    R"(
    {
        "_id": "611cd54e00000051c09becaf",
        "deviceId": "611cd54e00000051c09becae",
        "hasGetter": true,
        "hasSetter": false,
        "name": "motion",
        "show": true,
        "value": false,
        "valueFormatted": "false",
        "valueType": "bool",
        "deviceCategory": "generic_sensor",
        "deviceSubcategory": "motion",
        "parentCategory": "camera",
        "parentSubcategory": "outdoor_cam"
    }
    )"_json,
};

const JsonObject createScene = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "__NAME__",
    "parent_id": "5c050abd518af4117b2e2496",
    "permission": {
    },
    "then": [],
    "when": []
    }
)"_json;

const JsonObject createScene1 = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "NewR",
    "parent_id": "5c050abd518af4117b2e2496",
    "permission": {
      "devices": "s",
      "ezlo": "s",
      "rules": "s",
      "ui": "s",
      "users": "s"
    },
    "sender": "victory@ezlo.com",
    "serial": "0000000000897607",
    "then": [
      {
          "blockOptions": {
              "method": {
                  "args": {
                      "item": "item",
                      "value": "value"
                  },
                  "name": "setSwitchState"
              }
          },
          "blockType": "then",
          "fields": [
              {
                  "name": "item",
                  "type": "item",
                  "value": "897607_32770_2"
              },
              {
                  "name": "value",
                  "type": "option",
                  "value": false
              }
          ]
      }
    ],
    "when": [
      {
          "blockOptions": {
              "method": {
                  "args": {
                      "item": "item",
                      "value": "value"
                  },
                  "name": "isItemState"
              },
              "trigger_object": {
                  "item": "item"
              }
          },
          "blockType": "when",
          "fields": [
              {
                  "name": "item",
                  "type": "item",
                  "value": "897607_32770_1"
              },
              {
                  "name": "value",
                  "type": "option",
                  "value": true
              }
          ]
      }
    ]
    }
)"_json;

const JsonObject createScene2 = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "NewR",
    "sender": "victory@ezlo.com",
    "permission": {
        "devices": "s",
        "ezlo": "s",
        "rules": "s",
        "ui": "s",
        "users": "s"
    },
    "parent_id": "5c050abd518af4117b2e2496",
    "then": [
       {
           "blockOptions": {
               "method": {
                   "args": {
                       "item": "item",
                       "value": "value"
                   },
                   "name": "decreaseDimmer"
               }
           },
           "blockType": "then",
           "fields": [
               {
                   "name": "item",
                   "type": "item",
                   "value": "897607_32771_1"
               },
               {
                   "name": "value",
                   "type": "number",
                   "value": 10
               }
           ]
       }
    ],
    "when": [
       {
           "blockOptions": {
               "method": {
                   "args": {
                       "item": "item",
                       "value": "value"
                   },
                   "name": "isItemState"
               },
               "trigger_object": {
                   "item": "item"
               }
           },
           "blockType": "when",
           "fields": [
               {
                   "name": "item",
                   "type": "item",
                   "value": "897607_32770_1"
               },
               {
                   "name": "value",
                   "type": "option",
                   "value": true
               }
           ]
       }
    ]
}
)"_json;

const JsonObject createScene3 = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "0001-00000022",
    "parent_id": "5c050abd518af4117b2e2496",
    "then": [
    ],
    "when": [
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
                    {
                        "blockId":"0001-00000022",
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
                                "value": "897607_32770_1"
                            },
                            {
                                 "name":"value",
                                 "type":"token",
                                 "options":[
                                     {
                                         "value":"no_changes",
                                         "label":{
                                             "lang_tag":"stop_detecting",
                                             "text":"Stop detecting"
                                         }
                                     },
                                     {
                                         "value":"rapid_temperature_rise",
                                         "label":{
                                             "lang_tag":"rapid_temperature_rise",
                                             "text":"Rapid temperature rise"
                                         }
                                     },
                                     {
                                         "value":"rapid_temperature_fail",
                                         "label":{
                                             "lang_tag":"rapid_temperature_fail",
                                             "text":"Rapid temperature fail"
                                         }
                                     }
                                 ],
                                 "value":"rapid_temperature_fail"
                             }
                        ]
                    }
                 ]
              }
           ]
        },
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
                 "value": {
                    "blockId":"0001-00000022",
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
                            "value": "897607_32770_1"
                        },
                        {
                             "name":"value",
                             "type":"token",
                             "options":[
                                 {
                                     "value":"no_changes",
                                     "label":{
                                         "lang_tag":"stop_detecting",
                                         "text":"Stop detecting"
                                     }
                                 },
                                 {
                                     "value":"rapid_temperature_rise",
                                     "label":{
                                         "lang_tag":"rapid_temperature_rise",
                                         "text":"Rapid temperature rise"
                                     }
                                 },
                                 {
                                     "value":"rapid_temperature_fail",
                                     "label":{
                                         "lang_tag":"rapid_temperature_fail",
                                         "text":"Rapid temperature fail"
                                     }
                                 }
                             ],
                             "value":"rapid_temperature_fail"
                         }
                    ]
                 }
              }
           ]
        },
        {
           "blockId":"0001-00000022",
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
                   "value": "897607_32770_1"
               },
               {
                    "name":"value",
                    "type":"token",
                    "options":[
                        {
                            "value":"no_changes",
                            "label":{
                                "lang_tag":"stop_detecting",
                                "text":"Stop detecting"
                            }
                        },
                        {
                            "value":"rapid_temperature_rise",
                            "label":{
                                "lang_tag":"rapid_temperature_rise",
                                "text":"Rapid temperature rise"
                            }
                        },
                        {
                            "value":"rapid_temperature_fail",
                            "label":{
                                "lang_tag":"rapid_temperature_fail",
                                "text":"Rapid temperature fail"
                            }
                        }
                    ],
                    "value":"rapid_temperature_fail"
                }
           ]
        }
    ]
}
)"_json;

const JsonObject createScene3Expected = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "0001-00000022",
    "parent_id": "5c050abd518af4117b2e2496",
    "then": [
    ],
    "when": [
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
                        {
                            "blockId":"0001-00000022",
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
                                    "value": "897607_32770_1"
                                },
                                {
                                     "name":"value",
                                     "type":"token",
                                     "options":[
                                         {
                                             "value":"no_changes",
                                             "label":{
                                                 "lang_tag":"stop_detecting",
                                                 "text":"Stop detecting"
                                             }
                                         },
                                         {
                                             "value":"rapid_temperature_rise",
                                             "label":{
                                                 "lang_tag":"rapid_temperature_rise",
                                                 "text":"Rapid temperature rise"
                                             }
                                         },
                                         {
                                             "value":"rapid_temperature_fall",
                                             "label":{
                                                 "lang_tag":"rapid_temperature_fail",
                                                 "text":"Rapid temperature fall"
                                             }
                                         }
                                     ],
                                     "value":"rapid_temperature_fall"
                                 }
                            ]
                        }
                 ]
              }
           ]
        },
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
                 "value": {
                        "blockId":"0001-00000022",
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
                                "value": "897607_32770_1"
                            },
                            {
                                 "name":"value",
                                 "type":"token",
                                 "options":[
                                     {
                                         "value":"no_changes",
                                         "label":{
                                             "lang_tag":"stop_detecting",
                                             "text":"Stop detecting"
                                         }
                                     },
                                     {
                                         "value":"rapid_temperature_rise",
                                         "label":{
                                             "lang_tag":"rapid_temperature_rise",
                                             "text":"Rapid temperature rise"
                                         }
                                     },
                                     {
                                         "value":"rapid_temperature_fall",
                                         "label":{
                                             "lang_tag":"rapid_temperature_fail",
                                             "text":"Rapid temperature fall"
                                         }
                                     }
                                 ],
                                 "value":"rapid_temperature_fall"
                             }
                        ]
                   }
              }
           ]
        },
       {
           "blockId":"0001-00000022",
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
                   "value": "897607_32770_1"
               },
               {
                    "name":"value",
                    "type":"token",
                    "options":[
                        {
                            "value":"no_changes",
                            "label":{
                                "lang_tag":"stop_detecting",
                                "text":"Stop detecting"
                            }
                        },
                        {
                            "value":"rapid_temperature_rise",
                            "label":{
                                "lang_tag":"rapid_temperature_rise",
                                "text":"Rapid temperature rise"
                            }
                        },
                        {
                            "value":"rapid_temperature_fall",
                            "label":{
                                "lang_tag":"rapid_temperature_fail",
                                "text":"Rapid temperature fall"
                            }
                        }
                    ],
                    "value":"rapid_temperature_fall"
                }
           ]
       }
    ]
}
)"_json;

const JsonObject createSceneForStatusGetRequest = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "someScene",
    "parent_id": "",
    "then": [],
    "when": [
        {
            "blockId":"0000",
            "blockOptions": {
                "method": {
                    "args": {
                        "item": "item",
                        "comparator": "comparator",
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
                    "value": "897607_32770_2"
                },
                {
                      "name":"comparator",
                      "type":"string",
                      "value": ">"
                },
                {
                    "name": "value",
                    "type": "float",
                    "value": 71.6,
                    "scale": "fahrenheit"
                }
            ]
        },
        {
            "blockId":"0001",
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
                        {
                            "blockId":"0002",
                            "blockOptions": {
                                "method": {
                                    "args": { "item": "item", "value": "value" },
                                    "name": "isItemState"
                                }
                            },
                            "blockType": "when",
                            "fields": [
                                {
                                    "name": "item",
                                    "type": "item",
                                    "value": "897607_32770_1"
                                },
                                {
                                    "name":"value",
                                    "type":"int",
                                    "value":4
                                }
                            ]
                        }
                    ]
                }
            ]
        },
        {
            "blockId":"0003",
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
                    "value": {
                        "blockId":"0004",
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
                                "value": "897607_32770_1"
                            },
                            {
                                "name":"value",
                                "type":"string",
                                "value":"some_random_string"
                            }
                        ]
                    }
                }
            ]
        },
        {
            "blockId":"0005",
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
                    "value": "897607_32770_1"
                },
                {
                    "name":"value",
                    "type":"int",
                    "value":4
                }
            ]
        }
    ],
    "exceptions": [
        {
            "blockId":"0006",
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
                        {
                            "blockId":"0007",
                            "blockOptions": {
                                "method": {
                                    "args": {
                                        "item": "item",
                                        "comparator": "comparator",
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
                                    "value": "897607_32770_2"
                                },
                                {
                                    "name":"comparator",
                                    "type":"string",
                                    "value": "<"
                                },
                                {
                                    "name": "value",
                                    "type": "float",
                                    "value": 71.6,
                                    "scale": "fahrenheit"
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

const JsonObject createSceneForStatusGetExpectedResult = R"(
{
    "_id": "__SCENE_ID__",
    "status": {
        "lastChange": null,
        "state": "false",
        "when": [
            {
                "_id": "__BLOCK0_ID__",
                "lastChange": null,
                "name": "compareNumbers",
                "state": "false"
            },
            {
                "_id": "__BLOCK1_ID__",
                "lastChange": null,
                "name": "and",
                "state": "false",
                "blocks": [
                    {
                        "_id": "__BLOCK2_ID__",
                        "lastChange": null,
                        "name": "isItemState",
                        "state": "false"
                    }
                ]
            },
            {
                "_id": "__BLOCK3_ID__",
                "lastChange": null,
                "name": "not",
                "state": "false",
                "blocks": [
                    {
                        "_id": "__BLOCK4_ID__",
                        "lastChange": null,
                        "name": "isItemState",
                        "state": "false"
                    }
                ]
            },
            {
                "_id": "__BLOCK5_ID__",
                "lastChange": null,
                "name": "isItemState",
                "state": "false"
            }
        ],
        "exceptions": [
            {
                "_id": "__BLOCK6_ID__",
                "lastChange": null,
                "name": "or",
                "state": "false",
                "blocks": [
                    {
                        "_id": "__BLOCK7_ID__",
                        "lastChange": null,
                        "name": "compareNumbers",
                        "state": "false"
                    }
                ]
            }
        ]
    }
}
)"_json;

const JsonObject editedScene = R"(
{
    "_id": "5c5318aa518af44041018347",
    "eo": {
       "_id": "5c5318aa518af44041018347",
       "enabled": true,
       "group_id": null,
       "is_group": false,
       "name": "NewR",
       "parent_id": "5c050abd518af4117b2e2496",
       "then": [
           {
               "blockOptions": {
                   "method": {
                       "args": {
                           "item": "item",
                           "value": "value"
                       },
                       "name": "decreaseDimmer"
                   }
               },
               "blockType": "then",
               "fields": [
                   {
                       "name": "item",
                       "type": "item",
                       "value": "897607_32771_1"
                   },
                   {
                       "name": "value",
                       "type": "number",
                       "value": 10
                   }
               ]
           }
       ],
       "when": [
           {
               "blockOptions": {
                   "method": {
                       "args": {
                           "item": "item",
                           "value": "value"
                       },
                       "name": "isItemState"
                   },
                   "trigger_object": {
                       "item": "item"
                   }
               },
               "blockType": "when",
               "fields": [
                   {
                       "name": "item",
                       "type": "item",
                       "value": "897607_32770_1"
                   },
                   {
                       "name": "value",
                       "type": "option",
                       "value": true
                   }
               ]
           }
       ]
    },
    "permission": {
       "devices": "s",
       "ezlo": "s",
       "rules": "s",
       "ui": "s",
       "users": "s"
    },
    "sender": "victory@ezlo.com",
    "serial": "0000000000897607"
}
)"_json;

const JsonObject createSceneWithScalesItemState = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "NewR",
    "parent_id": "5c050abd518af4117b2e2496",
    "permission": {
      "devices": "s",
      "ezlo": "s",
      "rules": "s",
      "ui": "s",
      "users": "s"
    },
    "sender": "victory@ezlo.com",
    "serial": "0000000000897607",
    "then": [],
    "when": [
      {
          "blockOptions": {
              "method": {
                  "args": {
                      "item": "item",
                      "value": "value"
                  },
                  "name": "isItemState"
              },
              "trigger_object": {
                  "item": "item"
              }
          },
          "blockType": "when",
          "fields": [
              {
                  "name": "item",
                  "type": "item",
                  "value": "897607_32770_2"
              },
              {
                  "name": "value",
                  "type": "float",
                  "value": 71.6,
                  "scale": "fahrenheit"
              }
          ]
      }
    ]
    }
)"_json;

const JsonObject createSceneWithScalesCompareNumbers = R"(
{
    "enabled": true,
    "group_id": null,
    "is_group": false,
    "name": "NewR",
    "parent_id": "5c050abd518af4117b2e2496",
    "permission": {
      "devices": "s",
      "ezlo": "s",
      "rules": "s",
      "ui": "s",
      "users": "s"
    },
    "sender": "victory@ezlo.com",
    "serial": "0000000000897607",
    "then": [],
    "when": [
      {
          "blockOptions": {
              "method": {
                  "args": {
                      "item": "item",
                        "comparator": "comparator",
                      "value": "value"
                  },
                  "name": "compareNumbers"
              },
              "trigger_object": {
                  "item": "item"
              }
          },
          "blockType": "when",
          "fields": [
              {
                  "name": "item",
                  "type": "item",
                  "value": "897607_32770_2"
              },
              {
                    "name":"comparator",
                    "type":"string",
                    "value": ">"
              },
              {
                  "name": "value",
                  "type": "float",
                  "value": 71.6,
                  "scale": "fahrenheit"
              }
          ]
      }
    ]
    }
)"_json;

JsonObject compareNumbersRangeCondition = R"(
{
   "blockOptions": {
      "method": {
         "args": { "item": "item", "comparator":"comparator", "startValue":"startValue", "endValue":"endValue" },
         "name": "compareNumberRange"
      }
   },
   "blockType": "when",
   "fields": [
        {
        "name":"item",
        "type":"item",
        "value":"__ITEM_ID__"
     },
     {
        "name":"comparator",
        "type":"string",
        "value":"between"
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
)"_json;

JsonObject firmwareUpdateStateCondition = R"(
{
   "blockOptions":{
         "method":{
            "name":"isFirmwareUpdateState",
            "args":{
               "state":"state"
            }
         }
   },
   "blockType": "when",
   "fields":[
         {
            "name":"state",
            "type":"token",
            "value":"started"
         }
      ]
}
)"_json;

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

const std::string logicalOperatorWhenBlock = R"(
{
   "blockType":"when",
   "blockOptions":{
      "method":{
         "name":"__OPERATOR__",
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

const std::string blockReferenceExceptionTemplate = R"(
{
    "exception": {
        "type": "block_reference",
        "blockId": "__BLOCK_ID__"
    }
}
)";

const std::string externalBlockReferenceExceptionTemplate = R"(
{
    "exception": {
        "type": "block_reference",
        "sceneId": "__SCENE_ID__",
        "blockId": "__BLOCK_ID__"
    }
}
)";

const std::string expressionReferenceExceptionTemplate = R"(
{
    "exception": {
        "type": "expression_reference",
        "name": "__NAME__"
    }
}
)";

const JsonObject createSceneWithGlobalRestriction = R"(
{
    "enabled": true,
    "group_id": "",
    "is_group": false,
    "name": "EmptyName",
    "parent_id": "",
    "then" : [],
    "when" : [],
    "exceptions": []
}
)"_json;

const JsonObject hubForwardNotificationAction = R"(
{
               "_id":"66fda597000000080c37e0d0",
               "blockOptions":{
                  "method":{
                     "args":{
                        "device":"device",
                        "item":"item",
                        "notificationTemplate":"notificationTemplate",
                        "targets":"targets",
                        "notificationBlock":"notificationBlock"
                     },
                     "name":"forwardNotifications"
                  }
               },
               "blockType":"then",
               "fields":[
                  {
                     "name":"notificationTemplate",
                     "type":"templateId",
                     "value":"templateId"
                  },
                  {
                     "name":"targets",
                     "type":"array",
                     "value":[
                        "1",
                        "2",
                        "3"
                     ]
                  },
                  {
                     "name":"device",
                     "type":"string",
                     "value":"66fda59700000008017ba846"
                  },
                  {
                     "name":"item",
                     "type":"string",
                     "value":"66fda59700000008017ba847"
                  },
                  {
                     "name":"notificationBlock",
                     "type":"object",
                     "value":{"key":"value"}
                  }
               ]
            }
)"_json;

}  // namespace UT
}  // namespace SceneEngine
