#pragma once

#include "cored/model/setting/text_element.h"

#include "json_utils/json_fwd.h"

#include <unordered_map>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class SettingEnum {
public:
    using Token = std::string;

public:
    static SettingEnum fromJson( const JsonObject& json );
    JsonObject toJson() const;

    bool isValid() const;

private:
    std::unordered_map< Token, TextElement > m_settingEnum;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
