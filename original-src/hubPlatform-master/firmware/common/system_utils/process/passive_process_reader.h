#pragma once

#include "system_utils/process/process_reader.h"

#include <string>
#include <functional>
#include <mutex>
#include <memory>

namespace HubPlatform {
namespace Process {

class PassiveProcessReader {
public:
    // NOTE: Functor must quickly process the buffer
    using Functor = std::function< bool( const std::string& buffer ) >;

    PassiveProcessReader( const std::string& cmd );
    ~PassiveProcessReader();

    bool processStdout( const Functor& functor );
    bool processStderr( const Functor& functor );

    size_t findInStdout( const std::string& str, size_t pos = 0 );
    size_t findInStderr( const std::string& str, size_t pos = 0 );

    void clearStdout();
    void clearStderr();

    ProcessReader& process();

private:
    std::unique_ptr< ProcessReader > m_reader;
    std::mutex m_mutex;
    std::string m_stdoutBuff;
    std::string m_stderrBuff;
};

}  // namespace Process
}  // namespace HubPlatform
