#include "common_test_resources/resources/cored_resources.h"

namespace HubPlatform {
namespace Tests {

const std::string hubItemDictionaryValueAdd = R"(
{
    "method": "hub.item.dictionary.value.add",
    "id": "__ID__",
    "params": {
        "_id" : "__ITEM_ID__",
        "element": {
            "type": "__TYPE__",
            "value": __VALUE__
        }
    }
}
)";

const std::string hubItemDictionaryValueSet = R"(
{
    "method": "hub.item.dictionary.value.set",
    "id": "__ID__",
    "params": {
        "_id" : "__ITEM_ID__",
        "key" : "__KEY__",
        "element": {
            "type": "__TYPE__",
            "value": __VALUE__
        }
    }
}
)";

const std::string hubItemDictionaryClear = R"(
{
  "method": "hub.item.dictionary.clear",
  "id": "__ID__",
  "params": {
      "_id" : "__ITEM_ID__"
  }
}
)";

const std::string hubItemDictionaryValueRemove = R"(
{
    "method": "hub.item.dictionary.value.remove",
    "id": "__ID__",
    "params": {
        "_id" : "__ITEM_ID__",
        "key" : "__KEY__"
    }
}
)";

const std::string hubDevicesListRequest = R"(
{
    "method": "hub.devices.list",
    "id": "__ID__",
    "params": {}
}
)";

const std::string hubItemsListRequest = R"(
{
    "method": "hub.items.list",
    "id": "__ID__",
    "params": {}
}
)";

const std::string hubDataListRequest = R"(
{
    "api": "1.0",
    "method": "hub.data.list",
    "id": "__ID__",
    "params": {
        __PARAMS__
    }
}
)";

const std::string hubExtensionsPluginRun = R"(
{
    "method": "hub.extensions.plugin.run",
    "id": "__ID__",
    "params": {
        "script": "__SCRIPT__"
    }
}
)";

const std::string hubExtensionsPluginRunWithParams = R"(
{
    "method": "hub.extensions.plugin.run",
    "id": "__ID__",
    "params": {
        "script": "__SCRIPT__",
        "scriptParams": __PARAMS__
    }
}
)";

}  // namespace Tests
}  // namespace HubPlatform
