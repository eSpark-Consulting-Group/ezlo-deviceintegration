#include "ui_preferences_schema.h"
#include "log_utils/ezlo_log.h"

#include <fstream>

UIPreferencesSchema::UIPreferencesSchema( const std::string& schemaFilePath ) {
    JsonObject joSchema = JSON_EMPTY_OBJ;

    try {
        std::ifstream in( schemaFilePath );
        in >> joSchema;
    }
    catch( ... ) {
        joSchema = JSON_EMPTY_OBJ;

        Log::error( "Can't parse schema file: " + schemaFilePath );
    }

    if( !joSchema.empty() ) {
        for( auto it = joSchema.cbegin(), end = joSchema.cend(); it != end; ++it ) {
            const auto& entityTypeStr = it.key();

            const EntityType entityType = strToEntityType( entityTypeStr );
            if( entityType != EntityType::INVALID ) {
                m_schema[ entityType ] = createEntityPreferenceSchemas( it.value() );
            }
        }
    }
}

UIPreferencesSchema::EntityPreferenceSchemasMap UIPreferencesSchema::createEntityPreferenceSchemas(
    const JsonObject& joEntitySchemas ) {
    EntityPreferenceSchemasMap entitySchemas;
    for( auto it = joEntitySchemas.cbegin(), end = joEntitySchemas.cend(); it != end; ++it ) {
        entitySchemas[ it.key() ] = createEntityPreferencesSchema( it.value() );
    }

    return entitySchemas;
};

EntityPreferencesSchema UIPreferencesSchema::createEntityPreferencesSchema( const JsonObject& joEntitySchema ) {
    EntityPreferencesSchema entityPreferencesSchema;
    for( auto it = joEntitySchema.begin(), end = joEntitySchema.end(); it != end; ++it ) {
        entityPreferencesSchema.addPropertyInfo( it.key(), it.value() );
    }

    return entityPreferencesSchema;
}

const EntityPreferencesSchema* UIPreferencesSchema::getEntityPreferencesSchema( EntityType entityType,
                                                                                const std::string& entitySubtype ) {
    const auto& entitySchemas = m_schema[ entityType ];
    auto it = entitySchemas.find( entitySubtype );
    if( it != entitySchemas.end() )
        return &it->second;

    return nullptr;
}
