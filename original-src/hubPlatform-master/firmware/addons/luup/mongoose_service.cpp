#include "mongoose_service.h"

#include <thread>
#include <iostream>
#include <functional>

MongooseService::MongooseService( HubPlatform::Addons::Addon& addon, const std::string& webRoot )
: BaseService( addon, "3480", webRoot )
, m_context( nullptr )
, m_handleDataRequestWrapper( this ) {}

MongooseService::~MongooseService() {}

void MongooseService::start() {
    m_context = mg_start();

    if( mg_set_option( m_context, "root", getWebRoot().c_str() ) != 1 ) {
        throw std::runtime_error( "Failed to set webroot" );
    }

    if( mg_set_option( m_context, "ports", getPort().c_str() ) != 1 ) {
        throw std::runtime_error( "Failed to set port" );
    }

    mg_set_uri_callback( m_context, DataRequestDir.c_str(), m_handleDataRequestWrapper, NULL );
}

void MongooseService::handleDataRequest( mg_connection* connection, const mg_request_info* requestInfo, void* data ) {
    if( requestInfo->query_string ) {
        const auto query = tiny::http::uri::extract::query( requestInfo->query_string );
        const auto response = BaseService::handleDataRequest( query );
        mg_printf( connection,
                   "HTTP/1.1 200 OK\r\ncontent-Type: %s\r\n\r\n",
                   getResponseContentType( response.responseFormat ) );
        mg_write( connection, response.responseBody.c_str(), response.responseBody.size() );
    }
}

const char* MongooseService::getResponseContentType( ResponseFormat responseFormat ) {
    switch( responseFormat ) {
    case ResponseFormat::XML:
        return "text/xml";

    case ResponseFormat::JSON:
        return "application/json";

    case ResponseFormat::TEXT:
        return "text/plain";
    }

    return "text/plain";
}
