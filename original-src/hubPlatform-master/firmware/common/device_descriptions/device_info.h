#pragma once

#include "device_descriptions/zwave_definitions.h"
#include "device_descriptions/common_types.h"

#include "json_utils/json_wrapper.h"
#include "base_utils/non_copyable.h"

#include <string>
#include <unordered_map>

class DeviceInfo final : public common::NonCopyable {
public:
    using InstanceType = int;
    using ItemType = int;
    using ItemsSubTypesList = std::unordered_map< InstanceType, ItemType >;

public:
    DeviceInfo( const std::string& name,
                const std::string& model,
                int deviceType = DEVICE_TYPE_DEVICE,
                int deviceSubType = 0,
                const ItemsSubTypesList&& itemsSubtypes = ItemsSubTypesList(),
                const std::string&& mainItemName = "",
                const DeviceConfigurations& configurations = DeviceConfigurations(),
                bool hasExtraFields = false,
                bool hasGetter = false,
                bool hasSetter = false,
                const JsonArray& showAsRuleExcludedFields = JSON_EMPTY_ARR );

public:
    void setConfigurations( DeviceConfigurations&& configuration );

public:
    const std::string& getName() const;
    const std::string& getModel() const;
    int getType() const;
    int getSubType() const;
    const ItemsSubTypesList& getItemsSubTypes() const;
    const std::string& getMainItemName() const;
    const DeviceConfigurations& getConfigurations() const;
    bool hasExtraFields() const;
    bool hasGetter() const;
    bool hasSetter() const;
    JsonArray getShowAsRuleExcludedFields() const;

private:
    std::string m_name;
    std::string m_model;
    int m_deviceType;
    int m_deviceSubType;
    ItemsSubTypesList m_itemsSubtypes;
    std::string m_mainItemName;
    DeviceConfigurations m_deviceConfigurations;
    bool m_hasExtraFields;
    bool m_hasGetter;
    bool m_hasSetter;
    JsonArray m_showAsRuleExcludedFields;
};
