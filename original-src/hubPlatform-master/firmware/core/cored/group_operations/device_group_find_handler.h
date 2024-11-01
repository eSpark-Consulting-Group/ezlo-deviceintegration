#pragma once
#include "scenes_manager/scenes_processor/scene_request_handler_interface.h"
#include <set>
class IDbClient;
namespace HubPlatform {

namespace UT {
class ScenesAPIFixture;
class GroupOperationsFixture;
}  // namespace UT

namespace CoreD {

class DeviceGroupFindHandler {
public:
    DeviceGroupFindHandler( IDbClient& client, const std::string& dbName );
    ~DeviceGroupFindHandler() = default;

    JsonObject processFindRequest( const JsonObject& params );

private:
    const IDbClient& getDBClient() const;
    static std::string getStringFieldValue( const JsonObject& params,
                                            const std::string& fieldName,
                                            JsonObject& answer );
    JsonObject validateFindRequest( const JsonObject& params );
    bool findDevice( JsonArray& devices, JsonArray& exceptions ) const;
    void findCategories( JsonArray& categories, bool& hasCategory, bool& hasSubCategory ) const;
    JsonObject getSavedGroups();
    bool findSubcategory( const JsonObject& category ) const;

protected:
    IDbClient& m_dbClient;
    friend class UT::GroupOperationsFixture;

    std::string m_findCategory;
    std::string m_findSubcategory;
    std::string m_findDeviceId;
    std::string m_findGroupId;
    JsonArray m_findDeviceGroupIds;
    std::vector< Json > m_groups;
    std::set< std::string > m_ids;
    std::string m_dbName;
};

}  // namespace CoreD
}  // namespace HubPlatform
