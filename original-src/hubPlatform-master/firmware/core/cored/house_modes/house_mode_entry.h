#pragma once

#include "json_utils/json_fwd.h"
#include <string>
#include <set>

namespace HubPlatform {
namespace CoreDHouseModes {

struct HouseModeEntry {
    using Subcategories = std::set< std::string >;              // subcategories
    using Categories = std::map< std::string, Subcategories >;  // filter of categories
    using Notifications = std::shared_ptr< std::set< std::string > >;
    using DeviceIds = std::set< std::string >;

    HouseModeEntry( std::string id,
                    std::string name,
                    std::string description,
                    bool disarmedDefault,
                    bool ignoreDelay,
                    bool protect,
                    bool armed,
                    Notifications notificationUserIds,
                    Notifications notificationUserIdsDefault,
                    Categories armedCategories,
                    Categories alarmCategories );
    explicit HouseModeEntry( const JsonObject& obj );

    // could be the reflection under C++ 20
    JsonObject toJson() const;
    JsonObject toJsonDb() const;

    static void notificationsToJson( JsonObject& obj, const Notifications& notification );
    static JsonObject notificationsToJson( const Notifications& notifications );

    std::string m_id;
    std::string m_name;
    std::string m_description;
    bool m_disarmedDefault = true;
    bool m_ignoreDelay = false;
    bool m_protect = true;
    bool m_armed = false;
    DeviceIds m_disarmedDeviceIds;
    Notifications m_notificationUserIds;
    Notifications m_notificationUserIdsDefault;
    Categories m_armedCategories;
    DeviceIds m_alarmsOffDeviceIds;
    Categories m_alarmCategories;
    DeviceIds m_camerasOffDeviceIds;  //!< device identifiers of camera devices that should be off in a house mode
    DeviceIds m_bypassDeviceIds;
};

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
