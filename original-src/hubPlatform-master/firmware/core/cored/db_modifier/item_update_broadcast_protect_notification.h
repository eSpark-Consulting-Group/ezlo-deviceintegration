#pragma once

#include "cored/db_modifier/db_modifier.h"
#include "json_utils/json_utils.h"
#include "security_item_db_modifier.h"

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

class ProtectNotificationCase {
public:
    ProtectNotificationCase( Json& broadcastData,
                             const Json& device,
                             bool protectMode,
                             const SecurityItemDbModifier::BypassStatus& bypassStatus,
                             const IFeaturesManager& featureManager,
                             SecurityItemDbModifier& securityItemDbModifier,
                             IDbClient& dbClient );
    virtual void process(){};
    virtual ~ProtectNotificationCase() = default;

protected:
    Json& m_broadcastData;
    const Json& m_device;
    bool m_protectMode;
    const SecurityItemDbModifier::BypassStatus& m_bypassStatus;
    const IFeaturesManager& m_featureManager;
    SecurityItemDbModifier& m_securityItemDbModifier;
    IDbClient& m_dbClient;
};

class ProtectNotificationDefaultCase : ProtectNotificationCase {
public:
    ProtectNotificationDefaultCase( Json& broadcastData,
                                    const Json& device,
                                    bool protectMode,
                                    const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                    const IFeaturesManager& featureManager,
                                    SecurityItemDbModifier& securityItemDbModifier,
                                    IDbClient& dbClient );
    void process() override;

private:
    void processSecurityConfigFields();
};

class ProtectNotificationBypassCase : ProtectNotificationCase {
public:
    ProtectNotificationBypassCase( Json& broadcastData,
                                   const Json& device,
                                   bool protectMode,
                                   const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                   const IFeaturesManager& featureManager,
                                   SecurityItemDbModifier& securityItemDbModifier,
                                   IDbClient& dbClient );
    void process() override;

private:
    void addBypassSignal();
};

class ProtectNotificationShutdownFirstTimeCase : ProtectNotificationCase {
public:
    ProtectNotificationShutdownFirstTimeCase( Json& broadcastData,
                                              const Json& device,
                                              bool protectMode,
                                              const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                              const IFeaturesManager& featureManager,
                                              SecurityItemDbModifier& securityItemDbModifier,
                                              IDbClient& dbClient );
    void process() override;
};

class ProtectNotificationShutdownFilteredCase : ProtectNotificationCase {
public:
    ProtectNotificationShutdownFilteredCase( Json& broadcastData,
                                             const Json& device,
                                             bool protectMode,
                                             const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                             const IFeaturesManager& featureManager,
                                             SecurityItemDbModifier& securityItemDbModifier,
                                             IDbClient& dbClient );
    void process() override;
};

class ProtectNotificationTestModeCase : ProtectNotificationCase {
public:
    ProtectNotificationTestModeCase( Json& broadcastData,
                                     const Json& device,
                                     bool protectMode,
                                     const SecurityItemDbModifier::BypassStatus& bypassStatus,
                                     const IFeaturesManager& featureManager,
                                     SecurityItemDbModifier& securityItemDbModifier,
                                     IDbClient& dbClient );
    void process() override;
};

}  // namespace CoreD
}  // namespace HubPlatform
