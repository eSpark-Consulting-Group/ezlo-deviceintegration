#pragma once

namespace HubPlatform {

template < typename T >
class Badge final {
private:
    friend T;
    Badge() = default;
};

}  // namespace HubPlatform
