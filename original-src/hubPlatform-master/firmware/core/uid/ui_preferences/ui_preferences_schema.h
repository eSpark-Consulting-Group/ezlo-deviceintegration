#pragma once

#include "entity_type.h"
#include "entity_preferences_schema.h"

#include <string>
#include <unordered_map>

#include <common_utils/cpp14.h>

class UIPreferencesSchema {
public:
    UIPreferencesSchema( const std::string& schemaFilePath );

    const EntityPreferencesSchema* getEntityPreferencesSchema( EntityType entityType,
                                                               const std::string& entitySubtype );

private:
    using EntityPreferenceSchemasMap = std::unordered_map< std::string, EntityPreferencesSchema >;

private:
    static EntityPreferencesSchema createEntityPreferencesSchema( const JsonObject& joEntitySchema );
    static EntityPreferenceSchemasMap createEntityPreferenceSchemas( const JsonObject& joEntitySchemas );

private:
    std::unordered_map< EntityType, EntityPreferenceSchemasMap > m_schema;
};
