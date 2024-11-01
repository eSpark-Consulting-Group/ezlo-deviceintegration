#pragma once
#include "cameras/storages/file_sender.h"
#include <curl/curl.h>

namespace HubPlatform {

class HttpFileSender : public FileSender {
public:
    explicit HttpFileSender( const std::string& url );
    ~HttpFileSender() override;

    bool finalizeFile() override;
    bool sendData( uint8_t* data, size_t size ) override;

private:
    static size_t readCallback( char* buffer, size_t size, size_t nitems, void* userdata );

private:
    CURL* m_curl;
    CURLM* m_multiCurl;
    struct curl_slist* m_chunk;

    struct MemoryBuffer {
        uint8_t* data = nullptr;
        size_t size = 0;
    } m_memoryBuffer;
};
}  // namespace HubPlatform
