#ifndef REPORTER_INTERFACE_H
#define REPORTER_INTERFACE_H

#include "json_utils/json_fwd.h"
#include <string>
#include <chrono>

class ReporterInterface {
public:
    virtual ~ReporterInterface() {}
    virtual std::string prepareReportData() = 0;
    virtual void sendReportData() = 0;
    virtual int postInterval() const = 0;
    virtual uint64_t getCPUTime() = 0;
    virtual JsonObject getProcessMemory() = 0;
    virtual std::string getProcStatus() = 0;
    virtual int64_t getProcessUptimeTicks( const std::string& pid ) = 0;
    virtual std::chrono::seconds getProcessUptimeSeconds( int64_t uptimeTicks ) = 0;
};

#endif  // REPORTER_INTERFACE_H
