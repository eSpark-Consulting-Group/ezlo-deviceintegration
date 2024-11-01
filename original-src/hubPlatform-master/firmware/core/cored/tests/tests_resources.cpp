
#include "cored/tests/tests_resources.h"

namespace HubPlatform {
namespace UT {
namespace resources {
namespace daw {

const JsonObject SourceJson =
    R"(
        {
            "categories": [
                {
                    "category": "ALARMS",
                    "languageTag": "language_tag_0001",
                    "iconTag": "icon_tag_0001",
                    "devices": [
                        {
                            "deviceID": "1000",
                            "manufacturer" : "Aeon",
                            "model" : "DSD31",
                            "name": "Siren",
                            "pluginID": "2120"
                        }
                    ]
                },
                {
                    "category": "SENSORS",
                    "languageTag": "language_tag_0002",
                    "iconTag": "icon_tag_0002",
                    "devices": [
                        {
                            "deviceID": "1001",
                            "manufacturer" : "Fibaro",
                            "model" : "FGMS-001",
                            "name": "3 in 1 sensor",
                            "pluginID": "2121"
                        },
                        {
                            "deviceID": "1002",
                            "manufacturer" : "Philio",
                            "model" : "PSP05",
                            "name": "Motion Sensor",
                            "pluginID": "2122"
                        }
                    ]
                }
            ]
        })"_json;

const JsonObject CategoriesList =
    R"(
            {
                "result": [
                    {
                        "category": "ALARMS",
                        "languageTag": "language_tag_0001",
                        "iconTag": "icon_tag_0001"
                    },
                    {
                        "category": "SENSORS",
                        "languageTag": "language_tag_0002",
                        "iconTag": "icon_tag_0002"
                    }
                ]
            })"_json;

const JsonObject DeviceList =
    R"(
            {
                "result": [
                    {
                        "deviceID": "1000",
                        "manufacturer" : "Aeon",
                        "model" : "DSD31",
                        "name": "Siren",
                        "pluginID": "2120",
                        "wizardCategory": "ALARMS"
                    },
                    {
                        "deviceID": "1001",
                        "manufacturer" : "Fibaro",
                        "model" : "FGMS-001",
                        "name": "3 in 1 sensor",
                        "pluginID": "2121",
                        "wizardCategory": "SENSORS"
                    },
                    {
                        "deviceID": "1002",
                        "manufacturer" : "Philio",
                        "model" : "PSP05",
                        "name": "Motion Sensor",
                        "pluginID": "2122",
                        "wizardCategory": "SENSORS"
                    }
                ]
            })"_json;

const JsonObject AlarmDeviceList =
    R"(
            {
                "result": [
                    {
                        "deviceID": "1000",
                        "manufacturer" : "Aeon",
                        "model" : "DSD31",
                        "name": "Siren",
                        "pluginID": "2120",
                        "wizardCategory": "ALARMS"
                    }
                ]
            })"_json;
};  // namespace daw

const char FileDeviceAddingWizard[] = "/tmp/DeviceAddingWizard.json";

}  // namespace resources
}  // namespace UT
}  // namespace HubPlatform