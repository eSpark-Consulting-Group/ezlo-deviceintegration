#pragma once

#include "system_monitor/process.h"

#include <string>
#include <vector>
#include <list>

namespace HubPlatform {
namespace SystemMonitor {

class Service {
public:
    using ProcessLabel = std::string;
    using ProcessCommand = std::string;
    using ProcessInfoList = std::vector< std::pair< ProcessLabel, ProcessCommand > >;

    static Service& serviceInstance();
    int run();

private:
    Service( ProcessInfoList processInfoList );
    void setUp();
    void tearDown();
    static void signalHandler( int signalNumber );

private:
    ProcessInfoList m_processInfoList;
    std::list< Process > m_processList;
    bool m_canRun{ true };
};

}  // namespace SystemMonitor
}  // namespace HubPlatform
