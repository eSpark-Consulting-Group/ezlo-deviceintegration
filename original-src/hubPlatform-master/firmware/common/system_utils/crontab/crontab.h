#pragma once

#include "system_utils/file_lock.h"

#include <string>
#include <utility>
#include <memory>
#include <list>
#include <array>
#include <stdexcept>

namespace HubPlatform {

namespace UT {
class CrontabTestSuite;
}  // namespace UT

namespace system_utils {

class CrontabError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit CrontabError( const std::string& msg )
    : runtime_error( msg ){};
};

class CrontabInvalidFileError : public CrontabError {
public:
    explicit CrontabInvalidFileError( const std::string& msg )
    : CrontabError( msg ){};
};

class CrontabRecordError : public CrontabError {
public:
    explicit CrontabRecordError( const std::string& msg )
    : CrontabError( msg ){};
};

class Crontab {
    friend class UT::CrontabTestSuite;

public:
    using Schedule = std::array< std::string, 5 >;
    using Command = std::string;
    using Record = std::pair< Schedule, Command >;

    Crontab();
    Crontab( std::string filename );

    void lock();
    void unlock();
    void load();
    void write();
    void restartCron();

    void addRecord( Record record );
    void deleteRecord( const Command& command );
    std::unique_ptr< Record > findRecord( const Command& command );
    void updateCommandSchedule( const Command& command, Schedule schedule );
    bool isUpdated();

private:
    std::list< Record >::iterator findByCommand( const Command& command );

private:
    HubPlatform::filesystem::FileLock m_crontabLock;
    std::string m_filename;
    bool m_updated = false;
    std::list< Record > m_records;
};

}  // namespace system_utils
}  // namespace HubPlatform
