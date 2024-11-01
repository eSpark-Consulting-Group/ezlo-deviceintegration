#pragma once

#include "events_loop/epoll_events_loop.h"
#include "json_utils/json_fwd.h"
#include "database_client/idbclient.h"

#include <string>
#include <set>
#include <stdexcept>
#include <map>

namespace HubPlatform {
namespace HardwareManager {

namespace UT {
class NetlinkManagerTestSuite;
}

enum class HotPlugEventDeviceClass { USB, ETHERNET };
enum class HotPlugEventType { ADD, REMOVE };

struct HotPlugEvent {
    HotPlugEventDeviceClass deviceClass;
    HotPlugEventType type;
    std::string devicePath;

    /*
     * {
     *   "deviceClass": "usb" | "ethernet",
     *   "type": "add" | "remove",
     *   "devicePath": string
     * }
     */
    Json toJson() const;
};

class NetlinkManagerException : public std::runtime_error {
    using runtime_error::runtime_error;
};

/* this class represents a set of callbacks get called respect to netlink events */
class INetlinkEventsHandler {
public:
    virtual ~INetlinkEventsHandler();
    virtual void handleHotPlugEvent( const HotPlugEvent& event ) = 0;

    /* in some circumstances an i2c bus can be locked (no actions on the bus can be performed),
     * after it gets recovered an event is generated with a name of a recovered bus */
    virtual void handleI2CBusRecovered( const std::string& devicePath ) = 0;
};

class INetlinkSocketsCreator {
public:
    virtual ~INetlinkSocketsCreator();
    virtual int createNetlinkUeventSocket( uint32_t netlinkGroups ) = 0;
};

/* this class handles netlink events such as hotplug events [and may be used to monitor
 * gpio lines/buttons changes, ethernet cable insertion/removing, network routes changes].
 * This class uses an additional thread to listen for netlink events on netlink socket(s),
 * INetlinkEventsHandler's callbacks are called on behalf of this thread */
class NetlinkManager final {
    friend class UT::NetlinkManagerTestSuite;

    /* representation of an event used as kind of IPC between kernel and userspace - uevent */
    struct UEvent {
        enum class Actions { FAILED = -2, UNKNOWN = -1, ADD, BIND, CHANGE, MOVE, REMOVE, UNBIND, ONLINE };
        enum class SubSystems { FAILED = -2, UNKNOWN = -1, USB, NETWORK, I2C };

        static const std::map< std::string, Actions > actionNames;
        static const std::map< std::string, SubSystems > subsystemNames;

        explicit UEvent( const std::string& uevent );

        std::string header;

        // mandatory uevent's fields
        Actions action;
        SubSystems subsystem;
        std::string devpath;

        // optional uevent's fields
        using Key = std::string;
        using Value = std::string;
        std::map< Key, Value > optionalFields;
    };

public:
    NetlinkManager( IDbClient& dbClient, INetlinkEventsHandler& netlinkEventsHandler );
    ~NetlinkManager();

private:
    NetlinkManager( IDbClient& dbClient,
                    INetlinkEventsHandler& netlinkEventsHandler,
                    std::unique_ptr< INetlinkSocketsCreator > netlinkSocketsCreator );

    void syncEventsEnvironment();
    void syncUSBDeviceEvents();
    std::set< std::string > getRealUSBDevices();
    std::set< std::string > getDbUSBDevices();

    void registerEventSources();
    void handleNetlinkUevents();

    void handleUSBUevent( const UEvent& uevent );
    void handleUSBAddUevent( const UEvent& uevent );
    void handleUSBRemoveUevent( const UEvent& uevent );

    void handleI2CUevent( const UEvent& uevent );
    void handleI2COnlineUevent( const UEvent& uevent );

    void issueUSBAddHotPlugEvent( const std::string& devicePath );
    void issueUSBRemoveHotPlugEvent( const std::string& devicePath );

private:
    IDbClient& m_dbClient;
    INetlinkEventsHandler& m_netlinkEventsHandler;
    EventsLoop::EpollEventsLoop m_epollLoop;
    int m_netlinkUeventSocketFd = -1;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
