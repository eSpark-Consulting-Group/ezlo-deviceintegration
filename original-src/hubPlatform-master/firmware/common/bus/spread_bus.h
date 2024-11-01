#pragma once

#include "base_bus.h"

#include <sp.h>

#include <string>
#include <thread>
#include <atomic>

class SpreadBus : public BaseBus {
public:
    SpreadBus( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts );
    void start() override;
    void stop() override;
    void join( const char* group ) override;
    bool multicast( const char* group, int msgClass, const std::string& msg ) override;
    bool isRegularMessage( int msgType ) const override;

    virtual void run();
    void checkMulticastError( const std::string& message, int res ) const;
    int maxMessageLength() const override;

private:
    void reconnect();

private:
    std::string m_spreadName;
    bus_rx_data_t rx_data;
    mailbox m_mbox;
    sp_time m_timeout;
    std::thread m_thread;
    std::atomic_bool m_stopping;
};
