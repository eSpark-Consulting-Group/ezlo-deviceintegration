#include "device_item_group_manager.h"

#include "resources.h"
#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace CoreD {

namespace {

std::set< std::string > jsonArrayToStringSet( const JsonArray& array ) {
    std::set< std::string > stringSet;
    for( std::string s : array ) {
        stringSet.emplace( std::move( s ) );
    }
    return stringSet;
}

}  // namespace

void DeviceItemGroupManager::setDeviceGroups( const Json& groups ) {
    namespace Fields = Resources::DeviceGroup;
    m_deviceGroups.clear();

    for( auto& groupJson : groups ) {
        DeviceGroup deviceGroup;

        auto devicesIt = groupJson.find( Fields::Devices );
        if( devicesIt != groupJson.cend() ) {
            auto devices = jsonArrayToStringSet( *devicesIt );
            deviceGroup.matchDevices = [devices]( const std::string& deviceId ) {
                return ( devices.find( deviceId ) != devices.cend() );
            };
        }

        auto exceptionsIt = groupJson.find( Fields::Exceptions );
        if( exceptionsIt != groupJson.cend() ) {
            auto exceptions = jsonArrayToStringSet( *exceptionsIt );
            deviceGroup.matchExceptions = [exceptions]( const std::string& deviceId ) {
                return ( exceptions.find( deviceId ) != exceptions.cend() );
            };
        }

        auto categoriesIt = groupJson.find( Fields::Categories );
        if( categoriesIt != groupJson.cend() ) {
            for( auto& categoryJson : *categoriesIt ) {
                MatchString matchCategory;
                MatchString matchSubcategories;
                MatchString matchSubcategoryExceptions;

                std::string groupCategory = json_get_str( categoryJson, Fields::Category );

                std::set< std::string > subcategories;
                auto subcategoriesIt = categoryJson.find( Fields::Subcategories );
                if( subcategoriesIt != categoryJson.cend() ) {
                    subcategories = jsonArrayToStringSet( *subcategoriesIt );
                }

                std::set< std::string > exceptions;
                auto exceptionsIt = categoryJson.find( Fields::Exceptions );
                if( exceptionsIt != categoryJson.cend() ) {
                    exceptions = jsonArrayToStringSet( *exceptionsIt );
                }

                MatchCategorySubcategory match = [groupCategory, subcategories, exceptions](
                                                     const std::string& category, const std::string& subcategory ) {
                    if( !groupCategory.empty() && groupCategory != category )
                        return false;

                    if( exceptions.count( subcategory ) )
                        return false;

                    if( !subcategories.empty() && !subcategories.count( subcategory ) )
                        return false;

                    return true;
                };
                deviceGroup.matchCategories.emplace_back( std::move( match ) );
            }
        }
        m_deviceGroups.emplace( json_get_str( groupJson, "_id" ), std::move( deviceGroup ) );
    }
}

void DeviceItemGroupManager::setItemGroups( const Json& groups ) {
    // TODO use resources when implemented
    m_itemGroups.clear();

    for( auto& groupJson : groups ) {
        ItemGroup itemGroup;

        auto itemsIt = groupJson.find( "items" );
        if( itemsIt != groupJson.cend() ) {
            auto items = jsonArrayToStringSet( *itemsIt );
            itemGroup.matchItems = [items]( const std::string& itemId ) {
                return ( items.find( itemId ) != items.cend() );
            };
        }

        auto itemNamesIt = groupJson.find( "itemNames" );
        if( itemNamesIt != groupJson.cend() ) {
            auto itemNames = jsonArrayToStringSet( *itemNamesIt );
            itemGroup.matchItemNames = [itemNames]( const std::string& itemName ) {
                return ( itemNames.find( itemName ) != itemNames.cend() );
            };
        }

        auto exceptionsIt = groupJson.find( "exceptions" );
        if( exceptionsIt != groupJson.cend() ) {
            auto exceptions = jsonArrayToStringSet( *exceptionsIt );
            itemGroup.matchExceptions = [exceptions]( const std::string& itemId ) {
                return ( exceptions.find( itemId ) != exceptions.cend() );
            };
        }

        auto hasGetterIt = groupJson.find( "hasGetter" );
        if( hasGetterIt != groupJson.cend() ) {
            bool hasGetter = hasGetterIt->get< bool >();
            itemGroup.matchHasGetter = [hasGetter]( bool itemHasGetterValue ) {
                return hasGetter == itemHasGetterValue;
            };
        }

        auto hasSetterIt = groupJson.find( "hasSetter" );
        if( hasSetterIt != groupJson.cend() ) {
            bool hasSetter = hasSetterIt->get< bool >();
            itemGroup.matchHasGetter = [hasSetter]( bool itemHasSetterValue ) {
                return hasSetter == itemHasSetterValue;
            };
        }

        const std::string groupValueType = json_get_str( groupJson, "valueType" );
        if( !groupValueType.empty() ) {
            itemGroup.matchValueType = [groupValueType]( const std::string& valueType ) {
                return groupValueType == valueType;
            };
        }

        auto enumIt = groupJson.find( "enums" );
        if( enumIt != groupJson.cend() ) {
            auto enums = jsonArrayToStringSet( *enumIt );
            itemGroup.matchEnum = [enums]( const std::string& itemId ) {
                return ( enums.find( itemId ) != enums.cend() );
            };
        }

        m_itemGroups.emplace( json_get_str( groupJson, "_id" ), std::move( itemGroup ) );
    }
}

std::set< DeviceItemGroupManager::DeviceGroupId > DeviceItemGroupManager::matchDeviceGroupsForDeviceUpdatedEvent(
    const std::string& deviceId,
    const std::string& deviceCategory,
    const std::string& deviceSubcategory ) {
    std::set< DeviceItemGroupManager::DeviceGroupId > matchedIds;

    for( auto& groupIt : m_deviceGroups ) {
        const auto& groupId = groupIt.first;
        const auto& deviceGroup = groupIt.second;

        if( deviceGroup.matchExceptions && deviceGroup.matchExceptions( deviceId ) )
            continue;

        if( deviceGroup.matchDevices ) {
            if( deviceGroup.matchDevices( deviceId ) ) {
                matchedIds.emplace( groupId );
                continue;
            }
        }

        bool categoryMatch = false;
        if( !deviceGroup.matchCategories.empty() ) {
            for( auto& categorySubcategoryMatch : deviceGroup.matchCategories ) {
                if( categorySubcategoryMatch( deviceCategory, deviceSubcategory ) ) {
                    categoryMatch = true;
                    break;
                }
            }
            if( !categoryMatch )
                continue;
        }

        if( deviceGroup.matchDevices && !categoryMatch )
            continue;

        matchedIds.emplace( groupId );
    }

    return matchedIds;
}

std::set< DeviceItemGroupManager::DeviceGroupId > DeviceItemGroupManager::matchDeviceGroupsForItemUpdatedEvent(
    const Json& eventData ) {
    std::set< DeviceItemGroupManager::DeviceGroupId > matchedIds;

    for( auto& groupIt : m_deviceGroups ) {
        const auto& groupId = groupIt.first;
        const auto& deviceGroup = groupIt.second;

        auto deviceId = json_get_str( eventData, "deviceId" );
        if( deviceGroup.matchExceptions && deviceGroup.matchExceptions( deviceId ) )
            continue;

        if( deviceGroup.matchDevices ) {
            if( deviceGroup.matchDevices( deviceId ) ) {
                matchedIds.emplace( groupId );
                continue;
            }
        }

        bool categoryMatch = false;
        if( !deviceGroup.matchCategories.empty() ) {
            auto deviceCategory = json_get_str( eventData, "deviceCategory" );
            auto deviceSubcategory = json_get_str( eventData, "deviceSubcategory" );
            for( auto& categorySubcategoryMatch : deviceGroup.matchCategories ) {
                if( categorySubcategoryMatch( deviceCategory, deviceSubcategory ) ) {
                    categoryMatch = true;
                    break;
                }
            }
            if( !categoryMatch )
                continue;
        }

        if( deviceGroup.matchDevices && !categoryMatch )
            continue;

        matchedIds.emplace( groupId );
    }

    return matchedIds;
}

std::set< DeviceItemGroupManager::ItemGroupId > DeviceItemGroupManager::matchItemGroupsForItemUpdatedEvent(
    const Json& eventData ) {
    std::set< DeviceItemGroupManager::ItemGroupId > matchedIds;
    for( auto& groupIt : m_itemGroups ) {
        const auto& groupId = groupIt.first;
        const auto& itemGroup = groupIt.second;

        auto itemId = json_get_str( eventData, "_id" );
        if( itemGroup.matchExceptions && itemGroup.matchExceptions( itemId ) )
            continue;

        if( itemGroup.matchItems ) {
            if( itemGroup.matchItems( itemId ) ) {
                matchedIds.emplace( groupId );
                continue;
            }
        }

        size_t matchCount = 0;

        auto valueType = json_get_str( eventData, "valueType" );
        if( itemGroup.matchValueType ) {
            if( !itemGroup.matchValueType( valueType ) )
                continue;
            ++matchCount;
        }

        if( valueType == "token" && itemGroup.matchEnum ) {
            auto value = json_get_str( eventData, "value" );
            if( !itemGroup.matchEnum( value ) )
                continue;
            ++matchCount;
        }

        if( itemGroup.matchItemNames ) {
            auto itemName = json_get_str( eventData, "name" );
            if( !itemGroup.matchItemNames( itemName ) )
                continue;
            ++matchCount;
        }

        if( itemGroup.matchItems && !matchCount )
            continue;

        matchedIds.emplace( groupId );
    }

    return matchedIds;
}
}  // namespace CoreD
}  // namespace HubPlatform
