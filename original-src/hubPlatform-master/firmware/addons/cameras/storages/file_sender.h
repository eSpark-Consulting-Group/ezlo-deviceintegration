#pragma once
#include "cameras/defs.h"
#include "cameras/storages/file_remover.h"

#include <string>
#include <stdexcept>

namespace HubPlatform {

class FileSenderException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class FileSender {
public:
    virtual bool finalizeFile() = 0;
    virtual bool sendData( uint8_t* data, size_t size ) = 0;
    virtual void setDynamicMetaData( Json metaData ){};
    virtual void setResultCallback( ResultCallback callback ) {
        m_resultCallback = std::move( callback );
    };
    virtual FileRemoverSharedPtr getRemover() {
        return std::make_shared< EmptyFileRemover >();
    };
    virtual void setFileValidity( bool isValid ) {
        m_isValid = isValid;
    }
    virtual ~FileSender() = default;

protected:
    bool m_isValid = true;
    ResultCallback m_resultCallback = []( const Json& ) {};
};

}  // namespace HubPlatform
