#pragma once

#include "watcher/system_log/system_log_interface.h"

#include "system_utils/process/process_reader.h"

#include <map>
#include <memory>
#include <mutex>

namespace HubPlatform {
namespace Watcher {

class SystemLog : public SystemLogInterface {
public:
    ~SystemLog();

    void subscribe( std::string subscriberId, OnLogRecord callback ) override;
    void unsubscribe( std::string subscriberId ) override;

    void start( std::string logCmd );
    void stop();

private:
    std::mutex m_mutex;
    std::string m_buffer;
    std::map< std::string, OnLogRecord > m_subscribers;
    std::unique_ptr< Process::ProcessReader > m_processReader;
};

}  // namespace Watcher
}  // namespace HubPlatform
