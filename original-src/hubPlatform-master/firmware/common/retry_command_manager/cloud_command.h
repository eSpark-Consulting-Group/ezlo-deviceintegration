#pragma once

#include "command.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_utils.h"
#include "common_utils/cpp14.h"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace HubPlatform {
class CloudCommand : public Command {
public:
    enum class ReceiveStatus : int { Ok = 0, Error, Timeout, None };

    using SendCommandCallback = std::function< void( const Json& ) >;
    using NotifyCloudCallback = std::function< void( const Json& ) >;
    using SuccesCallback = std::function< void( const Json& ) >;

    CloudCommand( int maxRetriesNumber,
                  const Json& data,
                  SendCommandCallback sendCallback,
                  SuccesCallback successCallback,
                  NotifyCloudCallback failCallback,
                  const std::string& id = "" );

    CloudCommand( const Json& storedRecord,
                  SendCommandCallback sendCallback,
                  SuccesCallback successCallback,
                  NotifyCloudCallback failCallback );

    ~CloudCommand() override = default;

    void execute() override;
    void onFailure() override;
    void decrementRemainingRetries() override;
    int getRemainingRetries() const override;
    Json data() override;
    std::string id() override;
    void onSuccess() override;
    void receive( bool status ) override;
    void fail() override;
    int getRetryIndex() const override;
    ExecuteStatus checkResult() override;

private:
    std::mutex m_lock;
    bool m_isSend = false;
    bool m_isReceived = false;
    int m_remainingRetries = 0;
    int m_maxRetries = 0;
    Json m_data;
    SendCommandCallback m_sendCallback;
    SuccesCallback m_successCallback;
    NotifyCloudCallback m_failCallback;
    std::string m_id = "";
    ReceiveStatus m_status = ReceiveStatus::None;
};

using SendCommandPtr = std::shared_ptr< CloudCommand >;

}  // namespace HubPlatform
