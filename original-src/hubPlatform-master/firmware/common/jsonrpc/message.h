#pragma once

#include <string>
#include <tiny/tiny_atomic_shared_ptr.h>
#include <tiny/tiny_ws.h>
#include <tiny/tiny_url.h>
#include <tiny/tiny_consolecolors.h>
#include <tiny/tiny_utils.h>
#include "jsonrpc.h"
#include "legacy_synchronization_traits.h"

namespace rpc {

class Message {
protected:
    JsonObject joMessage_ = JSON_EMPTY_OBJ;
    JsonObject joAnswer_ = JSON_EMPTY_OBJ;

public:
    Message();
    Message( const Message& ) = delete;
    Message( Message&& ) = delete;
    Message& operator=( const Message& ) = delete;
    Message& operator=( Message&& ) = delete;
    virtual ~Message();
    //
    bool parse( const std::string& strMessage );
    bool answerPrepare();
    void copySender();
    //
    bool getMethod( std::string& strMethod );
    bool getMethod( std::string& strMethod ) const;
    std::string getMethod() const;
    //
    bool getId( std::string& strID );
    bool getId( std::string& strID ) const;
    std::string getId() const;
    //
    bool getParams( JsonObject& joParams );
    bool getParams( JsonObject& joParams ) const;
    JsonObject& getParams();  // auto-instantiates params
    const JsonObject& getParams() const;
    //
    bool getSerial( std::string& serial );
    bool getSerial( std::string& serial ) const;
    std::string getSerial()
        const;  // l_sergiy: this getSerial() version also trims lead/end spaces in returned string containing serial
    //
    const JsonObject& getMessage() const;
    JsonObject& getMessage();
    void setMessage( const JsonObject& jo );
    //
    bool isEmptyAnswer() const;
    JsonObject& getAnswer();
    const JsonObject& getAnswer() const;
    void setAnswer( const JsonObject& jo );
    //
    void setError( Errors::Error error );
    void setExceptionError( const char* strWhat = nullptr );
    //
    bool hasParamsField( const std::string& strField ) const;
    bool checkParamsField( const std::string& strField );
    //
    JsonObject& getResult();
    void setResult( const JsonObject& joResult );

    template < typename T >
    void setAnswerField( const std::string& strField, const T& refValue ) {
        joAnswer_[ strField ] = refValue;
    }
    template < typename T >
    void setMessageField( const std::string& strField, const T& refValue ) {
        joMessage_[ strField ] = refValue;
    }
    template < typename T >
    void setParamsField( const std::string& strField, const T& refValue ) {
        joMessage_[ "params" ][ strField ] = refValue;
    }
    template < typename T >
    void addToResult( const std::string& strField, const T& refValue ) {
        joAnswer_[ "result" ][ strField ] = refValue;
    }
    template < typename T >
    bool getParamsField( const std::string& strField, T& refResult ) {
        return getField( getParams(), strField, joAnswer_, refResult );
    }
    template < typename T >
    bool getParamsFieldAsArray( const std::string& strField, T& refResult ) {
        return getFieldAsArray( getParams(), strField, joAnswer_, refResult );
    }
    template < typename T >
    bool getOptionalParamsField( const std::string& strField, T& refResult ) {
        return getOptionalField( getParams(), strField, refResult );
    }

    // Switch lock mechanism for this type of pointer to use it's own lock instead of tiny global one to avoid
    // deadlocks.
    using GenericSender = tiny::ws::generic_sender;
    using GenericSenderPtr =
        tiny::retain_release_ptr< GenericSender, rpc::LegacySynchronizationTraits< GenericSender > >;

    virtual GenericSenderPtr getNetworkObject() const = 0;
    // virtual tiny::ws::generic_sender * getNetworkPtr() const { return getNetworkObject().get(); }
    virtual bool sendAnswer( bool bSendIfObjectOnly = false ) const {
        GenericSenderPtr px = getNetworkObject();
        if( !px )
            return false;
        const JsonObject& joAnswer = getAnswer();
        if( bSendIfObjectOnly ) {
            if( !joAnswer.is_object() )
                return false;
        }
        return px->sendMessage( json_write( joAnswer ) );
    }
    virtual bool postAnswer( bool bSendIfObjectOnly = false ) const {
        return sendAnswer( bSendIfObjectOnly );
    }
};  /// class Message

template < typename client_type = tiny::ws::client::client >
class TypedClientMessage : public Message {
public:
protected:
    using ClientPeerPtr = tiny::retain_release_ptr< client_type, rpc::LegacySynchronizationTraits< client_type > >;

public:
    TypedClientMessage( client_type* pClient )
    : pClient_( pClient ) {}
    TypedClientMessage( const TypedClientMessage& ) = delete;
    TypedClientMessage( TypedClientMessage&& ) = delete;
    TypedClientMessage& operator=( const TypedClientMessage& ) = delete;
    TypedClientMessage& operator=( TypedClientMessage&& ) = delete;
    ~TypedClientMessage() override = default;
    ClientPeerPtr getClientObject() const {
        return pClient_;
    }
    client_type* getClient() const {
        return pClient_.get();
    }
    GenericSenderPtr getNetworkObject() const override {
        GenericSenderPtr px = getClient();
        return px.get();
    }
    void setClient( client_type* pClient ) {
        pClient_ = pClient;
    }

protected:
    mutable ClientPeerPtr pClient_;
};  /// template class TypedClientMessage

template < typename peer_type = tiny::ws::server::peer >
class TypedServerMessage : public Message {
public:
    using ServerPeerPtr = tiny::retain_release_ptr< peer_type, rpc::LegacySynchronizationTraits< peer_type > >;

public:
    TypedServerMessage( peer_type* pPeer )
    : pPeer_( pPeer ) {}
    TypedServerMessage( ServerPeerPtr& pPeer )
    : pPeer_( pPeer ) {}
    TypedServerMessage( const TypedServerMessage& ) = delete;
    TypedServerMessage( TypedServerMessage&& ) = delete;
    TypedServerMessage& operator=( const TypedServerMessage& ) = delete;
    TypedServerMessage& operator=( TypedServerMessage&& ) = delete;
    ~TypedServerMessage() override {}
    ServerPeerPtr getPeerObject() const {
        return pPeer_;
    }
    // peer_type * getPeer() const { return pPeer_.get(); }
    GenericSenderPtr getNetworkObject() const override {
        ServerPeerPtr px = getPeerObject();
        return px.get();
    }
    void setPeer( peer_type* pPeer ) {
        pPeer_ = pPeer;
    }
    bool hasSender() const {
        return ( getMessage().count( "sender" ) > 0 ) ? true : false;
    }
    void setSender( const std::string& strSenderType ) {
        ServerPeerPtr pPeer = getPeerObject();
        if( !pPeer )
            return;
        setMessageField( "sender", JSON( "type" << strSenderType << "conn_id" << pPeer->getCidString() ) );
    }

protected:
    mutable ServerPeerPtr pPeer_;
};  /// template class TypedServerMessage

}  // namespace rpc
