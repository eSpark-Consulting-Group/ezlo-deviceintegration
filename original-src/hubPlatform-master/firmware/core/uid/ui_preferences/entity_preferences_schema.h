#pragma once

#include "json_utils/json_wrapper.h"

#include <unordered_map>
#include <string>

class EntityPreferencesSchema {
public:
    void addPropertyInfo( const std::string& propertyName, const JsonObject& joPropertyInfo );
    JsonObject composeDefaultPreferences() const;

private:
    std::unordered_map< std::string, JsonObject > m_properties;
};
