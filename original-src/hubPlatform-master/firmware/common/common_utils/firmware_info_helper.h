#pragma once

#include "base_utils/firmware_version.h"

#include <string>

namespace FirmwareInfoHelper {

const HubPlatform::Firmware::Version& getFirmwareVersion();
std::string getFirmwareVersionString();
std::string getSystemVersionString();
std::string getFirmwareRegion();

}  // namespace FirmwareInfoHelper
