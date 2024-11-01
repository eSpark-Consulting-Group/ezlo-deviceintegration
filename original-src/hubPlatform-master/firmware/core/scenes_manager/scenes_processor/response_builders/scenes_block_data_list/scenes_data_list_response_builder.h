#pragma once

#include <string>
#include <set>
#include <json_utils/json_utils.h>
#include "cored/remote_interface/cored_interface.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace ScenesManager {

class ScenesDataListResponseBuilder {
public:
    ScenesDataListResponseBuilder( CoreDRemoteInterface::CoreDInterface& );
    ~ScenesDataListResponseBuilder() = default;

    JsonObject getDataWithFilter();
    void setAll( bool all );
    void setValueTypes( bool valueTypes );

    void setScalableValueTypes( bool scalableValueTypes );
    void setValueScales( bool valueScales );
    void setSceneValueTypes( bool sceneValueTypes );
    void setValueTypeFamilies( bool valueTypeFamilies );
    void setComparisonOperators( bool comparisonOperators );
    void setComparisonMethods( bool comparisonMethods );
    void setActionMethods( bool actionsData );

    void getValueTypeData( JsonArrayBuilder& valueTypes, JsonArrayBuilder& scalableValueTypes, Json& valueScales );

    JsonObject getTypeFamilyTypes();
    bool getDefaultScale( const std::string& typeName );

private:
    bool m_valueTypes{ false };
    bool m_scalableValueTypes{ false };
    bool m_valueScales{ false };
    bool m_sceneValueTypes{ false };
    bool m_valueTypeFamilies{ false };
    bool m_comparisonOperators{ false };
    bool m_comparisonMethods{ false };
    bool m_actionMethods{ false };
    JsonObject getComparisonMethods();
    Json getComparisonOperators() const;

private:
    CoreDRemoteInterface::CoreDInterface& m_coreInterface;
};

}  // namespace ScenesManager
}  // namespace HubPlatform
