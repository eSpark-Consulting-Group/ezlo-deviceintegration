#pragma once

#include "networkd/gsm/nm_gsm.h"
#include "networkd/gsm/nm_at_executor.h"

#include <map>
#include <string>

namespace HubPlatform {
namespace Network {
namespace GSM {

class FakeATExecutor final : public IATExecutor {
public:
    using CommandReplyMap = std::map< std::string, std::string >;
    FakeATExecutor( const CommandReplyMap& commandReplymap );

    std::string execute( const std::string& command,
                         const std::chrono::milliseconds& waitForAnswer = std::chrono::milliseconds( 300 ) ) override;
    bool isInitialized() const override;

    bool lock() override;
    void unlock() override;
    bool isAccessible() const override;

    static constexpr const char* m_unknownCommand = "unknown command";

private:
    CommandReplyMap m_commandReplymap;
};

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
