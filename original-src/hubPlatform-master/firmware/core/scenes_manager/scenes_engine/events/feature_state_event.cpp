#include "scenes_manager/scenes_engine/events/feature_state_event.h"
#include "json_utils/json_utils.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"
#include "base_utils/semantic_version.h"

namespace SceneEngine {

FeatureStateEvent::FeatureStateEvent( FeatureName name,
                                      bool isSetState,
                                      bool state,
                                      FeatureVersion version,
                                      Comparator comparator )
: m_featureName( std::move( name ) )
, m_isSetStatus( isSetState )
, m_featureStatus( state )
, m_featureVersion( std::move( version ) )
, m_comparator( std::move( comparator ) ) {
    if( m_comparator.empty() ) {
        m_comparator = "==";
    }
}

std::string FeatureStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isFeatureState;
}

void FeatureStateEvent::init( bool status, const std::string& version ) {
    m_featureStatusSaved = status;
    m_featureVersionSaved = version;
    m_init = true;
}

bool FeatureStateEvent::compare( const std::string& name, bool status, const std::string version ) {
    bool checkStatus = true;
    bool checkVersion = true;

    ParameterCompareStatus statusCompare;
    ParameterCompareStatus versionCompare;

    if( m_featureName == name ) {
        // init state
        if( !m_init ) {
            init( status, version );
            return false;
        }

        checkSavedState( status, version, statusCompare, versionCompare );

        if( statusCompare == NotSet && versionCompare == NotSet ) {
            return false;
        }

        if( statusCompare == TheSame && versionCompare == TheSame ) {
            return false;
        }

        checkStatus = statusComparator( status );
        checkVersion = versionComparator( version );

        m_featureStatusSaved = status;
        m_featureVersionSaved = version;
        return checkStatus && checkVersion;
    }

    return false;
}

void FeatureStateEvent::checkSavedState( bool status,
                                         const std::string& version,
                                         ParameterCompareStatus& statusCompare,
                                         ParameterCompareStatus& versionCompare ) const {
    if( !m_isSetStatus ) {
        statusCompare = NotSet;
    }
    else {
        if( status == m_featureStatusSaved ) {
            statusCompare = TheSame;
        }
        else {
            statusCompare = Different;
        }
    }

    if( m_featureVersionSaved.empty() ) {
        versionCompare = NotSet;
    }
    else {
        if( version == m_featureVersionSaved ) {
            versionCompare = TheSame;
        }
        else {
            versionCompare = Different;
        }
    }
}

std::string FeatureStateEvent::getFeatureName() {
    return m_featureName;
}

bool FeatureStateEvent::statusComparator( bool status ) {
    if( m_isSetStatus ) {
        return m_featureStatus == status;
    }
    return true;
}

bool FeatureStateEvent::versionComparator( const std::string& version ) {
    bool checkVersion = true;
    if( !m_featureVersion.empty() && !m_comparator.empty() ) {
        HubPlatform::Common::SemanticVersion< int > currentVersion( version );
        HubPlatform::Common::SemanticVersion< int > triggerVersion( m_featureVersion );

        if( m_comparator == "==" ) {
            checkVersion = currentVersion == triggerVersion;
        }
        else if( m_comparator == ">" ) {
            checkVersion = currentVersion > triggerVersion;
        }
        else if( m_comparator == "<=" ) {
            checkVersion = currentVersion < triggerVersion || currentVersion == triggerVersion;
        }
        else if( m_comparator == ">=" ) {
            checkVersion = currentVersion > triggerVersion || currentVersion == triggerVersion;
        }
        else if( m_comparator == "<" ) {
            checkVersion = currentVersion < triggerVersion;
        }
        else if( m_comparator == "!=" ) {
            checkVersion = !( currentVersion == triggerVersion );
        }
    }

    return checkVersion;
}

}  // namespace SceneEngine
