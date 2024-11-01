#pragma once

#include "cored/db_modifier/db_modifier.h"
#include "cored/db_modifier/broadcast_modifier.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "bus/messagesclasses.h"
#include "cored/what_happened.h"
#include "provision_data_utils/provision_data.h"
#include "cored/db_modifier/item_update_broadcast_builder.h"

#include <string>
#include <functional>
#include <atomic>

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

class ItemDbModifier final : public DbModifier {
public:
    ItemDbModifier( Services::Service& service,
                    const ProvisionDataUtils::ProvisionData& provisionData,
                    Broadcaster broadcaster,
                    const IFeaturesManager& featureManager,
                    SecurityItemDbModifier& securityItemDbModifier );
    ~ItemDbModifier() override = default;

    Json& prepareUpdateBroadcastMessage( Json& broadcastData, const Json& oldFieldsValues );
    Json& prepareInsertBroadcastMessage( Json& broadcastData ) override;
    Json& prepareRemoveBroadcastMessage( Json& broadcastData, const Json& removedRecord ) override;

    Json prepareUserNotificationBroadcastMessage( const Json& data );

    bool hasChanges() const override;
    void setChangedStatus( bool isChanged ) override;

    void sendUpdateBroadcast( MessageSubClasses subClass, Json& broadcastData, const Json& oldFieldsValues ) override;

    void updateDictionary( const Id& recordId, const Json& updatePortion, const Dictionary& dictionary );

private:
    Json& renameScalableField( const std::string& replaceField,
                               const std::string& responseField,
                               const std::string& itemValueTypeStr,
                               Json& broadcastData );

private:
    bool shouldUserBeNotified( const std::string& itemName,
                               const std::string& deviceCategory,
                               const std::string& deviceSubcategory,
                               const Json& broadcastData,
                               const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForBatteryItem( const std::string& deviceCategory,
                                                    const std::string& deviceSubcategory,
                                                    const Json& broadcastData,
                                                    const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForSecurityThreatItem( const std::string& deviceCategory,
                                                           const std::string& deviceSubcategory,
                                                           const Json& broadcastData,
                                                           const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForSmokeItems( const std::string& deviceCategory,
                                                   const std::string& deviceSubcategory,
                                                   const Json& broadcastData,
                                                   const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForLockOperationItems( const std::string& deviceCategory,
                                                           const std::string& deviceSubcategory,
                                                           const Json& broadcastData,
                                                           const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForTamperingCoverAlarmItems( const std::string& deviceCategory,
                                                                 const std::string& deviceSubcategory,
                                                                 const Json& broadcastData,
                                                                 const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForDwStateItems( const std::string& deviceCategory,
                                                     const std::string& deviceSubcategory,
                                                     const Json& broadcastData,
                                                     const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForWaterLeakAlarmItems( const std::string& deviceCategory,
                                                            const std::string& deviceSubcategory,
                                                            const Json& broadcastData,
                                                            const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedForMotionItems( const std::string& deviceCategory,
                                                    const std::string& deviceSubcategory,
                                                    const Json& broadcastData,
                                                    const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedCoAlarmItems( const std::string& deviceCategory,
                                                  const std::string& deviceSubcategory,
                                                  const Json& broadcastData,
                                                  const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedGlassBreakageAlarmItems( const std::string& deviceCategory,
                                                             const std::string& deviceSubcategory,
                                                             const Json& broadcastData,
                                                             const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedUserCodeOperationItems( const std::string& deviceCategory,
                                                            const std::string& deviceSubcategory,
                                                            const Json& broadcastData,
                                                            const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedThermostatModeItems( const std::string& deviceCategory,
                                                         const std::string& deviceSubcategory,
                                                         const Json& broadcastData,
                                                         const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedThermostatFanModeItems( const std::string& deviceCategory,
                                                            const std::string& deviceSubcategory,
                                                            const Json& broadcastData,
                                                            const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedSwitchItems( const std::string& deviceCategory,
                                                 const std::string& deviceSubcategory,
                                                 const Json& broadcastData,
                                                 const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedBarrierStateItems( const std::string& deviceCategory,
                                                       const std::string& deviceSubcategory,
                                                       const Json& broadcastData,
                                                       const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedWaterTemperatureAlarmItems( const std::string& deviceCategory,
                                                                const std::string& deviceSubcategory,
                                                                const Json& broadcastData,
                                                                const Json& oldFieldsValues );

    static bool shouldUserBeNotifiedHeatAlarmItems( const std::string& deviceCategory,
                                                    const std::string& deviceSubcategory,
                                                    const Json& broadcastData,
                                                    const Json& oldFieldsValues );
    static bool shouldUserBeNotifiedDoorBellRing( const std::string& deviceCategory,
                                                  const std::string& deviceSubcategory,
                                                  const Json& broadcastData,
                                                  const Json& oldFieldsValues );

    static bool notifyUserAboutHardwareState( const std::string& deviceCategory,
                                              const std::string& deviceSubcategory,
                                              const Json& broadcastData,
                                              const Json& oldFieldsValues );
    static bool notifyUserAboutSoftwareState( const std::string& deviceCategory,
                                              const std::string& deviceSubcategory,
                                              const Json& broadcastData,
                                              const Json& oldFieldsValues );
    static bool notifyUserAboutHealthPositionChanged( const std::string& deviceCategory,
                                                      const std::string& deviceSubcategory,
                                                      const Json& broadcastData,
                                                      const Json& oldFieldsValues );

    static bool notifyUserAboutSirenState( const std::string& deviceCategory,
                                           const std::string& deviceSubcategory,
                                           const Json& broadcastData,
                                           const Json& oldFieldsValues );

    static bool notifyUserAboutProtectAlarm( const std::string& deviceCategory,
                                             const std::string& deviceSubcategory,
                                             const Json& broadcastData,
                                             const Json& oldFieldsValues );

    static Json prepareUserLockOperationBroadcast( const IDbClient& db, const Json& data );
    static Json prepareUserActionBroadcast( const IDbClient& db, const Json& data );
    static Json generatePinCodeEnteredBroadcast( const IDbClient& db, const Json& data, int userId );
    Json& prepareHouseModeData( const Json& broadcastData, Json& broadcast );
    Json addProtectConfig( const Json& broadcastData ) const;
    bool isUsingProtectConfig() override final;

private:
    Services::Service& m_service;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    std::atomic_bool m_changedStatus{ false };
    Dictionary m_dictionary;
    const IFeaturesManager& m_featureManager;
    std::vector< BroadcastModifierUPtr > m_modifyBroadcast;
    SecurityItemDbModifier& m_securityItemDbModifier;
    std::unique_ptr< ItemUpdateBroadcastBuilder > m_itemUpdateBroadcastBuilder;

private:
    friend class HubPlatform::UT::DbModifierTest;
    friend class HubPlatform::UT::ItemDbModifierTest;
};

}  // namespace CoreD
}  // namespace HubPlatform
