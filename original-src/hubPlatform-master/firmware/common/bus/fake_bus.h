#pragma once

#include "bus/base_bus.h"

class FakeBus : public BaseBus {
public:
    FakeBus( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts );
    bool isRegularMessage( int msgType ) const override;
    void start() override;
    void stop() override;
    void join( const char* group ) override;
    bool multicast( const char* group, int msgClass, const std::string& msg ) override;
};
