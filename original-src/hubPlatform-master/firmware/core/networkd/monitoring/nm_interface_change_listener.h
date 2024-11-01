#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace Network {

class InterfaceInfoListener {
public:
    virtual ~InterfaceInfoListener() = default;

    virtual void onNewInterfaceInfo( Json info ) = 0;
    virtual void onNewInterfaceAdded( const std::string& interfaceName ) {}
    virtual void onInterfaceRemoved( const std::string& interfaceName ) {}
};

}  // namespace Network
}  // namespace HubPlatform
