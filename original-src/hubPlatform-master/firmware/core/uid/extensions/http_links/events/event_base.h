#pragma once
#include "json_utils/json_utils.h"
#include <string>

namespace HubPlatform {
namespace HttpLinks {

class EventBase {
public:
    EventBase() = default;
    EventBase( std::string id, std::string linkId, Json userData );

    virtual ~EventBase() = default;
    virtual Json toJson() const;
    virtual Json toJsonFull() const;
    virtual const char* name() const = 0;

    const std::string& id() const;
    void id( const std::string& value );
    const std::string& linkId() const;
    void linkId( const std::string& value );
    const Json& userData() const;
    void userData( Json value );

private:
    std::string m_id;
    std::string m_linkId;
    Json m_userData;
};

}  // namespace HttpLinks
}  // namespace HubPlatform