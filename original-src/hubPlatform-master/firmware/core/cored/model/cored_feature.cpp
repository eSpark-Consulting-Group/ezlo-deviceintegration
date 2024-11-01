#include "cored/model/cored_feature.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

Feature::Feature( Json json )
: m_json( std::move( json ) ) {}

std::string Feature::getName() const {
    return json_get_str( m_json, Resources::FeaturesFields::Id );
}

bool Feature::getStatus() const {
    return json_get_bool( m_json, Resources::FeaturesFields::Status, false );
}

Feature::Version Feature::getVersion() const {
    return Feature::Version( json_get_str( m_json, Resources::FeaturesFields::Version ) );
}

bool Feature::isEditable() const {
    return json_get_bool( m_json, Resources::FeaturesFields::Editable, false );
}

bool Feature::setStatus( bool status ) {
    if( !isEditable() )
        return false;

    json_add_bool( m_json, Resources::FeaturesFields::Status, status );
    return true;
}

bool Feature::isCustom() const {
    return json_get_bool( m_json, Resources::FeaturesFields::IsCustom, false );
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
