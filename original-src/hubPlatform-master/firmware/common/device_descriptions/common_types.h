#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include "device_id.h"
#include "device_configuration.h"

class DeviceInfo;

using DeviceInfoPtr = std::shared_ptr< DeviceInfo >;
using DeviceConfigurations = std::vector< DeviceConfiguration >;
using DeviceDescriptions = std::map< DeviceId, DeviceInfoPtr >;
using ManufacturersNames = std::map< DeviceId::ManufacturersEnumType, const std::string >;
using DeviceInfoCommonList = std::map< int, const std::string >;
using DeviceTypesList = DeviceInfoCommonList;
using ItemSubTypesList = DeviceInfoCommonList;
using DeviceSubTypesList = DeviceInfoCommonList;
using ItemType = int;
using InstanceType = int;
using ItemsSubTypesList = std::vector< std::pair< ItemType, InstanceType > >;
using FilenameType = std::string;
using DevicesShowAsRuleExcludedItems = std::map< std::string, std::vector< std::string > >;
using DevicesHasGetter = std::map< std::string, bool >;
using DevicesHasSetter = std::map< std::string, bool >;
