#pragma once

#include "provision_data_utils/provision_data.h"
#include "provision_data_utils/update_status.h"

#include "json_utils/json_fwd.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ProvisionDataUtils {

class ProvisionDataManager {
public:
    ProvisionDataManager();

    const ProvisionData& getProvisionData() const;

    bool init( const ProvisionData& data );
    UpdateStatus update( const ProvisionData& data );
    void readData();

private:
    bool checkout();
    bool commit( bool factory, bool factory2 );

    UpdateStatus compareProvisioning( const ProvisionData& oldData, const ProvisionData& newData ) const;

private:
    ProvisionData m_factory;
    ProvisionData m_merged;
};

ProvisionDataManager& getManager();

}  // namespace ProvisionDataUtils
}  // namespace HubPlatform
