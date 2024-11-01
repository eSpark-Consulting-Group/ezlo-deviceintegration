#pragma once

namespace HubPlatform {
namespace HttpLinks {

class HttpLinksError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

}  // namespace HttpLinks
}  // namespace HubPlatform