
#include "networkd/tests/fakes/fake_at_executor.h"

namespace HubPlatform {
namespace Network {
namespace GSM {

FakeATExecutor::FakeATExecutor( const CommandReplyMap& commandReplymap )
: m_commandReplymap( commandReplymap ) {}

std::string FakeATExecutor::execute( const std::string& command, const std::chrono::milliseconds& waitForAnswer ) {
    auto it = m_commandReplymap.find( command );
    if( it != m_commandReplymap.end() ) {
        return it->second;
    }

    return m_unknownCommand;
}

bool FakeATExecutor::isInitialized() const {
    return true;
}

bool FakeATExecutor::lock() {
    return true;
}

void FakeATExecutor::unlock() {}

bool FakeATExecutor::isAccessible() const {
    return true;
}

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
