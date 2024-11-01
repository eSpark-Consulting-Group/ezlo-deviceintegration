#pragma once

#include <string>
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "cored/model/values/values.h"
#include "group.h"

namespace HubPlatform {
namespace CoreD {

class ItemGroupInfo : public GroupInfo {
public:
    ItemGroupInfo() = default;
    ItemGroupInfo( std::string id,
                   std::string name,
                   bool hasGetter,
                   bool hasSetter,
                   bool persistent,
                   JsonArray itemNames,
                   std::string valueType,
                   JsonArray theEnum,
                   std::string valueTypeFamily );

public:
    void setId( std::string id );
    const std::string& getId() const;
    const std::string& getName() const;
    bool getHasGetter() const;
    bool getHasSetter() const;
    bool getPersistent() const;
    const JsonArray& getItemNames() const;
    CoreD::Model::ValueType getValueType() const;
    const JsonArray& getEnum() const;
    const std::string& getValueTypeFamily() const;
    const std::string& getRole() const;
    const JsonObject& getInfoObject() const;

public:
    Json toJson() const override;
    bool fromJson( const JsonObject& data ) override;

private:
    std::string m_id;
    std::string m_name;
    bool m_hasGetter{ false };
    bool m_hasSetter{ false };
    bool m_persistent{ false };
    JsonArray m_itemNames;
    std::string m_valueType;
    JsonArray m_enum;
    std::string m_valueTypeFamily;
    std::string m_role;
    JsonObject m_info;
};

}  // namespace CoreD
}  // namespace HubPlatform
