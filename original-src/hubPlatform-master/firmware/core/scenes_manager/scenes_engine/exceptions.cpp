#include "scenes_manager/scenes_engine/exceptions.h"

#include <utility>

namespace SceneEngine {

SceneException::SceneException( Errors::Error error ) noexcept
: m_error( error )
, m_what() {}

SceneException::SceneException( Errors::Error error, const char* what ) noexcept
: m_error( error )
, m_what( ( what ) ? what : "" ) {}

SceneException::SceneException( Errors::Error error, std::string what ) noexcept
: m_error( error )
, m_what( std::move( what ) ) {}

SceneException::SceneException( Errors::Error error, std::string badField, const char* what ) noexcept
: m_error( error )
, m_what( ( what ) ? what : "" )
, m_badField( std::move( badField ) ) {}

SceneException::SceneException( const SceneException& error ) noexcept
: m_error( error.m_error )
, m_what( error.m_what ) {}

const char* SceneException::what() const noexcept {
    return m_what.c_str();
}

Errors::Error SceneException::getError() const {
    return m_error;
}

const std::string& SceneException::getBadField() const {
    return m_badField;
}

}  // namespace SceneEngine
