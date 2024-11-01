#pragma once

#include "cored/db_modifier/db_modifier.h"
#include "cored/db_modifier/broadcast_modifier.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "bus/messagesclasses.h"
#include "cored/what_happened.h"
#include "provision_data_utils/provision_data.h"
#include "security_item_db_modifier.h"

#include <string>
#include <functional>
#include <atomic>
#include <cloud_devices/cloud_devices_service.h>

class IDbClient;

namespace HubPlatform {

namespace Services {
class Service;
}

namespace UT {
class DbModifierTest;
class ItemDbModifierTest;
}  // namespace UT

namespace CoreDHouseModes {
class HouseModeEntry;
}  // namespace CoreDHouseModes

namespace CoreD {

class IFeaturesManager;
class BroadcastModifier;
class SecurityItemDbModifier;

using Broadcaster = std::function< void( MessageSubClasses, const Json& ) >;
using ShouldUserBeNotified = std::function< bool( const std::string& itemName,
                                                  const std::string& deviceCategory,
                                                  const std::string& deviceSubcategory,
                                                  const Json& broadcastData,
                                                  const Json& oldFieldsValues ) >;

class ItemUpdateBroadcastBuilder {
public:
    ItemUpdateBroadcastBuilder( Services::Service& service,
                                const ProvisionDataUtils::ProvisionData& provisionData,
                                const IFeaturesManager& featureManager,
                                SecurityItemDbModifier& securityItemDbModifier );

    Json& prepareUpdateBroadcastMessage( Json& broadcastData,
                                         const Json& oldFieldsValues,
                                         ShouldUserBeNotified shouldUserBeNotified );

private:
    Json& renameScalableField( const std::string& replaceField,
                               const std::string& responseField,
                               const std::string& itemValueTypeStr,
                               Json& broadcastData );
    void addBroadcastTimestamp( Json& broadcastData );
    void setUserCodeInfo( Json& broadcastData, const Json& item, const Model::ValueType& valueType ) const;
    void setRoomInfo( Json& broadcastData, const Json& device ) const;
    void setCategoryInfo( const Json& oldFieldsValues,
                          const ShouldUserBeNotified& shouldUserBeNotified,
                          const std::string& ItemId,
                          const std::string& deviceId,
                          const std::string& name,
                          Json& broadcastData,
                          const Json& device ) const;

    void setProtectNotificationInfo( Json& broadcastData,
                                     const Json& device,
                                     bool protectMode,
                                     const SecurityItemDbModifier::BypassStatus& bypassStatus );

    void setGenericInfo( Json& broadcastData,
                         const Json& item,
                         const std::string& deviceId,
                         const CoreDHouseModes::HouseModeEntry::Notifications& notificationUserIds ) const;
    void setValueInfo( Json& broadcastData,
                       const Json& item,
                       const std::string& itemValueTypeStr,
                       const Model::ValueType& valueType );

private:
    Services::Service& m_service;
    IDbClient& m_dbClient;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    std::atomic_bool m_changedStatus{ false };
    const IFeaturesManager& m_featureManager;
    std::vector< BroadcastModifierUPtr > m_modifyBroadcast;
    SecurityItemDbModifier& m_securityItemDbModifier;
};

}  // namespace CoreD
}  // namespace HubPlatform
