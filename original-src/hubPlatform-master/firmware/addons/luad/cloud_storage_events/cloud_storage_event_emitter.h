#pragma once

#include <lua.h>
#include "service_interfaces/service.h"

namespace HubPlatform {
namespace LuaD {

class CloudStorageEventsEmitter {
public:
    explicit CloudStorageEventsEmitter( Services::Service& service );
    void emitTokenUpdatedEvent();
    void emitTokenExpiredEvent();

private:
    Services::Service& m_service;
};

}  // namespace LuaD
}  // namespace HubPlatform
