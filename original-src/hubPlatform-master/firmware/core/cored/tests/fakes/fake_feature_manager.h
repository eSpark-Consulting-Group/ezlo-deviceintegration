#pragma once

#include "cored/features_manager.h"

#include <map>

namespace HubPlatform {
namespace CoreD {
namespace UT {

class FakeFeatureManager final : public IFeaturesManager {
public:
    void setFeatureStatus( const std::string& feature, bool status ) override;
    bool getFeatureStatus( const std::string& feature ) const override;
    void setUpExternalFeatures() override;

    using Callback = std::function< void( std::string name, bool status ) >;
    void onFeatureStatusUpdated( Callback callback ) override;

private:
    std::map< std::string, bool > m_featuresStorage;
};

}  // namespace UT
}  // namespace CoreD
}  // namespace HubPlatform
