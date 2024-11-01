#pragma once

#include "cameras/tests/memory_file_sender.h"
#include "tiny/tiny_http.h"
#include <map>

namespace HubPlatform {

class HttpFileServer final {
public:
    HttpFileServer();
    ~HttpFileServer();
    int start( const std::string& addr, int port );

private:
    void onRequestComplete( tiny::http::request& req, tiny::http::response& res );
    void onChunkComplete( tiny::http::request& req );

private:
    uv_loop_t* m_loop;
    tiny::http::server* m_server = nullptr;
    std::map< std::string /* file name */, MemoryFileSender > m_outputs;
};
}  // namespace HubPlatform
