#pragma once

#include "entity_type.h"
#include "ui_preferences_schema.h"

#include "base_utils/non_copyable.h"

#include <string>
#include <unordered_map>

class UIPreferencesManager : public common::NonCopyable {
public:
    static UIPreferencesManager& getInstance();

    void setPreferences( EntityType entityType, const std::string& id, const JsonObject& preferences );
    JsonObject getPreferencesEntry( EntityType entityType, const std::string& id );
    std::vector< JsonObject > getAllPreferenceEntries( EntityType entityType );

private:
    UIPreferencesManager();

    std::vector< std::string > updatePreferenceProperties( EntityType entityType,
                                                           const std::string& id,
                                                           const JsonObject& preferences );
    void updateDbCache( EntityType entityType );
    void updateDbCache( EntityType entityType, const std::string& id );

    static std::vector< std::string > getAvailableEntityIds( EntityType entityType );
    static bool isEntityAvailable( EntityType entityType, const std::string& id );
    JsonObject composeDefaultPreferencesEntry( EntityType entityType, const std::string& id );
    JsonObject composeDefaultPreferences( EntityType entityType, const std::string& id );

private:
    std::unordered_map< EntityType, std::unordered_map< std::string, JsonObject >, EntityTypeHash > m_dbCache;
    UIPreferencesSchema m_uiPreferencesSchema;
};
