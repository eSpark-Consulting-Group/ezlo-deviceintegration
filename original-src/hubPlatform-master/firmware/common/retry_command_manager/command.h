#pragma once

#include "json_utils/json_fwd.h"

namespace HubPlatform {

enum class ExecuteStatus : int { AnswerOk, AnswerError, Fail };

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void decrementRemainingRetries() = 0;
    virtual void onFailure() = 0;
    virtual void onSuccess() = 0;
    virtual int getRemainingRetries() const = 0;
    virtual std::string id() = 0;
    virtual Json data() = 0;
    virtual void receive( bool status ) = 0;
    virtual void fail() = 0;
    virtual int getRetryIndex() const = 0;
    virtual ExecuteStatus checkResult() = 0;
};

using CommandPtr = std::shared_ptr< Command >;

}  // namespace HubPlatform
