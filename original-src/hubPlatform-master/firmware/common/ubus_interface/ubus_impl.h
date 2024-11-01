#pragma once

#include "ubus_interface.h"

#include <string>
#include <functional>
#include <memory>

struct ubus_context;
struct blob_attr;
struct ubus_request;

namespace HubPlatform {
namespace Ubus {

class UBusImpl : public UBusInterface {
public:
    UBusImpl() = default;
    UBusImpl( const UBusImpl& ) = delete;
    UBusImpl& operator=( UBusImpl const& ) = delete;

    bool connect( const char* uBusSocket ) override;
    bool isConnected() const override;
    bool addUBusObject( ubus_object& uBusObject ) override;

    bool sendCommand( const std::string& objName,
                      const std::string& command,
                      const std::string& msgKey,
                      const std::string& msgValue,
                      const replyCallback& replyFunction ) override;

    bool sendCommand( const std::string& objName,
                      const std::string& command,
                      const std::string& msgKey,
                      const std::string& msgValue,
                      std::string& commandReply ) override;

    ~UBusImpl() override;
    ubus_context* getContext() override;
    int checkConnection( const std::string& objName ) override;
    uint& getObjId() override;
    void breakUloop() override;

private:
    static void runCallback( ubus_request* req, int type, blob_attr* msg );

private:
    ubus_context* m_context = nullptr;
    uint m_objId = 0;

    int m_replyWaitTimeOut = 5;
};

}  // namespace Ubus
}  // namespace HubPlatform
