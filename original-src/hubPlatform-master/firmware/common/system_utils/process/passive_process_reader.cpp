#include "system_utils/process/passive_process_reader.h"

#include "common_utils/cpp14.h"

#include <unistd.h>
#include <sys/wait.h>

namespace HubPlatform {
namespace Process {

PassiveProcessReader::PassiveProcessReader( const std::string& cmd ) {
    auto stdoutCb = [this]( const char* buff, size_t num ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        m_stdoutBuff.append( buff, num );
    };

    auto stderrCb = [this]( const char* buff, size_t num ) {
        std::unique_lock< std::mutex > lock( m_mutex );
        m_stderrBuff.append( buff, num );
    };

    m_reader = std::make_unique< ProcessReader >( cmd, std::move( stdoutCb ), std::move( stderrCb ) );
}

PassiveProcessReader::~PassiveProcessReader() {
    m_reader.reset();
}

bool PassiveProcessReader::processStdout( const Functor& functor ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    return functor( m_stdoutBuff );
}

bool PassiveProcessReader::processStderr( const Functor& functor ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    return functor( m_stderrBuff );
}

size_t PassiveProcessReader::findInStdout( const std::string& str, size_t pos ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    return m_stdoutBuff.find( str, pos );
}

size_t PassiveProcessReader::findInStderr( const std::string& str, size_t pos ) {
    std::unique_lock< std::mutex > lock( m_mutex );
    return m_stderrBuff.find( str, pos );
}

void PassiveProcessReader::clearStdout() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_stdoutBuff.clear();
}

void PassiveProcessReader::clearStderr() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_stderrBuff.clear();
}

ProcessReader& PassiveProcessReader::process() {
    return *m_reader;
}

}  // namespace Process
}  // namespace HubPlatform
