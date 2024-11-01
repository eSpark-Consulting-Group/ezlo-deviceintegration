#pragma once

#include "service_interfaces/service.h"
#include "common_utils/addons/addon_version.h"

namespace HubPlatform {
namespace UT {
class AddonInterfacesTestSuite;
}

namespace Addons {

class Addon : public Services::Service {
    friend class UT::AddonInterfacesTestSuite;

public:
    Addon( const std::string& name );
    virtual ~Addon();

    const std::string& getQualifiedName() const override;
    static std::string createQualifiedName( const std::string& name );

    virtual CommonUtils::AddonVersion getVersion() const = 0;

protected:
    Addon( const std::string& name,
           std::unique_ptr< Services::BusWrapper > busWrapper,
           std::unique_ptr< IDbClient > dbClient );

    void setUpFinished() override;
    void tearDown() override;

    void handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) override;

    void sendAddonStartedBroadcast();

private:
    std::string m_qualifiedName;

    int64_t m_setupId;
};
}  // namespace Addons
}  // namespace HubPlatform
