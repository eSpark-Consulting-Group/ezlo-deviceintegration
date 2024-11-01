#include "zwave/common/auth_mode_utils.h"

#include "zwave/common/zwave_exceptions.h"
#include "zwave/resources/zwave_resources.h"

#include "base_utils/comparators.h"

#include <znet/znet.h>

#include <map>

namespace HubPlatform {
namespace Zwave {
namespace AuthModeUtils {
namespace {
using ModeToKey = std::map< const char*, int, Comparators::StringsIgnoreCase >;

const ModeToKey& getModeToKey() {
    static const ModeToKey s_modeToKey{ { Resources::AuthModes::AccessControl, ZNET_SECURITY2_S2_CLASS_2 },
                                        { Resources::AuthModes::Authenticated, ZNET_SECURITY2_S2_CLASS_1 },
                                        { Resources::AuthModes::Unauthenticated, ZNET_SECURITY2_S2_CLASS_0 },
                                        { Resources::AuthModes::S0, ZNET_SECURITY2_S0_NET_KEY },
                                        { Resources::AuthModes::ClientSide, ZNET_SECURITY2_S2_CSA } };

    return s_modeToKey;
}
}  // namespace

Modes keyListToModes( std::uint8_t keyList ) {
    Modes modes;

    for( const auto& pair : getModeToKey() ) {
        if( pair.second & keyList )
            modes.emplace_back( pair.first );
    }

    return modes;
}

std::uint8_t modesToKeyList( const Modes& modes ) {
    std::uint8_t result{ 0 };

    const auto& modeToKey = getModeToKey();
    for( const auto& mode : modes ) {
        auto it = modeToKey.find( mode.c_str() );
        if( it == modeToKey.end() )
            throw AuthModeError{ "Unsupported authentication mode: " + mode };

        result |= it->second;
    }

    return result;
}

void checkAuthMode( const char* mode ) {
    const auto& modeToKey = getModeToKey();
    if( modeToKey.find( mode ) == modeToKey.end() )
        throw AuthModeError{ "Unsupported authentication mode: " + std::string{ mode } };
}

}  // namespace AuthModeUtils
}  // namespace Zwave
}  // namespace HubPlatform
