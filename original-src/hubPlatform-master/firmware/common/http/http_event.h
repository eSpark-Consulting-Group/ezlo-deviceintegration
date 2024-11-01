#pragma once
#include "http/http_options.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <memory>

namespace HubPlatform {
namespace CommonHttp {

class EventVisitor;

class Event {
public:
    Event() = default;
    Event( std::string id, std::string url, Json userData ) noexcept;
    Event( Event&& ) = default;
    Event& operator=( const Event& ) = default;
    Event& operator=( Event&& ) = default;

    virtual ~Event() = default;
    virtual Json toJson() const;
    virtual void fromJson( const Json& );
    virtual void accept( EventVisitor& visitor ) = 0;

    const std::string& id() const;
    const std::string& url() const;
    const Json& userData() const;

protected:
    std::string m_id;
    std::string m_url;
    Json m_userData;
};

using EventPtr = std::shared_ptr< Event >;

namespace Fields {
extern const char* const id;
extern const char* const url;
extern const char* const userData;
}  // namespace Fields

}  // namespace CommonHttp
}  // namespace HubPlatform
