#pragma once

#include <string>

enum class ResponseFormat { TEXT, JSON, XML };

struct ServiceResponse {
    ServiceResponse( ResponseFormat format, std::string body )
    : responseFormat( format )
    , responseBody( std::move( body ) ) {}

    ServiceResponse( const ServiceResponse& response ) = default;
    ServiceResponse( ServiceResponse&& response ) = default;

    ServiceResponse& operator=( const ServiceResponse& response ) = default;
    ServiceResponse& operator=( ServiceResponse&& response ) = default;

    ResponseFormat responseFormat;
    std::string responseBody;
};
