#pragma once

#include <string>

namespace HubPlatform {
namespace Uid {
namespace Extensions {

class BusMessenger;
class BusExtension;
class HttpExtension;

// This interface is an entrypoint to your UI extension capabilities
class Extension {
public:
    using BusExtension = Uid::Extensions::BusExtension;
    using HttpExtension = Uid::Extensions::HttpExtension;

public:
    virtual ~Extension() = default;

    virtual const std::string& getName() const = 0;

    // Can be nullptr if your extension is not interested in bus communications
    virtual BusExtension* getBusExtension() = 0;

    // Can be nullptr if your extension is not interested in handling http requests
    virtual HttpExtension* getHttpExtension() = 0;
};

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
