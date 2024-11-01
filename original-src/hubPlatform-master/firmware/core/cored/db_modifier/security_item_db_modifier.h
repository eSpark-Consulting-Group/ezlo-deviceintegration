#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <set>

#include "jsonrpc/jsonrpc.h"

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

namespace {
class HouseModeEntry;
}  // namespace

namespace CoreD {

class IFeaturesManager;

namespace SwingerShutdown {
class SwingerShutdown;
};  // namespace SwingerShutdown

class SecurityItemDbModifier {
public:
    enum class BypassStatus { SHUTDOWN_PASS, SHUTDOWN_FILTERED, SHUTDOWN_FIRST_TIME, BYPASS, NONE };
    SecurityItemDbModifier( Services::Service& service,
                            const IFeaturesManager& featureManager,
                            SwingerShutdown::SwingerShutdown& swingerShutdown );

    bool isProtectNotification( const Json& updateNotification );

    bool prepareProtectServiceField( Json& broadcast, const std::string& deviceId );
    SecurityItemDbModifier::BypassStatus getBypassModeProtectNotification(
        const CoreDHouseModes::HouseModeEntry& currentHouseMode,
        const std::string& deviceId );

    void setTestMode( bool test );
    bool inTestMode();

private:
    BypassStatus filteredBySwingerShutdown( const std::string& deviceId );
    bool filteredBySwingerShutdownIfEnabledByFeatures();
    BypassStatus getUnreachedStatus( const std::string& deviceId );

private:
    Services::Service& m_service;
    const IFeaturesManager& m_featureManager;
    SwingerShutdown::SwingerShutdown& m_swingerShutdown;
    std::atomic< bool > m_testMode{ false };

private:
    friend class HubPlatform::UT::DbModifierTest;
};

}  // namespace CoreD
}  // namespace HubPlatform
