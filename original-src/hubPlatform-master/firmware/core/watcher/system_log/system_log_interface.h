#pragma once

#include <string>
#include <functional>

namespace HubPlatform {
namespace Watcher {

class SystemLogInterface {
public:
    using OnLogRecord = std::function< void( const std::string& ) >;
    virtual ~SystemLogInterface() = default;

    virtual void subscribe( std::string subscriberId, OnLogRecord callback ) = 0;
    virtual void unsubscribe( std::string subscriberId ) = 0;
};

}  // namespace Watcher
}  // namespace HubPlatform
