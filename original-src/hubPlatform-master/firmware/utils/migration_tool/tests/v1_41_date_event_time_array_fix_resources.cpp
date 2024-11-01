#include "migration_tool/tests/v1_41_date_event_time_array_fix_resources.h"

namespace HubPlatform {
namespace UT {
namespace v1_41_date_event_time_array_fix {

const Json simpleSceneWithErrorBlock = R"(
{
  "_id": "some_id_simple_case",
  "enabled": true,
  "group_id": null,
  "is_group": false,
  "name": "simple_case",
  "parent_id": "",
  "then": [],
  "user_notifications": [],
  "when": [
    {
      "blockOptions": {
        "method": {
          "args": {
            "time": "time",
            "type": "type",
            "weekdays": "weekdays"
          },
          "name": "isDate"
        }
      },
      "blockType": "when",
      "fields": [
        {
          "name": "type",
          "type": "string",
          "value": "weekly"
        },
        {
          "name": "time",
          "type": "24_hours_time_array",
          "value": {
            "bool": true,
            "item": 1
          }
        },
        {
          "name": "weekdays",
          "type": "int_array",
          "value": [ 1, 3 ]
        }
      ]
    }
  ]
})"_json;

const Json fixedSimpleScene = R"(
{
  "_id": "some_id_simple_case",
  "enabled": true,
  "group_id": null,
  "is_group": false,
  "name": "simple_case",
  "parent_id": "",
  "then": [],
  "user_notifications": [],
  "when": [
    {
      "blockOptions": {
        "method": {
          "args": {
            "time": "time",
            "type": "type",
            "weekdays": "weekdays"
          },
          "name": "isDate"
        }
      },
      "blockType": "when",
      "fields": [
        {
          "name": "type",
          "type": "string",
          "value": "weekly"
        },
        {
          "name": "time",
          "type": "24_hours_time_array",
          "value": []
        },
        {
          "name": "weekdays",
          "type": "int_array",
          "value": [ 1, 3 ]
        }
      ]
    }
  ]
})"_json;

const Json complicatedSceneWithErrorBlocks = R"(
{
  "_id": "some_id_complicated_case",
  "enabled": true,
  "group_id": null,
  "is_group": false,
  "name": "complicated_case",
  "parent_id": null,
  "then": [],
  "user_notifications": [],
  "when": [
    {
      "blockOptions": {
        "method": {
          "args": { "blocks": "blocks" },
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
              "blockName": "or_and",
              "blockOptions": {
                "method": {
                  "args": { "blocks": "blocks" },
                  "name": "and"
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
                            "time": "time",
                            "type": "type",
                            "weekdays": "weekdays"
                          },
                          "name": "isDate"
                        }
                      },
                      "blockType": "when",
                      "fields": [
                        {
                          "name": "type",
                          "type": "string",
                          "value": "weekly"
                        },
                        {
                          "name": "time",
                          "type": "24_hours_time_array",
                          "value": { "bool": true, "item": 1 }
                        },
                        {
                          "name": "weekdays",
                          "type": "int_array",
                          "value": [ 2, 3, 5 ]
                        }
                      ]
                    },
                    {
                      "blockName": "or_and_xor",
                      "blockOptions": {
                        "method": {
                          "args": { "blocks": "blocks" },
                          "name": "xor"
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
                                    "days": "days",
                                    "time": "time",
                                    "type": "type"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "monthly"
                                },
                                {
                                  "name": "time",
                                  "type": "24_hours_time_array",
                                  "value": { "bool": true, "item": 1 }
                                },
                                {
                                  "name": "days",
                                  "type": "int_array",
                                  "value": [ 8, 9, 17, 30 ]
                                }
                              ]
                            },
                            {
                              "blockOptions": {
                                "function": {
                                  "follow": { "delayReset": 2 }
                                },
                                "method": {
                                  "args": { "blocks": "blocks" },
                                  "name": "function"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "blocks",
                                  "type": "blocks",
                                  "value": [
                                    {
                                      "blockName": "or_and_follow",
                                      "blockOptions": {
                                        "method": {
                                          "args": { "blocks": "blocks" },
                                          "name": "and"
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
                                                    "time": "time",
                                                    "type": "type",
                                                    "weekdays": "weekdays"
                                                  },
                                                  "name": "isDate"
                                                }
                                              },
                                              "blockType": "when",
                                              "fields": [
                                                {
                                                  "name": "type",
                                                  "type": "string",
                                                  "value": "weekly"
                                                },
                                                {
                                                  "name": "time",
                                                  "type": "24_hours_time_array",
                                                  "value": { "bool": true, "item": 1 }
                                                },
                                                {
                                                  "name": "weekdays",
                                                  "type": "int_array",
                                                  "value": [ 2, 3, 4, 5 ]
                                                }
                                              ]
                                            },
                                            {
                                              "blockOptions": {
                                                "method": {
                                                  "args": { "block": "block" },
                                                  "name": "not"
                                                }
                                              },
                                              "blockType": "when",
                                              "fields": [
                                                {
                                                  "name": "block",
                                                  "type": "block",
                                                  "value": {
                                                    "blockOptions": {
                                                      "method": {
                                                        "args": {
                                                          "time": "time",
                                                          "type": "type",
                                                          "weekdays": "weekdays"
                                                        },
                                                        "name": "isDate"
                                                      }
                                                    },
                                                    "blockType": "when",
                                                    "fields": [
                                                      {
                                                        "name": "type",
                                                        "type": "string",
                                                        "value": "weekly"
                                                      },
                                                      {
                                                        "name": "time",
                                                        "type": "24_hours_time_array",
                                                        "value": { "bool": true, "item": 1 }
                                                      },
                                                      {
                                                        "name": "weekdays",
                                                        "type": "int_array",
                                                        "value": [ 2, 4, 5 ]
                                                      }
                                                    ]
                                                  }
                                                }
                                              ]
                                            }
                                          ]
                                        }
                                      ]
                                    }
                                  ]
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            },
            {
              "blockName": "or_or",
              "blockOptions": {
                "method": {
                  "args": { "blocks": "blocks" },
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
                      "blockName": "or_or_and",
                      "blockOptions": {
                        "method": {
                          "args": { "blocks": "blocks" },
                          "name": "and"
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
                                    "time": "time",
                                    "type": "type",
                                    "weekdays": "weekdays"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "weekly"
                                },
                                {
                                  "name": "time",
                                  "type": "24_hours_time_array",
                                  "value": { "bool": true, "item": 1 }
                                },
                                {
                                  "name": "weekdays",
                                  "type": "int_array",
                                  "value": [ 2, 4, 5 ]
                                }
                              ]
                            },
                            {
                              "blockOptions": {
                                "method": {
                                  "args": { "block": "block" },
                                  "name": "not"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "block",
                                  "type": "block",
                                  "value": {
                                    "blockOptions": {
                                      "method": {
                                        "args": {
                                          "days": "days",
                                          "time": "time",
                                          "type": "type"
                                        },
                                        "name": "isDate"
                                      }
                                    },
                                    "blockType": "when",
                                    "fields": [
                                      {
                                        "name": "type",
                                        "type": "string",
                                        "value": "monthly"
                                      },
                                      {
                                        "name": "time",
                                        "type": "24_hours_time_array",
                                        "value": { "bool": true, "item": 1 }
                                      },
                                      {
                                        "name": "days",
                                        "type": "int_array",
                                        "value": [ 6, 9, 17, 18, 19, 20 ]
                                      }
                                    ]
                                  }
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    },
                    {
                      "blockName": "or_or_and_2",
                      "blockOptions": {
                        "method": {
                          "args": { "blocks": "blocks" },
                          "name": "and"
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
                                    "days": "days",
                                    "time": "time",
                                    "type": "type"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "monthly"
                                },
                                {
                                  "name": "time",
                                  "type": "24_hours_time_array",
                                  "value": { "bool": true, "item": 1 }
                                },
                                {
                                  "name": "days",
                                  "type": "int_array",
                                  "value": [ 9, 15, 17, 18, 19, 29, 30, 31 ]
                                }
                              ]
                            },
                            {
                              "blockOptions": {
                                "method": {
                                  "args": {
                                    "time": "time",
                                    "type": "type",
                                    "weeks": "weeks"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "weeks"
                                },
                                {
                                  "name": "weeks",
                                  "type": "int_array",
                                  "value": [ 2, 3, 4 ]
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          ]
        }
      ]
    }
  ]
})"_json;

const Json fixedComplicatedScene = R"(
{
  "_id": "some_id_complicated_case",
  "enabled": true,
  "group_id": null,
  "is_group": false,
  "name": "complicated_case",
  "parent_id": null,
  "then": [],
  "user_notifications": [],
  "when": [
    {
      "blockOptions": {
        "method": {
          "args": { "blocks": "blocks" },
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
              "blockName": "or_and",
              "blockOptions": {
                "method": {
                  "args": { "blocks": "blocks" },
                  "name": "and"
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
                            "time": "time",
                            "type": "type",
                            "weekdays": "weekdays"
                          },
                          "name": "isDate"
                        }
                      },
                      "blockType": "when",
                      "fields": [
                        {
                          "name": "type",
                          "type": "string",
                          "value": "weekly"
                        },
                        {
                          "name": "time",
                          "type": "24_hours_time_array",
                          "value": []
                        },
                        {
                          "name": "weekdays",
                          "type": "int_array",
                          "value": [ 2, 3, 5 ]
                        }
                      ]
                    },
                    {
                      "blockName": "or_and_xor",
                      "blockOptions": {
                        "method": {
                          "args": { "blocks": "blocks" },
                          "name": "xor"
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
                                    "days": "days",
                                    "time": "time",
                                    "type": "type"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "monthly"
                                },
                                {
                                  "name": "time",
                                  "type": "24_hours_time_array",
                                  "value": []
                                },
                                {
                                  "name": "days",
                                  "type": "int_array",
                                  "value": [ 8, 9, 17, 30 ]
                                }
                              ]
                            },
                            {
                              "blockOptions": {
                                "function": {
                                  "follow": { "delayReset": 2 }
                                },
                                "method": {
                                  "args": { "blocks": "blocks" },
                                  "name": "function"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "blocks",
                                  "type": "blocks",
                                  "value": [
                                    {
                                      "blockName": "or_and_follow",
                                      "blockOptions": {
                                        "method": {
                                          "args": { "blocks": "blocks" },
                                          "name": "and"
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
                                                    "time": "time",
                                                    "type": "type",
                                                    "weekdays": "weekdays"
                                                  },
                                                  "name": "isDate"
                                                }
                                              },
                                              "blockType": "when",
                                              "fields": [
                                                {
                                                  "name": "type",
                                                  "type": "string",
                                                  "value": "weekly"
                                                },
                                                {
                                                  "name": "time",
                                                  "type": "24_hours_time_array",
                                                  "value": []
                                                },
                                                {
                                                  "name": "weekdays",
                                                  "type": "int_array",
                                                  "value": [ 2, 3, 4, 5 ]
                                                }
                                              ]
                                            },
                                            {
                                              "blockOptions": {
                                                "method": {
                                                  "args": { "block": "block" },
                                                  "name": "not"
                                                }
                                              },
                                              "blockType": "when",
                                              "fields": [
                                                {
                                                  "name": "block",
                                                  "type": "block",
                                                  "value": {
                                                    "blockOptions": {
                                                      "method": {
                                                        "args": {
                                                          "time": "time",
                                                          "type": "type",
                                                          "weekdays": "weekdays"
                                                        },
                                                        "name": "isDate"
                                                      }
                                                    },
                                                    "blockType": "when",
                                                    "fields": [
                                                      {
                                                        "name": "type",
                                                        "type": "string",
                                                        "value": "weekly"
                                                      },
                                                      {
                                                        "name": "time",
                                                        "type": "24_hours_time_array",
                                                        "value": []
                                                      },
                                                      {
                                                        "name": "weekdays",
                                                        "type": "int_array",
                                                        "value": [ 2, 4, 5 ]
                                                      }
                                                    ]
                                                  }
                                                }
                                              ]
                                            }
                                          ]
                                        }
                                      ]
                                    }
                                  ]
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            },
            {
              "blockName": "or_or",
              "blockOptions": {
                "method": {
                  "args": { "blocks": "blocks" },
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
                      "blockName": "or_or_and",
                      "blockOptions": {
                        "method": {
                          "args": { "blocks": "blocks" },
                          "name": "and"
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
                                    "time": "time",
                                    "type": "type",
                                    "weekdays": "weekdays"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "weekly"
                                },
                                {
                                  "name": "time",
                                  "type": "24_hours_time_array",
                                  "value": []
                                },
                                {
                                  "name": "weekdays",
                                  "type": "int_array",
                                  "value": [ 2, 4, 5 ]
                                }
                              ]
                            },
                            {
                              "blockOptions": {
                                "method": {
                                  "args": { "block": "block" },
                                  "name": "not"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "block",
                                  "type": "block",
                                  "value": {
                                    "blockOptions": {
                                      "method": {
                                        "args": {
                                          "days": "days",
                                          "time": "time",
                                          "type": "type"
                                        },
                                        "name": "isDate"
                                      }
                                    },
                                    "blockType": "when",
                                    "fields": [
                                      {
                                        "name": "type",
                                        "type": "string",
                                        "value": "monthly"
                                      },
                                      {
                                        "name": "time",
                                        "type": "24_hours_time_array",
                                        "value": []
                                      },
                                      {
                                        "name": "days",
                                        "type": "int_array",
                                        "value": [ 6, 9, 17, 18, 19, 20 ]
                                      }
                                    ]
                                  }
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    },
                    {
                      "blockName": "or_or_and_2",
                      "blockOptions": {
                        "method": {
                          "args": { "blocks": "blocks" },
                          "name": "and"
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
                                    "days": "days",
                                    "time": "time",
                                    "type": "type"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "monthly"
                                },
                                {
                                  "name": "time",
                                  "type": "24_hours_time_array",
                                  "value": []
                                },
                                {
                                  "name": "days",
                                  "type": "int_array",
                                  "value": [ 9, 15, 17, 18, 19, 29, 30, 31 ]
                                }
                              ]
                            },
                            {
                              "blockOptions": {
                                "method": {
                                  "args": {
                                    "time": "time",
                                    "type": "type",
                                    "weeks": "weeks"
                                  },
                                  "name": "isDate"
                                }
                              },
                              "blockType": "when",
                              "fields": [
                                {
                                  "name": "type",
                                  "type": "string",
                                  "value": "weeks"
                                },
                                {
                                  "name": "weeks",
                                  "type": "int_array",
                                  "value": [ 2, 3, 4 ]
                                }
                              ]
                            }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          ]
        }
      ]
    }
  ]
})"_json;

}  // namespace v1_41_date_event_time_array_fix
}  // namespace UT
}  // namespace HubPlatform
