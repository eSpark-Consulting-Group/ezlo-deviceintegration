#pragma once

#include <functional>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Vidoo4Hub {

class IVidooClient {
public:
    using MessageCallback = std::function< void( const JsonObject& ) >;
    virtual void setUp() = 0;
    virtual void tearDown() = 0;
    virtual void sendMessage( const JsonObject& msg ) = 0;
    virtual ~IVidooClient() = default;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
