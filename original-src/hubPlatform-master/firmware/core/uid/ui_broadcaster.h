#pragma once

#include "uid/ui_service.h"
#include <tiny/tiny_ws.h>

class uid_broadcaster : public tiny::ws::generic_sender {
public:
    uid_broadcaster();
    virtual ~uid_broadcaster();
    bool sendMessage( const std::string& msg, int nOpCode ) override;
    bool sendMessageText( const std::string& msg ) override;
    bool sendMessageBinary( const std::string& msg ) override;
    bool postMessage( const std::string& msg );
    bool postMessage( const std::string& msg, int nOpCode );
    bool sendMessageNoLog( const std::string& msg, int nOpCode );
    bool postMessageNoLog( const std::string& msg, int nOpCode );

private:
    HubPlatform::Uid::Service& m_uiService;
};  /// class uid_broadcaster

using UidBroadcasterPtr =
    tiny::retain_release_ptr< uid_broadcaster, rpc::LegacySynchronizationTraits< uid_broadcaster > >;
extern UidBroadcasterPtr& global_uid_broadcaster();
