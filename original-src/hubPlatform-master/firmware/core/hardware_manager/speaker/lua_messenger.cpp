
#include "hardware_manager/speaker/lua_messenger.h"
#include "hardware_manager/hm_service.h"
#include "hardware_manager/speaker/model/resources.h"

#include "hardware_manager/speaker/model/speaker_errors.h"

#include "json_utils/json_utils.h"
#include "tiny/tiny_utils.h"

#include <cassert>

namespace HubPlatform {
namespace HardwareManager {
namespace Speaker {

LuaMessenger::LuaMessenger( IEnvironment& environment )
: m_environment( environment ) {}

bool LuaMessenger::sendEvent( const PlayingInfo& info, PlayingStatus status, ErrorInfo::Code errorCode ) {
    if( info.getHandlerScriptPath().empty() ) {
        LOGGER_TRACE( cc::trace( "Playing script data doesn't have handler script. Event will be skipped: " ) +
                      cc::j( info.toJson() ) );
        return true;
    }

    auto message = JSON( Lua::Fields::id << info.getId() << Fields::status << toString( status ) );

    if( errorCode != ErrorInfo::Code::NoError ) {
        message[ Fields::reason ] = { { Fields::code, static_cast< int >( errorCode ) },
                                      { Fields::text, ErrorInfo::toString( errorCode ) } };
    }

    if( !info.getUserData().empty() ) {
        message[ Fields::userData ] = info.getUserData();
    }

    m_environment.sendExecutePluginScript( info.getHandlerScriptPath(), message );
    return true;
}

}  // namespace Speaker
}  // namespace HardwareManager
}  // namespace HubPlatform
