#pragma once

#include <map>
#include <vector>
#include <json_utils/json_utils.h>

namespace HubPlatform {

namespace CoreD {

class WebRtcGroupInfo {
public:
    WebRtcGroupInfo();
    Json getMemoryLimitAndUsage();
    Json getOomControl();

private:
    Json extractOomControl( const std::string& outputString );
    Json extractMemoryLimitAndUsage( const std::string& outputString );

private:
    std::unique_ptr< FILE, decltype( &pclose ) > pipePtr{ nullptr, &pclose };
    std::ostringstream m_groupInfo;
};

}  // namespace CoreD
}  // namespace HubPlatform
