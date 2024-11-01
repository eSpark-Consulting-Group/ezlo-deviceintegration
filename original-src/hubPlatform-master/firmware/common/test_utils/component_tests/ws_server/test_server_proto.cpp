
#include "test_utils/component_tests/ws_server/test_server_proto.h"

#include "device_model/h/permission.h"

namespace HubPlatform {
namespace Tests {

TestServerProto::TestServerProto()
: Parser(
      {
          { "ping", std::bind( &TestServerProto::ping, this, std::placeholders::_1 ) },
          { "register", std::bind( &TestServerProto::registerHub, this, std::placeholders::_1 ) },
          { "cloud.access_keys_controller_sync", std::bind( &TestServerProto::ignore, this, std::placeholders::_1 ) },
          { "cloud.controller_subscribe_abstracts",
            std::bind( &TestServerProto::ignore, this, std::placeholders::_1 ) },
      },
      std::bind( &TestServerProto::unknown, this, std::placeholders::_1 ) ) {}

bool TestServerProto::processCmd( TestPeerMessage& rpcMessage ) {
    return processCmd( rpcMessage, [this]( const std::string& method, rpc::Message& rpcMessage ) {
        getHandler( method )( dynamic_cast< TestPeerMessage& >( rpcMessage ) );
    } );
}

bool TestServerProto::processCmd( rpc::Message& rpcMessage, TestServerProto::CallMethod callMethod ) {
    const JsonObject& joMessage = rpcMessage.getMessage();
    JsonObject& joAnswer = rpcMessage.getAnswer();

    std::string broadcastId;
    if( json_get( joMessage, "id", broadcastId ) && ( broadcastId == "ui_broadcast" ) ) {
        Log::warn( cc::warn( "It's a broadcast message. It shouldn't be here: " ) + cc::j( joMessage ) );
        return false;
    }

    JsonObject joError = JSON_EMPTY_OBJ;
    if( json_get_obj( joMessage, "error", joError ) ) {
        Log::warn( cc::warn( "It's a error message. It shouldn't be here: " ) + cc::j( joMessage ) );
        return false;
    }

    std::string method;
    if( !json_get( joMessage, "method", method ) ) {
        method.clear();
        Log::warn( cc::warn( "The method isn't found. It shouldn't be here: " ) + cc::j( joMessage ) );
        return false;
    }

    if( method.empty() ) {
        Log::warn( cc::warn( "JSON doesn't contain method name. It shouldn't be here: " ) + cc::j( joMessage ) );
        return false;
    }

    std::string id;
    if( !json_get( joMessage, "id", id ) ) {
        id.clear();
        Log::warn( cc::warn( "\"id\" is empty in JSON for method \"" + method + "\" (NMA Test Emulator)" ) );
        return false;
    }

    JsonObject joSender = JSON_EMPTY_OBJ;
    if( !json_get_obj( joMessage, "sender", joSender ) ) {
        joSender = JSON_NULL_VAL;
    }

    Log::info( cc::info( "Component test will process command " ) + cc::warn( method ) );
    callMethod( method, rpcMessage );

    if( joAnswer.is_object() && joAnswer.count( Errors::ERROR_FIELD ) > 1 )
        joAnswer.erase( "result" );

    else if( !joAnswer.empty() ) {
        if( !joSender.is_null() ) {
            joAnswer[ "sender" ] = joSender;
        }

        if( !id.empty() )
            json_add_str( joAnswer, "id", id );

        if( joAnswer.count( Errors::ERROR_FIELD ) == 0 ) {
            json_add_null( joAnswer, Errors::ERROR_FIELD );
        }
    }

    return true;
}

void TestServerProto::unknown( TestPeerMessage& rpcMessage ) {
    Log::warn( cc::warn( "JSON contains unknown method name. It shouldn't be here: " ) +
               cc::j( rpcMessage.getMessage() ) );
}
void TestServerProto::ping( TestPeerMessage& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    json_add_str( joAnswer, "reply", "Pong" );
}

void TestServerProto::registerHub( TestPeerMessage& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer = {
        { "error", nullptr }, { "method", "registered" }, { "result", { { "cache-hash", "" }, { "cache-owner", "" } } }
    };

    std::string serial;

    if( !rpcMessage.getParamsField( "serial", serial ) ) {
        serial.clear();
        Log::error( cc::info( "Hub without " ) + cc::error( "serial" ) + cc::info( " tries to register: " ) +
                    cc::j( rpcMessage.getMessage() ) );
        return;
    }

    rpcMessage.getPeerObject()->onHubRegistered( serial );
}

void TestServerProto::ignore( HubPlatform::Tests::TestPeerMessage& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    joAnswer = nullptr;
}

}  // namespace Tests
}  // namespace HubPlatform
