#pragma once

#include "provision_data_utils/provision_data.h"
#include "common_utils/cpp14.h"
#include <string>
#include <fstream>

namespace HubPlatform {

namespace UT {
class ProvisioningTestSuite;
}

namespace Provisioning {

class Provisioning {
    friend class UT::ProvisioningTestSuite;

public:
    Provisioning();
    bool hasProvisionInformation();
    bool requestProvisionInformation();

    bool setCloudCertificate();
    bool setHubSslKeyPrivate();
    bool setHubSslKeyPublic();
    bool setRootPassword();
    bool setZwaveRegion();
    bool setOpkgTrustedKey();

private:
    bool writeConfigDataToFile( const std::string& value, const std::string& filePath );
    auto createFileAndCheckValue( const std::string& filePath, const std::string& value )
        -> std::unique_ptr< std::ofstream >;
    auto createFile( const std::string& filePath ) -> std::unique_ptr< std::ofstream >;

private:
    bool replaceRootPassword( const std::string& filePath, const std::string& replaceWith );

private:
    ProvisionDataUtils::ProvisionData m_provisionData;
};

}  // namespace Provisioning
}  // namespace HubPlatform
