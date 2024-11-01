#include "fake_bus.h"

FakeBus::FakeBus( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts )
: BaseBus( pname, rxCallback ) {}
bool FakeBus::isRegularMessage( int msgType ) const {
    return true;
}
void FakeBus::start() {}
void FakeBus::stop() {}
void FakeBus::join( const char* group ) {}
bool FakeBus::multicast( const char* group, int msgClass, const std::string& msg ) {
    return true;
}
