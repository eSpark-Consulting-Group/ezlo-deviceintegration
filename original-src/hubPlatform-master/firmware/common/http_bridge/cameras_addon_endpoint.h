#pragma once

#include "http_bridge/endpoint.h"
#include "http_bridge/endpoint_handlers.h"
#include "system_utils/filesystem.h"
#include <memory>
#include <mutex>

namespace HubPlatform {
namespace HttpBridge {

class ApiBridge;
class SessionData;

class ICamerasEndpointHandler {
public:
    using File = std::shared_ptr< FILE >;

    ~ICamerasEndpointHandler() = default;

    virtual void getSnapshot( CallbackParams params,
                              const std::string& streamId,
                              bool cached,
                              const std::string& etag ) = 0;
    virtual size_t getFileSize( CallbackParams params );
    virtual size_t readFile( CallbackParams params, char* dst, size_t size );
    virtual void removeFile( CallbackParams params );
    virtual bool isHandlerValid() = 0;
    virtual std::string getErrorMessage( CallbackParams params );
    virtual void setErrorMessage( CallbackParams params, const std::string& errorMessage );
    virtual time_t getCachedEndTimestamp( HttpBridge::CallbackParams params ) = 0;
    virtual time_t getCachedTimestamp( HttpBridge::CallbackParams params ) = 0;

protected:
    void callbackOnWritable( CallbackParams params );

    struct FileAutoRemover {
        ~FileAutoRemover() {
            if( !m_usingCache ) {
                filesystem::removeFile( filePath.c_str() );
            }
        }
        std::string filePath;
        bool m_usingCache;
    };

    struct FileState {
        File file;
        std::string error;
        size_t fileSize;
        FileAutoRemover fileAutoRemover;
        std::string streamId;
        std::string filePath;
        std::string requestId;
    };

    std::mutex m_mutex;
    std::map< void*, FileState > m_files;
};

class EmptyCamerasEndpointHandler : public ICamerasEndpointHandler {
public:
    void getSnapshot( CallbackParams params,
                      const std::string& streamId,
                      bool cached,
                      const std::string& requestId ) override;
    bool isHandlerValid() override;
    virtual time_t getCachedEndTimestamp( HttpBridge::CallbackParams params ) override;
    virtual time_t getCachedTimestamp( HttpBridge::CallbackParams params ) override;
};

class CamerasAddonEndpoint : public Endpoint, public EndpointHandlers< CamerasAddonEndpoint > {
public:
    explicit CamerasAddonEndpoint( ApiBridge& apiBridge );
    static EndpointPtr makeEndpoint( ApiBridge& apiBridge );

public:
    ~CamerasAddonEndpoint() override = default;
    int endpointCallback( struct lws* wsi,
                          enum lws_callback_reasons reason,
                          void* user,
                          void* in,
                          size_t len ) override;

    void setEndpointHandler( std::unique_ptr< ICamerasEndpointHandler >&& handler );

protected:
    virtual int httpDummy( CallbackParams params );
    virtual int onHttp( CallbackParams params, BufferPosition bufPos );
    virtual int onHttpWriteable( CallbackParams params, BufferPosition bufPos );
    virtual int onClosedHttp( CallbackParams params, BufferPosition bufPos );

private:
    static void setEtagHeaderIfNeeded( HubPlatform::HttpBridge::SessionData& sessionData );
    int writeHttpHeaders( SessionData& sessionData,
                          int httpStatus,
                          const char* contentType,
                          uint8_t* start,
                          uint8_t*& p,
                          uint8_t* end,
                          bool closeSession,
                          CallbackParams params );

private:
    std::unique_ptr< ICamerasEndpointHandler > m_endpointHandler;
    ApiBridge& m_api;

    static const int ConnectionTimeout = 70;

    static bool isCachedParamInRequest( SessionData& sessionData );
    bool isModifiedCachedImage( CallbackParams& params, SessionData& sessionData );
};

}  // namespace HttpBridge
}  // namespace HubPlatform
