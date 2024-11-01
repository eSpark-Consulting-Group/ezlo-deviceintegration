#include "cored/model/setting/setting_enum.h"
#include "cored/model/exceptions.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

SettingEnum SettingEnum::fromJson( const JsonObject& json ) {
    if( !json.is_object() )
        throw CoreDModelError{ "Unexpected Json format for SettingEnum: " + json.dump() };

    SettingEnum settingEnum;

    for( auto it = json.begin(); it != json.end(); ++it ) {
        settingEnum.m_settingEnum.emplace( it.key(), TextElement::fromJson( it.value() ) );
    }

    return settingEnum;
}

JsonObject SettingEnum::toJson() const {
    auto result = Json::object();

    for( const auto& pair : m_settingEnum )
        result[ pair.first ] = pair.second.toJson();

    return result;
}

bool SettingEnum::isValid() const {
    return !m_settingEnum.empty();
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
