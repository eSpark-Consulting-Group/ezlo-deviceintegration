#include "zwave/script_events/user_code_updated.h"

#include "zwave/resources/zwave_resources.h"

#include <znet/converters/user_code.hpp>

namespace HubPlatform {
namespace Zwave {

UserCodeUpdated::UserCodeUpdated( uint8_t nodeId,
                                  uint8_t channelId,
                                  znet::UserCode::UserCodeRanges&& usersCodeRanges,
                                  uint16_t usersCount,
                                  znet::UserCode::UserPollStatus pollingStatus )
: ValueEvent( Resources::ScriptEvent::Name::UserCodeUpdated, nodeId, ZNET_COMMAND_CLASS_USER_CODE, channelId )
, m_userCodesRanges( std::move( usersCodeRanges ) )
, m_usersCount( usersCount )
, m_userPollingStatus( pollingStatus ) {}

Json UserCodeUpdated::toJson() const {
    using namespace Resources::ScriptEvent;

    auto result = ValueEvent::toJson();

    auto& joRanges = result[ Param::UserCodeRanges ] = Json::array();

    for( const auto& range : m_userCodesRanges ) {
        auto joRange = Json::object();

        joRange[ Param::UserId ] = range.id;

        if( range.id != range.lastId ) {
            joRange[ Param::LastUserId ] = range.lastId;
        }

        if( const char* status = znet::user_code_converter::userIdStatusToString( range.status ) ) {
            joRange[ Param::UserStatus ] = status;
        }

        joRange[ Param::UserCode ] = range.code;

        joRanges.push_back( std::move( joRange ) );
    }

    result[ Param::SupportedUsersCount ] = m_usersCount;

    if( const char* status = znet::user_code_converter::userPollStatusToString( m_userPollingStatus ) ) {
        result[ Param::UserPollingStatus ] = status;
    }

    return result;
}

}  // namespace Zwave
}  // namespace HubPlatform
