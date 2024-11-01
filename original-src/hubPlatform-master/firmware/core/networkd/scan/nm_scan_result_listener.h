#pragma once

#include "json_utils/json_fwd.h"

#include <string>

namespace HubPlatform {
namespace Network {

class WifiScanResultListener {
public:
    virtual ~WifiScanResultListener() = default;

    virtual void onScanStarted( const std::string& interfaceName ) = 0;
    virtual void onScanProcess( const std::string& interfaceName, JsonArray&& networks ) = 0;
    virtual void onScanFinished( const std::string& interfaceName ) = 0;
    virtual void onScanFailed( const std::string& interfaceName, const std::string& errorDescription ) = 0;
};

}  // namespace Network
}  // namespace HubPlatform
