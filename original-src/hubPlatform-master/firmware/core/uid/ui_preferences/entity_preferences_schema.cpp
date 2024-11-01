#include "entity_preferences_schema.h"

void EntityPreferencesSchema::addPropertyInfo( const std::string& propertyName, const JsonObject& joPropertyInfo ) {
    m_properties[ propertyName ] = joPropertyInfo;
}

JsonObject EntityPreferencesSchema::composeDefaultPreferences() const {
    auto joPreferences = JSON_EMPTY_OBJ;
    for( const auto& entry : m_properties ) {
        const std::string& propertyName = entry.first;
        const JsonObject& joPropertyInfo = entry.second;

        joPreferences[ propertyName ] = joPropertyInfo[ "defaultValue" ];
    }

    return joPreferences;
}
