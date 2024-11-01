#include "cloud_devices/script_events/command_response.h"
#include "cloud_devices/resources/cloud_devices_resources.h"

namespace HubPlatform {
namespace CloudDevices {

CommandResponse::CommandResponse( std::string commandId, Json result, Json error )
: Services::ScriptEvent( Resources::ScriptEvent::Name::CommandResponse )
, m_commandId( std::move( commandId ) )
, m_result( std::move( result ) )
, m_error( std::move( error ) ) {}

Json CommandResponse::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = Services::ScriptEvent::toJson();
    result[ Fields::CommandId ] = m_commandId;
    result[ Fields::Result ] = m_result;
    result[ Fields::Error ] = m_error;
    return result;
}

}  // namespace CloudDevices
}  // namespace HubPlatform
