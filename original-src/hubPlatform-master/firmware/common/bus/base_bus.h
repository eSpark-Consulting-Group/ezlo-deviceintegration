#pragma once

#include "json_utils/json_wrapper.h"

#include <functional>
#include <memory>

namespace Limits {
constexpr int MaxMessageLength = 102400;
constexpr int MaxMulticastMessageLength = 102400;
constexpr int MaxGroups = 100;
constexpr int MaxGroupName = 32;
};  // namespace Limits

typedef struct {
    char msg[ Limits::MaxMessageLength ], sender[ Limits::MaxGroupName ],
        target_groups[ Limits::MaxGroups ][ Limits::MaxGroupName ];
    int num_groups, service_type, endian_mismatch, msg_size;
    std::int16_t msg_type;
} bus_rx_data_t;

using RxCallBack = std::function< void( const bus_rx_data_t* ) >;

class MaxMessageLengthError : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class BaseBus {
public:
    BaseBus( const std::string& privateName, const RxCallBack& rxCallBack );
    virtual ~BaseBus() = default;

    virtual bool isRegularMessage( int msgType ) const = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void join( const char* group ) = 0;
    virtual bool multicast( const char* group, int msgClass, const std::string& msg ) = 0;

    bool send( const char* group, int msgClass, int msgSubclass, const JsonObject& msg );
    bool send( const char* group, int msgClass, int msgSubClass, const std::string& msg );

    virtual int maxMessageLength() const;
    virtual void checkMessageLength( int messageLength );

    static bool isBroadcastMessage( int msgClass );

protected:
    const std::string& getPrivateName() const;
    const RxCallBack& getRxCallback() const;

    static bool isUiBroadcastMessage( int msgClass );

private:
    std::string m_privateName;

    RxCallBack m_rxCallback;
};

using BaseBusPtr = std::unique_ptr< BaseBus, void ( * )( BaseBus* ) >;
BaseBusPtr createBus( const std::string& privateName,
                      const RxCallBack& rxCallBack,
                      bool silent,
                      int connectionAttempts );
