#pragma once

#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace CoreD {
namespace SwingerShutdown {

class SwingerShutdownInterface {
public:
    explicit SwingerShutdownInterface( Services::Service& hostService );
    JsonObject getSwingerShutdownList( const JsonObject& params );
    JsonObject resetDeviceStatistics( const JsonObject& params );
    JsonObject disableAdd( const JsonObject& params );
    JsonObject disableRemove( const JsonObject& params );
    JsonObject limitsSet( const JsonObject& params );
    JsonObject limitsReset( const JsonObject& params );

private:
    Services::Service& m_hostService;
};

}  // namespace SwingerShutdown
}  // namespace CoreD
}  // namespace HubPlatform
