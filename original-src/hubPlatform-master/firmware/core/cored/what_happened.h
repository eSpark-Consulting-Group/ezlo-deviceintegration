#pragma once

#include "json_utils/json_fwd.h"
#include "bus/messagesclasses.h"

#include <string>
#include <functional>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class WhatHappened final {
public:
    WhatHappened( IDbClient& client );

    ~WhatHappened() = default;
    WhatHappened( const WhatHappened& cpy ) = delete;
    WhatHappened& operator=( const WhatHappened& cpy ) = delete;
    WhatHappened( WhatHappened&& mv ) = delete;
    WhatHappened& operator=( WhatHappened&& mv ) = delete;

    void saveDeviceLog( const JsonObject& deviceData, const MessageSubClasses msgSubclass );
    void saveItemLog( const JsonObject& itemUpdated );

    void saveRuleLog( const JsonObject& obj, const MessageSubClasses event );
    void saveRuleDeleted( const JsonObject& args );  // see implementation. Sender + id the same as for saveRuleLog ????

    void saveDeviceAdded( const JsonObject& deviceData );
    void saveDeviceRemoved( const JsonObject& device );
    void saveUserAssign( const JsonObject& args, bool state );
    void saveUserBind( const JsonObject& args, bool state );
    // TODO: it should be devided from this interface.
    void getWhatHappened( const JsonObject& params, JsonObject& answer );

private:
    void saveRuleLog( const std::string& ruleId, const MessageSubClasses event, const std::string& source );
    void saveRuleLog( const std::string& ruleId,
                      const MessageSubClasses event,
                      const std::vector< std::string >& itemIdsList );

    enum RuleState { ENABLED = 0, DISABLED, CREATED, CHANGED, DELETED };

    // use DB specific name
    void insertWithAge( const JsonObject& obj );
    void saveRuleChangeState( const JsonObject& rule, const RuleState state );
    void saveRuleExecuted( const JsonObject& rule, const MessageSubClasses event );

private:
    IDbClient& dbClient;
};

JsonObject getWhatHappened( IDbClient& client, const JsonObject& params );

}  // namespace CoreD
}  // namespace HubPlatform
