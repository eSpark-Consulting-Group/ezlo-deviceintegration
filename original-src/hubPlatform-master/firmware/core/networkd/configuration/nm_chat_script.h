#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

namespace ChatOption {
extern const char* const PDPType;
}  // namespace ChatOption

class ChatScript {
public:
    virtual bool saveScript( const Json& config ) = 0;
};

class ChatScript3G : public ChatScript {
public:
    bool saveScript( const Json& config ) override;
};

}  // namespace Network
}  // namespace HubPlatform
