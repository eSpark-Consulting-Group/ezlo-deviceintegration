#include "message.h"
#include <tiny/tiny_utils.h>

namespace rpc {

Message::Message() = default;
Message::~Message() = default;

bool Message::parse( const std::string& strMessage ) {
    return parseJson( strMessage, joMessage_ );
}

bool Message::answerPrepare() {
    return ::answerPrepare( joMessage_, joAnswer_ );
}

void Message::copySender() {
    ::copySender( joMessage_, joAnswer_ );
}

bool Message::getMethod( std::string& strMethod ) {
    return getField( joMessage_, "method", joAnswer_, strMethod );
}
bool Message::getMethod( std::string& strMethod ) const {
    return ( const_cast< Message* >( this ) )->getMethod( strMethod );
}
std::string Message::getMethod() const {
    std::string strMethod;
    if( !getMethod( strMethod ) )
        strMethod.clear();
    return strMethod;
}

bool Message::getId( std::string& strID ) {
    return getField( joMessage_, "id", joAnswer_, strID );
}
bool Message::getId( std::string& strID ) const {
    return ( const_cast< Message* >( this ) )->getId( strID );
}
std::string Message::getId() const {
    std::string strID;
    if( !getId( strID ) )
        strID.clear();
    return strID;
}

bool Message::getParams( JsonObject& joParams ) {
    return getField( joMessage_, "params", joAnswer_, joParams );
}
bool Message::getParams( JsonObject& joParams ) const {
    return ( const_cast< Message* >( this ) )->getParams( joParams );
}
JsonObject& Message::getParams() {  // auto-instantiates params
    if( joMessage_.count( "params" ) == 0 || joMessage_[ "params" ].type() != Json::value_t::object )
        joMessage_[ "params" ] = JSON_EMPTY_OBJ;
    return joMessage_[ "params" ];
}
const JsonObject& Message::getParams() const {
    if( joMessage_.count( "params" ) == 0 ) {
        // Log::error( cc::error("Expected \"params\" field") );
        static const JsonObject g_joEmpty( JSON_EMPTY_OBJ );
        return g_joEmpty;
    }
    return joMessage_[ "params" ];
}

bool Message::getSerial( std::string& strSerial ) {
    return getNonEmptyField( getParams(), "serial", joAnswer_, strSerial );
}
bool Message::getSerial( std::string& getSerial ) const {
    return ( const_cast< Message* >( this ) )->getSerial( getSerial );
}
std::string Message::getSerial()
    const {  // l_sergiy: this getSerial() version also trims lead/end spaces in returned string containing serial
    std::string strSerial;
    if( !getSerial( strSerial ) )
        strSerial.clear();
    strSerial = tiny::tools::trim_copy( strSerial );  // l_sergiy: this getSerial() version also trims lead/end spaces
                                                      // in returned string containing serial
    return strSerial;
}

const JsonObject& Message::getMessage() const {
    return joMessage_;
}
JsonObject& Message::getMessage() {
    return joMessage_;
}
void Message::setMessage( const JsonObject& jo ) {
    joMessage_ = jo;
}

bool Message::isEmptyAnswer() const {
    return joAnswer_.empty();
}
JsonObject& Message::getAnswer() {
    return joAnswer_;
}
const JsonObject& Message::getAnswer() const {
    return joAnswer_;
}
void Message::setAnswer( const JsonObject& jo ) {
    joAnswer_ = jo;
}

void Message::setError( Errors::Error error ) {
    ::setError( joAnswer_, error );
}
void Message::setExceptionError( const char* strWhat  // = nullptr
) {
    if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
        strWhat = "exception without description";
    setError( Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR );
    joAnswer_[ std::string( Errors::ERROR_FIELD ) ][ "exceptionDescription" ] = std::string( strWhat );
}

bool Message::hasParamsField( const std::string& strField ) const {
    return getParams().count( strField ) > 0;
}
bool Message::checkParamsField( const std::string& strField ) {
    if( hasParamsField( strField ) )
        return true;
    JsonObject joErrorMessage;
    Errors::getBadRequest( strField, joErrorMessage );
    setErrorObject( joAnswer_, joErrorMessage );
    return false;
}

JsonObject& Message::getResult() {
    if( joAnswer_.count( "result" ) == 0 )
        joAnswer_[ "result" ] = JSON_EMPTY_OBJ;
    else if( joAnswer_[ "result" ].type() != Json::value_t::object ) {
        joAnswer_.erase( "result" );
        joAnswer_[ "result" ] = JSON_EMPTY_OBJ;
    }
    return joAnswer_[ "result" ];
}
void Message::setResult( const JsonObject& joResult ) {
    joAnswer_[ "result" ] = joResult;
}

}  // namespace rpc
