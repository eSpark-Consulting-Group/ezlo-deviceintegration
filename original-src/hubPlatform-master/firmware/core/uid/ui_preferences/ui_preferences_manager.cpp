
#include "uid/ui_preferences/ui_preferences_manager.h"
#include "uid/fo_converter.h"

#include "database_client/db_tables_names.h"
#include "base_utils/ezlodb.h"

#include "bus/messagesclasses.h"
#include "uid/broadcast.h"

UIPreferencesManager::UIPreferencesManager()
: m_uiPreferencesSchema( "/opt/firmware/config/UIPreferencesSchema.json" ) {
    auto allPreferenceEntries = Db::findAll( HaUiPreferencesDBName, JSON_EMPTY_OBJ );
    for( auto& preferenceEntry : allPreferenceEntries ) {
        const std::string entityTypeStr = preferenceEntry[ "entityType" ];

        const EntityType entityType = strToEntityType( entityTypeStr );
        if( entityType != EntityType::INVALID ) {
            const std::string id = preferenceEntry[ "_id" ];
            m_dbCache[ entityType ][ id ] = std::move( preferenceEntry );
        }
    }
}

UIPreferencesManager& UIPreferencesManager::getInstance() {
    static UIPreferencesManager instance;
    return instance;
}

void UIPreferencesManager::setPreferences( EntityType entityType,
                                           const std::string& id,
                                           const JsonObject& preferences ) {
    auto changedProperties = updatePreferenceProperties( entityType, id, preferences );
    if( !changedProperties.empty() ) {
        JsonObject message;
        message[ "entityType" ] = entityTypeToStr( entityType );
        message[ "_id" ] = id;
        message[ "properties" ] = std::move( changedProperties );

        Broadcast::sendToUis( UI_PREFERENCE_PROPERTIES_CHANGED, message );
    }
}

std::vector< std::string > UIPreferencesManager::updatePreferenceProperties( EntityType entityType,
                                                                             const std::string& id,
                                                                             const JsonObject& preferences ) {
    std::vector< std::string > changedPropNames;

    auto preferencesEntry = getPreferencesEntry( entityType, id );
    if( !preferencesEntry.empty() ) {
        JsonObject& joPreferences = preferencesEntry[ "preferences" ];
        for( auto it = preferences.begin(), end = preferences.end(); it != end; ++it ) {
            auto propName = it.key();
            if( joPreferences.count( propName ) != 0 ) {
                auto joNewPropValue = it.value();
                auto joOldPropValue = joPreferences[ propName ];

                if( joNewPropValue.type() == joOldPropValue.type() && joNewPropValue != joOldPropValue ) {
                    joPreferences[ propName ] = std::move( joNewPropValue );
                    changedPropNames.push_back( std::move( propName ) );
                }
            }
        }
    }

    if( !changedPropNames.empty() ) {
        const auto query = JSON( "entityType" << entityTypeToStr( entityType ) << "_id" << id );
        Db::update( HaUiPreferencesDBName, query, JSON( "$set" << preferencesEntry ) );

        m_dbCache[ entityType ][ id ] = std::move( preferencesEntry );
    }

    return changedPropNames;
}

std::vector< JsonObject > UIPreferencesManager::getAllPreferenceEntries( EntityType entityType ) {
    updateDbCache( entityType );

    const auto& preferenceEntries = m_dbCache[ entityType ];

    std::vector< JsonObject > allEntries;
    allEntries.reserve( preferenceEntries.size() );

    std::transform( preferenceEntries.cbegin(),
                    preferenceEntries.cend(),
                    std::back_inserter( allEntries ),
                    []( const std::pair< std::string, JsonObject >& pair ) { return pair.second; } );

    return allEntries;
}

JsonObject UIPreferencesManager::getPreferencesEntry( EntityType entityType, const std::string& id ) {
    updateDbCache( entityType, id );
    return m_dbCache[ entityType ][ id ];
}

void UIPreferencesManager::updateDbCache( EntityType entityType, const std::string& id ) {
    auto& preferenceEntries = m_dbCache[ entityType ];
    if( isEntityAvailable( entityType, id ) ) {
        if( preferenceEntries.find( id ) == preferenceEntries.end() ) {
            preferenceEntries[ id ] = composeDefaultPreferencesEntry( entityType, id );
        }
    }
    else {
        preferenceEntries.erase( id );
    }
}

void UIPreferencesManager::updateDbCache( EntityType entityType ) {
    auto& preferenceEntries = m_dbCache[ entityType ];
    auto tmpPreferenceEntries = std::move( preferenceEntries );

    for( const auto& id : getAvailableEntityIds( entityType ) ) {
        preferenceEntries[ id ] = ( tmpPreferenceEntries.find( id ) == tmpPreferenceEntries.end() ) ?
                                      composeDefaultPreferencesEntry( entityType, id ) :
                                      std::move( tmpPreferenceEntries[ id ] );
    }
}

std::vector< std::string > UIPreferencesManager::getAvailableEntityIds( EntityType entityType ) {
    std::vector< std::string > ids;

    switch( entityType ) {
    case EntityType::ITEM: {
        const auto items = Fo::getItems();
        ids.reserve( items.size() );

        for( auto& joItem : items ) {
            ids.push_back( joItem[ "_id" ] );
        }
    } break;
    }

    return ids;
}

bool UIPreferencesManager::isEntityAvailable( EntityType entityType, const std::string& id ) {
    switch( entityType ) {
    case EntityType::ITEM:
        return Fo::hasItem( id );
    }

    return false;
}

JsonObject UIPreferencesManager::composeDefaultPreferencesEntry( EntityType entityType, const std::string& id ) {
    JsonObject preferencesEntry = JSON_EMPTY_OBJ;
    preferencesEntry[ "entityType" ] = entityTypeToStr( entityType );
    preferencesEntry[ "_id" ] = id;
    preferencesEntry[ "preferences" ] = composeDefaultPreferences( entityType, id );

    return preferencesEntry;
}

JsonObject UIPreferencesManager::composeDefaultPreferences( EntityType entityType, const std::string& id ) {
    switch( entityType ) {
    case EntityType::ITEM: {
        const auto items = Fo::getItems( JSON( "_id" << JSON( "$eq" << id ) ) );
        if( !items.empty() ) {
            const std::string itemType = items[ 0 ][ "name" ];

            const EntityPreferencesSchema* preferencesSchema =
                m_uiPreferencesSchema.getEntityPreferencesSchema( entityType, itemType );
            if( preferencesSchema ) {
                return preferencesSchema->composeDefaultPreferences();
            }
        }
    } break;
    }

    return JSON_EMPTY_OBJ;
}
