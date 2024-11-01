#pragma once

#include "service_interfaces/script_event.h"

#include <vector>

namespace HubPlatform {
namespace Zwave {

class LearnShowKey final : public Services::ScriptEvent {
public:
    explicit LearnShowKey( const std::vector< int >& key );

    Json toJson() const override;

private:
    std::vector< int > m_key;
};

}  // namespace Zwave
}  // namespace HubPlatform
