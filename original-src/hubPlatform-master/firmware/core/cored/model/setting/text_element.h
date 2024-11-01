#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {

class TextElement {
public:
    static TextElement fromJson( const JsonObject& json );
    JsonObject toJson() const;

    bool isValid() const;

    bool operator==( const TextElement& other ) const;

private:
    std::string m_text;
    std::string m_langTag;
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
