#pragma once

#include <string>

namespace HubPlatform {
namespace Opkg {

struct KeyFile final {
    static KeyFile fromString( std::string fingerprint, const std::string& string );
    static std::string toString( const KeyFile& feed );

    bool isValid() const;

    const std::string fingerprint;
    const std::string comment;
    const std::string key;
};

}  // namespace Opkg
}  // namespace HubPlatform