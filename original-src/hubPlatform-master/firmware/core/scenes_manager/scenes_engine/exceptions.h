#pragma once

#include "jsonrpc/error_codes.h"

#include <exception>
#include <string>

namespace SceneEngine {

class SceneException : public std::exception {
public:
    SceneException( SceneException&& error ) noexcept = default;
    SceneException( const SceneException& error ) noexcept;

    explicit SceneException( Errors::Error error ) noexcept;
    SceneException( Errors::Error error, const char* what ) noexcept;
    SceneException( Errors::Error error, std::string what ) noexcept;
    SceneException( Errors::Error error, std::string badField, const char* what ) noexcept;

    ~SceneException() noexcept override = default;

    const char* what() const noexcept override;
    Errors::Error getError() const;
    const std::string& getBadField() const;

private:
    const Errors::Error m_error;
    std::string m_what;
    std::string m_badField;
};

}  // namespace SceneEngine
