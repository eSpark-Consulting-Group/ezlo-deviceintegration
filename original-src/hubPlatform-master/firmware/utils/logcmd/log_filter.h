#pragma once

#include <string>
#include <vector>
#include <set>

#include "logcmd/log_file.h"

namespace HubPlatform {
namespace LogCmd {

class LogFilter {
public:
    virtual ~LogFilter() = default;
    virtual bool operator()( const LogFile& logFile ) const = 0;
};

class LogFilterAny : public LogFilter {
public:
    bool operator()( const LogFile& logFile ) const override;
};

class LogFilterRegular : public LogFilter {
public:
    bool operator()( const LogFile& logFile ) const override;
};

class LogFilterRotated : public LogFilter {
public:
    bool operator()( const LogFile& logFile ) const override;
};

class LogFilterSelected : public LogFilter {
public:
    LogFilterSelected( const std::vector< std::string >& selectedNames );
    bool operator()( const LogFile& logFile ) const override;

private:
    std::set< std::string > m_selectedNames;
};

class LogFilterArchives : public LogFilter {
public:
    bool operator()( const LogFile& logFile ) const override;
};

}  // namespace LogCmd
}  // namespace HubPlatform
