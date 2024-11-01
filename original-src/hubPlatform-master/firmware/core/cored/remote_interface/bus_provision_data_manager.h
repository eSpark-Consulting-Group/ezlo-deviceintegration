#pragma once

#include "provision_data_utils/provision_data.h"
#include "service_interfaces/service.h"

#include "json_utils/json_fwd.h"
#include "common_utils/cpp14.h"
#include "tiny/tiny_utils.h"

namespace HubPlatform {
namespace CoreDRemoteInterface {

class BusProvisionDataManager {
public:
    explicit BusProvisionDataManager( Services::Service& hostService ) {
        // NOTE: sometimes only controller key is needed, maybe it is possible in those cases request only it
        auto data = hostService.sendRequest( GroupsNames::CORED, REQUEST_PROVISION_DATA, JSON_EMPTY_OBJ );
        ProvisionDataUtils::ProvisionData provisionData;
        m_provisionData.fillFromJson( data );
        m_provisionData.setControllerKey(
            tiny::tools::base64::decode( json_get_str( data, "controllerKey" ) ).c_str() );
    }

    const ProvisionDataUtils::ProvisionData& getProvisionData() const {
        return m_provisionData;
    }

private:
    ProvisionDataUtils::ProvisionData m_provisionData;
};

inline BusProvisionDataManager& getBusProvisionDataManager( Services::Service& hostService ) {
    static BusProvisionDataManager manager( hostService );
    return manager;
}

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
