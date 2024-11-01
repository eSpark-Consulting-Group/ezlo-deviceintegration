#pragma once

#include "watcher/system_log/system_log_interface.h"

#include <map>

namespace HubPlatform {
namespace UT {

class FakeSystemLog : public Watcher::SystemLogInterface {
public:
    void subscribe( std::string subscriberId, OnLogRecord callback ) override {
        m_subscribers.emplace( std::move( subscriberId ), std::move( callback ) );
    }

    void unsubscribe( std::string subscriberId ) override {
        m_subscribers.erase( subscriberId );
    }

    void publishLogRecord( const std::string& logRecord ) {
        for( auto& element : m_subscribers ) {
            element.second( logRecord );
        }
    }

    std::map< std::string, OnLogRecord > m_subscribers;
};

}  // namespace UT
}  // namespace HubPlatform
