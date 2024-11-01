#include "device_group_find_handler.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/model/resources/cored_resources.h"
#include "database_client/idbclient.h"
#include "device_group.h"
#include "resources.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "device_group.h"

namespace HubPlatform {
namespace CoreD {

const auto ID = "_id";

DeviceGroupFindHandler::DeviceGroupFindHandler( IDbClient& client, const std::string& dbName )
: m_dbClient( client )
, m_dbName( std::move( dbName ) ) {}

const IDbClient& DeviceGroupFindHandler::getDBClient() const {
    return m_dbClient;
}

std::string DeviceGroupFindHandler::getStringFieldValue( const JsonObject& params,
                                                         const std::string& fieldName,
                                                         JsonObject& answer ) {
    JsonObject idObject;
    std::string result;

    if( json_get( params, fieldName, idObject ) ) {
        if( !idObject.is_string() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", fieldName );
            return "";
        }

        result = idObject.get< std::string >();

        if( result.empty() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Empty field value", fieldName );
            return "";
        }
    }

    return result;
}

JsonObject DeviceGroupFindHandler::validateFindRequest( const JsonObject& params ) {
    using namespace Resources;
    Json answer;
    JsonObject idObject;

    JsonObject request = JSON_EMPTY_OBJ;

    m_findCategory = getStringFieldValue( params, DeviceGroup::Category, answer );
    if( !answer.empty() ) {
        return answer;
    }

    m_findSubcategory = getStringFieldValue( params, DeviceGroup::Subcategory, answer );
    if( !answer.empty() ) {
        return answer;
    }

    m_findDeviceId = getStringFieldValue( params, DeviceGroup::DeviceId, answer );
    if( !answer.empty() ) {
        return answer;
    }

    m_findGroupId = getStringFieldValue( params, DeviceGroup::DeviceGroupId, answer );
    if( !answer.empty() ) {
        return answer;
    }

    if( json_get( params, DeviceGroup::DeviceGroupIds, m_findDeviceGroupIds ) ) {
        if( !m_findDeviceGroupIds.is_array() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Wrong value type", DeviceGroup::DeviceGroupIds );
            return answer;
        }

        if( m_findDeviceGroupIds.empty() ) {
            setErrorWithField( answer, Errors::WRONG_PARAMS, "Empty field value", DeviceGroup::DeviceGroupIds );
            return answer;
        }
    }

    if( m_findCategory.empty() && m_findSubcategory.empty() && m_findDeviceGroupIds.empty() && m_findGroupId.empty() &&
        m_findDeviceId.empty() ) {
        setError( answer, Errors::BAD_PARAMS );
        return answer;
    }

    return JSON_EMPTY_OBJ;
}

JsonObject DeviceGroupFindHandler::processFindRequest( const JsonObject& params ) {
    using namespace Resources;
    JsonObject idObject;
    JsonArrayBuilder resultArray;
    auto answer = validateFindRequest( params );
    if( !answer.empty() ) {
        return answer;
    }

    answer = getSavedGroups();
    if( !answer.empty() ) {
        return answer;
    }

    for( const auto& group : m_groups ) {
        DeviceGroupInfo info;
        info.fromJson( group );
        if( m_ids.empty() || ( !m_ids.empty() && m_ids.count( info.getId() ) ) ) {
            bool hasCategory = false;
            bool hasSubCategory = false;
            bool hasDevice = false;
            if( !m_findCategory.empty() || !m_findSubcategory.empty() ) {
                auto categories = info.getCategories();

                findCategories( categories, hasCategory, hasSubCategory );
                if( !m_findCategory.empty() && !hasCategory || !m_findSubcategory.empty() && !hasSubCategory ) {
                    continue;
                }
            }
            if( !m_findDeviceId.empty() ) {
                auto devices = info.getDevices();
                auto exceptions = info.getExceptions();
                if( !devices.empty() ) {
                    hasDevice = findDevice( devices, exceptions );
                }
                if( !hasDevice ) {
                    continue;
                }
            }

            if( hasDevice || hasCategory || hasSubCategory ||
                ( m_findDeviceId.empty() && m_findCategory.empty() && m_findSubcategory.empty() ) ) {
                JsonObject resultObject =
                    JSON( DeviceGroup::Id << info.getId() << DeviceGroup::Name << info.getName() );
                resultArray << resultObject;
            }
        }
    }

    answer[ DeviceGroup::DeviceGroups ] = resultArray;
    return answer;
}

JsonObject DeviceGroupFindHandler::getSavedGroups() {
    JsonObject answer;
    JsonObject request = JSON_EMPTY_OBJ;

    if( m_findDeviceGroupIds.empty() && !m_findGroupId.empty() ) {
        request[ ID ] = m_findGroupId;
    }
    else {
        for( const auto& id : m_findDeviceGroupIds ) {
            m_ids.insert( id.get< std::string >() );
        }
    }

    m_groups = getDBClient().findAll( m_dbName, request, nullptr );
    return JSON_EMPTY_OBJ;
}

void DeviceGroupFindHandler::findCategories( JsonArray& categories, bool& hasCategory, bool& hasSubCategory ) const {
    using namespace Resources;
    for( const auto& category : categories ) {
        // Search by category
        if( !m_findCategory.empty() ) {
            auto categoryName = json_get_str( category, DeviceGroup::Category );
            if( m_findCategory == categoryName ) {
                hasCategory = true;
            }
            else {
                continue;
            }
        }

        // Search by subcategory
        if( !m_findSubcategory.empty() ) {
            hasSubCategory = findSubcategory( category );
            if( !hasSubCategory ) {
                continue;
            }
        }

        if( !m_findCategory.empty() && !hasCategory || !m_findSubcategory.empty() && !hasSubCategory ) {
            continue;
        }
    }
}

bool DeviceGroupFindHandler::findSubcategory( const JsonObject& category ) const {
    bool hasSubCategory = false;
    using namespace Resources;
    auto subCategories = category[ DeviceGroup::Subcategories ];

    if( subCategories.empty() ) {
        return true;
    }

    for( auto subCategoryName : subCategories ) {
        if( json_get_str( subCategoryName ) == m_findSubcategory ) {
            hasSubCategory = true;
            break;
        }
    }
    return hasSubCategory;
}

bool DeviceGroupFindHandler::findDevice( JsonArray& devices, JsonArray& exceptions ) const {
    bool hasDevice = false;
    for( const auto& deviceName : devices ) {
        if( deviceName.get< std::string >() == m_findDeviceId ) {
            bool hasException = false;
            if( !exceptions.empty() ) {
                for( const auto& exceptionName : exceptions ) {
                    if( exceptionName.get< std::string >() == m_findDeviceId ) {
                        hasException = true;
                    }
                }
            }
            if( hasException ) {
                continue;
            }

            hasDevice = true;
            break;
        }
    }
    return hasDevice;
}

}  // namespace CoreD
}  // namespace HubPlatform
