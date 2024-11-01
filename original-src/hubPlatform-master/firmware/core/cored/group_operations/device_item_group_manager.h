#pragma once

#include "json_utils/json_wrapper.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <functional>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class DeviceItemGroupManager {
public:
    using DeviceGroupId = std::string;
    using ItemGroupId = std::string;

    void setDeviceGroups( const Json& groups );
    void setItemGroups( const Json& groups );
    std::set< DeviceGroupId > matchDeviceGroupsForItemUpdatedEvent( const Json& message );
    std::set< ItemGroupId > matchItemGroupsForItemUpdatedEvent( const Json& message );
    std::set< DeviceItemGroupManager::DeviceGroupId > matchDeviceGroupsForDeviceUpdatedEvent(
        const std::string& deviceId,
        const std::string& deviceCategory,
        const std::string& deviceSubcategory );

private:
    using MatchString = std::function< bool( const std::string& ) >;
    using MatchBool = std::function< bool( bool ) >;
    using MatchCategorySubcategory = std::function< bool( const std::string&, const std::string& ) >;

    struct DeviceGroup {
        MatchString matchDevices;
        MatchString matchExceptions;
        std::vector< MatchCategorySubcategory > matchCategories;
    };

    struct ItemGroup {
        MatchString matchItemNames;
        MatchString matchItems;
        MatchString matchExceptions;
        MatchBool matchHasGetter;
        MatchBool matchHasSetter;
        MatchString matchValueType;
        MatchString matchEnum;
    };

private:
    std::map< DeviceGroupId, DeviceGroup > m_deviceGroups;
    std::map< ItemGroupId, ItemGroup > m_itemGroups;
};
}  // namespace CoreD
}  // namespace HubPlatform
