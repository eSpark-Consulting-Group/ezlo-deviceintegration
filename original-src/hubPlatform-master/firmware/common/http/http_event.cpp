#include "http/http_event.h"
#include "http/http_options.h"
#include "json_utils/json_utils.h"
#include <utility>

namespace HubPlatform {
namespace CommonHttp {

namespace Fields {
const char* const id = "id";
const char* const url = "url";
const char* const userData = "user_data";
}  // namespace Fields

Event::Event( std::string id, std::string url, Json userData ) noexcept
: m_id( std::move( id ) )
, m_url( std::move( url ) )
, m_userData( std::move( userData ) ) {}

Json Event::toJson() const {
    Json result = Json::object();
    json_add_str( result, Fields::id, m_id );
    json_add_str( result, Fields::url, m_url );
    if( !m_userData.empty() )  // optional
        result[ Fields::userData ] = m_userData;

    return result;
}

void Event::fromJson( const Json& from ) {
    m_id = json_get_str( from, Fields::id );
    m_url = json_get_str( from, Fields::url );

    m_userData.clear();
    if( from.count( Fields::userData ) )
        m_userData = from[ Fields::userData ];
}

const std::string& Event::id() const {
    return m_id;
}
const std::string& Event::url() const {
    return m_url;
}
const Json& Event::userData() const {
    return m_userData;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
