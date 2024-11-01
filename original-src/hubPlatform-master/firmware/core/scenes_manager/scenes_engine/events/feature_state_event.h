#pragma once

#include "scenes_manager/scenes_engine/events/event_base.h"

namespace SceneEngine {

enum ParameterCompareStatus { NotSet, TheSame, Different };

class FeatureStateEvent : public EventBase {
public:
    using FeatureName = std::string;
    using FeatureVersion = std::string;
    using Comparator = std::string;

    explicit FeatureStateEvent( FeatureName name,
                                bool isSetState,
                                bool state,
                                FeatureVersion version,
                                Comparator comparator );
    std::string getMethodName() const override;
    std::string getFeatureName();
    bool compare( const std::string& name, bool state, const std::string version );

private:
    bool versionComparator( const std::string& version );
    bool statusComparator( bool state );

private:
    FeatureName m_featureName;
    bool m_featureStatus;
    FeatureVersion m_featureVersion;
    Comparator m_comparator;

    bool m_featureStatusSaved;
    FeatureVersion m_featureVersionSaved;
    bool m_isSetStatus = false;
    bool m_init = false;

    void init( bool status, const std::string& version );

    void checkSavedState( bool status,
                          const std::string& version,
                          SceneEngine::ParameterCompareStatus& statusCompare,
                          SceneEngine::ParameterCompareStatus& versionCompare ) const;
};

using FeatureStateEventPtr = std::shared_ptr< FeatureStateEvent >;

}  // namespace SceneEngine
