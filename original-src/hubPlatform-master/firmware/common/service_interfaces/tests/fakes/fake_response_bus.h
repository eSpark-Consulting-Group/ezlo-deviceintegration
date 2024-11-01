#pragma once

#include "bus/fake_bus.h"
#include "service_interfaces/message.h"

#include <jsonrpc/jsonrpc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>

namespace HubPlatform {
namespace Services {
namespace UT {

class FakeResponseBus : public FakeBus {
public:
    FakeResponseBus( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts )
    : FakeBus( pname, rxCallback, silent, connectionAttempts ) {}

    bool multicast( const char* group, int msgClass, const std::string& msg ) override {
        auto json = Json::parse( msg );

        Services::Message message;
        message.group = std::string( group );
        message.msgClass = msgClass;
        message.msgSubClass = msgSubclassToKey( msgClass, json_get_str( json, "msg_subclass" ) );
        message.data = json[ "result" ];
        m_messages.push_back( std::move( message ) );

        return true;
    }

    Services::Message getMessage() const {
        if( m_messages.empty() ) {
            return Services::Message();
        }
        auto message = std::move( m_messages.back() );
        m_messages.pop_back();
        return message;
    }

    Services::Message getFirstMessage() const {
        if( m_messages.empty() ) {
            return Services::Message();
        }
        auto message = std::move( m_messages.front() );
        m_messages.pop_front();
        return message;
    }

private:
    mutable std::deque< Services::Message > m_messages;
};

class FakeResponseBridgeBus : public FakeResponseBus {
public:
    FakeResponseBridgeBus( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts )
    : FakeResponseBus( pname, rxCallback, silent, connectionAttempts ) {}

    bool multicast( const char* group, int msgClass, const std::string& msg ) override {
        if( !m_groups.empty() && m_groups.find( group ) == m_groups.end() ) {
            return false;
        }
        if( !m_msgClasses.empty() && m_msgClasses.find( msgClass ) == m_msgClasses.end() ) {
            return false;
        }
        auto callback = getRxCallback();
        bus_rx_data_t rxData;
        memcpy( rxData.msg, msg.c_str(), msg.size() );
        rxData.msg_size = msg.size();
        rxData.msg_type = msgClass;
        rxData.num_groups = 1;
        callback( &rxData );
        return FakeResponseBus::multicast( group, msgClass, msg );
    }

    void setGroupFilter( const std::set< std::string >& group ) {
        m_groups = group;
    }

    void setClassFilter( const std::set< int >& msgClass ) {
        m_msgClasses = msgClass;
    }

private:
    std::set< std::string > m_groups;
    std::set< int > m_msgClasses;
};

}  // namespace UT
}  // namespace Services
}  // namespace HubPlatform
