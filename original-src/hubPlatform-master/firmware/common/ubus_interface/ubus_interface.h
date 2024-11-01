#pragma once

#include <memory>
#include <string>
#include <functional>
#include <sys/types.h>

struct ubus_object;
struct ubus_context;

namespace HubPlatform {
namespace Ubus {

class UBusInterface {
public:
    using replyCallback = std::function< void( std::string ) >;

    virtual ~UBusInterface() = default;
    virtual bool connect( const char* uBusSocket ) = 0;
    virtual bool isConnected() const = 0;
    virtual bool addUBusObject( ubus_object& uBusObject ) = 0;
    virtual bool sendCommand( const std::string& objName,
                              const std::string& command,
                              const std::string& msgKey,
                              const std::string& msgValue,
                              const replyCallback& replyFunction ) = 0;

    virtual bool sendCommand( const std::string& objName,
                              const std::string& command,
                              const std::string& msgKey,
                              const std::string& msgValue,
                              std::string& commandReply ) = 0;
    virtual ubus_context* getContext() = 0;
    virtual int checkConnection( const std::string& objName ) = 0;
    virtual uint& getObjId() = 0;
    virtual void breakUloop() = 0;
};

}  // namespace Ubus
}  // namespace HubPlatform
