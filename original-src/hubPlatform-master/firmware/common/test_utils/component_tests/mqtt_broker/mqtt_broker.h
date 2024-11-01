#pragma once

#include "test_utils/temporary_file_wrapper.h"
#include "system_utils/process/forker.h"

#include <string>
#include <stdexcept>
#include <memory>

namespace HubPlatform {
namespace Tests {

class MQTTBrokerError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit MQTTBrokerError( const std::string& msg )
    : runtime_error( msg ){};
};

class MQTTBroker {
public:
    MQTTBroker( std::string binaryPath = "" );
    ~MQTTBroker();
    void start( size_t port, Process::Forker::OnProcessCompletion onFinished );
    void stop();
    std::string getURI();

private:
    Process::Forker::Instance m_processForker;
    pid_t m_pid = 0;
    std::unique_ptr< TemporaryFileWrapper > m_brokerLog;

    std::string m_binaryPath;
    size_t m_port = 0;
};

}  // namespace Tests
}  // namespace HubPlatform
