#include "cloud_storage_event_emitter.h"
#include "token_updated.h"
#include "token_expired.h"

namespace HubPlatform {
namespace LuaD {

CloudStorageEventsEmitter::CloudStorageEventsEmitter( Services::Service& service )
: m_service( service ) {}

void CloudStorageEventsEmitter::emitTokenUpdatedEvent() {
    m_service.emit( ScriptEvents::TokenUpdated() );
}

void CloudStorageEventsEmitter::emitTokenExpiredEvent() {
    m_service.emit( ScriptEvents::TokenExpired() );
}

}  // namespace LuaD
}  // namespace HubPlatform