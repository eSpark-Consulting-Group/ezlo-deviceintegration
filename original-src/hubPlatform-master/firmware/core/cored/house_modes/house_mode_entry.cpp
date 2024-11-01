#include "cored/house_modes/house_mode_entry.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"
#include <utility>

namespace HubPlatform {
namespace CoreDHouseModes {

using namespace Resources;

HouseModeEntry::HouseModeEntry( std::string id,
                                std::string name,
                                std::string description,
                                bool disarmedDefault,
                                bool ignoreDelay,
                                bool protect,
                                bool armed,
                                Notifications notificationUserIds,
                                Notifications notificationUserIdsDefault,
                                Categories armedCategories,
                                Categories alarmCategories )
: m_id( std::move( id ) )
, m_name( std::move( name ) )
, m_description( std::move( description ) )
, m_disarmedDefault( disarmedDefault )
, m_ignoreDelay( ignoreDelay )
, m_protect( protect )
, m_armed( armed )
, m_armedCategories( std::move( armedCategories ) )
, m_alarmCategories( std::move( alarmCategories ) )
, m_notificationUserIds( std::move( notificationUserIds ) )
, m_notificationUserIdsDefault( std::move( notificationUserIdsDefault ) ) {}

HouseModeEntry::HouseModeEntry( const JsonObject& obj ) {
    try {
        json_get( obj, HouseModeFields::IdFieldName, m_id );
        json_get( obj, HouseModeFields::ModeNameFieldName, m_name );
        json_get( obj, HouseModeFields::DescriptionFieldName, m_description );
        json_get( obj, HouseModeFields::DisarmedDefaultFieldName, m_disarmedDefault );

        if( !json_get( obj, HouseModeFields::IgnoreSwitchDelay, m_ignoreDelay ) ) {
            m_ignoreDelay = false;
        }

        m_protect = json_get_bool( obj, HouseModeFields::ProtectFieldName, true );
        m_armed = json_get_bool( obj, HouseModeFields::ArmedFieldName, false );

        JsonArray disarmedDevicesArr;
        if( json_get_arr( obj, HouseModeFields::DisarmedDevicesFieldName, disarmedDevicesArr ) )
            for( const auto& deviceId : disarmedDevicesArr )
                m_disarmedDeviceIds.insert( deviceId.get< std::string >() );

        JsonArray notificationsArr;
        if( json_get_arr( obj, HouseModeFields::NotificationsFieldName, notificationsArr ) ) {
            m_notificationUserIds = std::make_unique< std::set< std::string > >();
            for( const auto& userId : notificationsArr )
                m_notificationUserIds->insert( userId.get< std::string >() );
        }

        JsonArray notificationsDefaultArr;
        if( json_get_arr( obj, HouseModeFields::NotificationsDefaultFieldName, notificationsDefaultArr ) ) {
            m_notificationUserIdsDefault = std::make_unique< std::set< std::string > >();
            for( const auto& userId : notificationsDefaultArr )
                m_notificationUserIdsDefault->insert( userId.get< std::string >() );
        }

        JsonArray alarmsOffDevicesArr;
        if( json_get_arr( obj, HouseModeFields::AlarmsOffDevicesFieldName, alarmsOffDevicesArr ) )
            for( const auto& deviceId : alarmsOffDevicesArr )
                m_alarmsOffDeviceIds.insert( deviceId.get< std::string >() );

        JsonArray camerasOffDevicesArr;
        if( json_get_arr( obj, HouseModeFields::CamerasOffDevicesFieldName, camerasOffDevicesArr ) )
            for( const auto& deviceId : camerasOffDevicesArr )
                m_camerasOffDeviceIds.insert( deviceId.get< std::string >() );

        JsonArray bypassDevicesArr;
        if( json_get_arr( obj, HouseModeFields::BypassDevicesFieldName, bypassDevicesArr ) )
            for( const auto& deviceId : bypassDevicesArr )
                m_bypassDeviceIds.insert( deviceId.get< std::string >() );

        JsonArray armedCategoriesArr;
        if( json_get_arr( obj, HouseModeFields::ArmedCategoriesFieldName, armedCategoriesArr ) ) {
            for( const auto& categoryJson : armedCategoriesArr ) {
                const std::string category = categoryJson[ HouseModeFields::CategoryFieldName ];

                Subcategories& subcategories = m_armedCategories[ category ];  // at least it will be empty
                JsonArray subcategoriesJson;
                if( json_get_arr( categoryJson, HouseModeFields::SubcategoriesFieldName, subcategoriesJson ) ) {
                    for( const auto& subcategoryJson : subcategoriesJson ) {
                        const std::string subcategory = subcategoryJson.get< std::string >();
                        subcategories.insert( subcategory );
                    }
                }
            }
        }

        JsonArray alarmCategoriesArr;
        if( json_get_arr( obj, HouseModeFields::AlarmCategoriesFieldName, alarmCategoriesArr ) ) {
            for( const auto& categoryJson : alarmCategoriesArr ) {
                const std::string category = categoryJson[ HouseModeFields::CategoryFieldName ];

                Subcategories& subcategories = m_alarmCategories[ category ];  // at least it will be empty
                JsonArray subcategoriesJson;
                if( json_get_arr( categoryJson, HouseModeFields::SubcategoriesFieldName, subcategoriesJson ) ) {
                    for( const auto& subcategoryJson : subcategoriesJson ) {
                        const std::string subcategory = subcategoryJson.get< std::string >();
                        subcategories.insert( subcategory );
                    }
                }
            }
        }
    }
    catch( const std::exception& ex ) {
        Log::error( "HouseModeEntry::HouseModeEntry, json is not valid, error :" + std::string( ex.what() ) );
    }
}

// could be the reflection under C++ 20
JsonObject HouseModeEntry::toJson() const {
    JsonObject result{ { HouseModeFields::IdFieldName, m_id },
                       { HouseModeFields::ModeNameFieldName, m_name },
                       { HouseModeFields::DescriptionFieldName, m_description },
                       { HouseModeFields::DisarmedDefaultFieldName, m_disarmedDefault },
                       { HouseModeFields::ProtectFieldName, m_protect },
                       { HouseModeFields::ArmedFieldName, m_armed },
                       { HouseModeFields::NotificationsFieldName, JSON_EMPTY_ARR },
                       { HouseModeFields::DisarmedDevicesFieldName, JSON_EMPTY_ARR },
                       { HouseModeFields::AlarmsOffDevicesFieldName, JSON_EMPTY_ARR },
                       { HouseModeFields::CamerasOffDevicesFieldName, JSON_EMPTY_ARR },
                       { HouseModeFields::BypassDevicesFieldName, JSON_EMPTY_ARR } };
    if( m_ignoreDelay ) {
        result[ HouseModeFields::IgnoreSwitchDelay ] = true;
    }
    JsonArray& notificationsArr = result[ HouseModeFields::NotificationsFieldName ];
    if( m_disarmedDefault ) {
        notificationsToJson( notificationsArr, m_notificationUserIdsDefault );
    }
    else {
        notificationsToJson( notificationsArr, m_notificationUserIds );
    }
    result[ HouseModeFields::DisarmedDevicesFieldName ] = m_disarmedDeviceIds;
    result[ HouseModeFields::AlarmsOffDevicesFieldName ] = m_alarmsOffDeviceIds;
    result[ HouseModeFields::CamerasOffDevicesFieldName ] = m_camerasOffDeviceIds;
    result[ HouseModeFields::BypassDevicesFieldName ] = m_bypassDeviceIds;
    return result;
}

JsonObject HouseModeEntry::toJsonDb() const {
    JsonObject result = toJson();
    result[ HouseModeFields::ArmedCategoriesFieldName ] = JSON_EMPTY_ARR;

    JsonArray& armedCategoriesArr = result[ HouseModeFields::ArmedCategoriesFieldName ];
    for( auto& categoryPair : m_armedCategories ) {
        JsonObject categoryJson = JSON_EMPTY_OBJ;
        categoryJson[ HouseModeFields::CategoryFieldName ] = categoryPair.first;

        if( !categoryPair.second.empty() ) {
            categoryJson[ HouseModeFields::SubcategoriesFieldName ] = JSON_EMPTY_ARR;
            JsonArray& subcategoryArr = categoryJson[ HouseModeFields::SubcategoriesFieldName ];
            std::copy( categoryPair.second.begin(), categoryPair.second.end(), std::back_inserter( subcategoryArr ) );
        }

        armedCategoriesArr.push_back( std::move( categoryJson ) );
    }

    JsonArray& alarmCategoriesArr = result[ HouseModeFields::AlarmCategoriesFieldName ];
    for( auto& categoryPair : m_alarmCategories ) {
        JsonObject categoryJson = JSON_EMPTY_OBJ;
        categoryJson[ HouseModeFields::CategoryFieldName ] = categoryPair.first;

        if( !categoryPair.second.empty() ) {
            categoryJson[ HouseModeFields::SubcategoriesFieldName ] = JSON_EMPTY_ARR;
            JsonArray& subcategoryArr = categoryJson[ HouseModeFields::SubcategoriesFieldName ];
            std::copy( categoryPair.second.begin(), categoryPair.second.end(), std::back_inserter( subcategoryArr ) );
        }

        alarmCategoriesArr.push_back( std::move( categoryJson ) );
    }

    JsonArray& notificationsDefault = result[ HouseModeFields::NotificationsDefaultFieldName ];
    HouseModeEntry::notificationsToJson( notificationsDefault, m_notificationUserIdsDefault );

    return result;
}

void HouseModeEntry::notificationsToJson( JsonObject& obj, const HouseModeEntry::Notifications& notifications ) {
    if( notifications ) {
        obj = *notifications;
    }
    else {
        obj = nullptr;
    }
}

JsonObject HouseModeEntry::notificationsToJson( const HouseModeEntry::Notifications& notifications ) {
    JsonObject obj = JSON_EMPTY_OBJ;
    notificationsToJson( obj, notifications );
    return std::move( obj );
}

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
