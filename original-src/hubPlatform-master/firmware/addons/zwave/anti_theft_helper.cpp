#include "zwave/anti_theft_helper.h"
#include "base_utils/md5.h"
#include "common_utils/hub_info_helper.h"

#include <tiny/include/tiny/tiny_utils.h>

#include <algorithm>
#include <random>

namespace HubPlatform {
namespace Zwave {
namespace AntiTheftHelper {

constexpr uint8_t AntiTheftHintVersion = 1;
constexpr uint8_t AntiTheftHintSize = 10;

// TODO: move anti-theft stuff (or part of it to znet)
std::vector< uint8_t > generateHint() {
    constexpr auto data32Size = ( AntiTheftHintSize + sizeof( uint32_t ) - 1 ) / sizeof( uint32_t );
    union {
        uint32_t data32[ data32Size ];
        uint8_t data8[ AntiTheftHintSize ];
    };

    std::generate_n( data32, data32Size, std::mt19937( std::random_device()() ) );
    data8[ AntiTheftHintSize - 1 ] = AntiTheftHintVersion;  // version;

    return { data8, data8 + AntiTheftHintSize };
}

znet::utils::optional< std::vector< uint8_t > > getMagicCode( const std::vector< uint8_t >& hint,
                                                              const std::string& userSpecificString ) {
    // check size
    if( hint.size() != AntiTheftHintSize ) {
        return {};
    }

    // check version
    if( hint.back() != AntiTheftHintVersion ) {
        return {};
    }

    Md5 md5;
    md5.append( hint );
    md5.append( userSpecificString );
    const uint8_t salt[] = { 0x8A, 0x9D, 0x04, 0x8E, 0xC1, 0x52, 0xCB, 0xF9,
                             0x75, 0xD8, 0x07, 0xEA, 0x63, 0x9F, 0x83, 0x3B };
    md5.append( salt, sizeof( salt ) );

    auto result = md5.getBin();

    assert( result.size() == 16 );

    static_assert( AntiTheftHintSize < 16, "check anti-theft size" );

    result.resize( AntiTheftHintSize );

    return { std::move( result ) };
}

znet::utils::optional< std::vector< uint8_t > > getMagicCode( const std::vector< uint8_t >& hint ) {
    const auto userSpecificString =
#if( defined( __HUB_TYPE_LOCAL__ ) )
        "LocalHub";
#else
        tiny::tools::to_lower( HubInfoHelper::getMacAddress() ) + HubInfoHelper::getSerial();
#endif
    return getMagicCode( hint, userSpecificString );
}

}  // namespace AntiTheftHelper
}  // namespace Zwave
}  // namespace HubPlatform
