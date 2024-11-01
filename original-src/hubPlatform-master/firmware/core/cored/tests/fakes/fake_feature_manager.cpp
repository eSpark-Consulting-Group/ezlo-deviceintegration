#include "cored/tests/fakes/fake_feature_manager.h"

namespace HubPlatform {
namespace CoreD {
namespace UT {

void FakeFeatureManager::setFeatureStatus( const std::string& feature, bool status ) {
    m_featuresStorage[ feature ] = status;
}

bool FakeFeatureManager::getFeatureStatus( const std::string& feature ) const {
    auto it = m_featuresStorage.find( feature );

    if( it != m_featuresStorage.end() && it->second ) {
        return true;
    }

    return false;
}

void FakeFeatureManager::setUpExternalFeatures() {}

void FakeFeatureManager::onFeatureStatusUpdated( FakeFeatureManager::Callback callback ) {}

}  // namespace UT
}  // namespace CoreD
}  // namespace HubPlatform
