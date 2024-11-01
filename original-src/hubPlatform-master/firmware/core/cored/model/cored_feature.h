#pragma once

#include "base_utils/semantic_version.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace CoreD {
namespace Model {

class Feature final {
public:
    using Version = HubPlatform::Common::SemanticVersion< int >;

    explicit Feature( Json json );

    std::string getName() const;
    bool getStatus() const;
    Version getVersion() const;
    bool isEditable() const;
    bool isCustom() const;

    bool setStatus( bool status );

private:
    Json m_json;
};

using FeaturePtr = std::shared_ptr< HubPlatform::CoreD::Model::Feature >;

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
