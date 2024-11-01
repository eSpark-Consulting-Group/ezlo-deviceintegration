#pragma once
#include "json_utils/json_fwd.h"
#include "service_interfaces/service.h"
#include <functional>

namespace HubPlatform {
namespace CamerasRemoteInterface {

class CamerasRemoteInterface final {
public:
    using Callback = std::function< void( const Json& message ) >;

    CamerasRemoteInterface( Services::Service& hostService );

    /* @brief callback returns "filePath" field in successful case and
     * "error" field in unsuccessful case */
    void getStreamSnapshot( const std::string& streamId, Callback callback ) const;

private:
    Services::Service& m_hostService;
    std::string m_groupName;
};

}  // namespace CamerasRemoteInterface
}  // namespace HubPlatform
