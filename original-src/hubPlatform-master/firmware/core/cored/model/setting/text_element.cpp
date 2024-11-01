#include "cored/model/setting/text_element.h"
#include "cored/model/exceptions.h"
#include "cored/model/resources/cored_resources.h"

#include "json_utils/json_custom_getters.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

TextElement TextElement::fromJson( const JsonObject& json ) {
    using namespace JsonCustomGetters;
    using namespace Resources::TextElementFields;

    const auto getter =
        makeStructGetter( makeMemberMapping( &TextElement::m_text, Text ),
                          makeMemberMapping( &TextElement::m_langTag, LangTag, OptionalMemberPolicy() ) );

    TextElement element;
    if( !getter( json, element ) )
        throw CoreDModelError{ "Unexpected Json format for TextElement: " + json.dump() };

    return element;
}

JsonObject TextElement::toJson() const {
    using namespace Resources::TextElementFields;

    auto result = Json::object();

    result[ Text ] = m_text;

    if( !m_langTag.empty() )
        result[ LangTag ] = m_langTag;

    return result;
}

bool TextElement::isValid() const {
    return !m_text.empty();
}

bool TextElement::operator==( const TextElement& other ) const {
    return m_text == other.m_text && m_langTag == other.m_langTag;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
