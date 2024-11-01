
#include "device_descriptions/device_info.h"
#include "device_descriptions/device_configuration.h"

DeviceInfo::DeviceInfo( const std::string& name,
                        const std::string& model,
                        int deviceType,
                        int deviceSubType,
                        const ItemsSubTypesList&& itemsSubtypes,
                        const std::string&& mainItemName,
                        const DeviceConfigurations& configurations,
                        bool hasExtraFields,
                        bool hasGetter,
                        bool hasSetter,
                        const JsonArray& showAsRuleExcludedFields )
: m_name( name )
, m_model( model )
, m_deviceType( deviceType )
, m_deviceSubType( deviceSubType )
, m_itemsSubtypes( std::move( itemsSubtypes ) )
, m_mainItemName( std::move( mainItemName ) )
, m_deviceConfigurations( configurations )
, m_hasExtraFields{ hasExtraFields }
, m_hasGetter{ hasGetter }
, m_hasSetter{ hasGetter }
, m_showAsRuleExcludedFields{ showAsRuleExcludedFields } {}

void DeviceInfo::setConfigurations( DeviceConfigurations&& configuration ) {
    m_deviceConfigurations = std::move( configuration );
}

const std::string& DeviceInfo::getName() const {
    return m_name;
}

const std::string& DeviceInfo::getModel() const {
    return m_model;
}

int DeviceInfo::getType() const {
    return m_deviceType;
}

int DeviceInfo::getSubType() const {
    return m_deviceSubType;
}

const DeviceInfo::ItemsSubTypesList& DeviceInfo::getItemsSubTypes() const {
    return m_itemsSubtypes;
}

const std::string& DeviceInfo::getMainItemName() const {
    return m_mainItemName;
}

const DeviceConfigurations& DeviceInfo::getConfigurations() const {
    return m_deviceConfigurations;
}

bool DeviceInfo::hasExtraFields() const {
    return m_hasExtraFields;
}

bool DeviceInfo::hasGetter() const {
    return m_hasGetter;
}

bool DeviceInfo::hasSetter() const {
    return m_hasSetter;
}

JsonArray DeviceInfo::getShowAsRuleExcludedFields() const {
    return m_showAsRuleExcludedFields;
}