#include "uid/extensions/ui_http_utils.h"

#include <libwebsockets.h>

namespace HubPlatform {
namespace Uid {
namespace Extensions {
namespace Utils {

int writeError( struct lws* wsi, int code, const std::string& errorMessage ) {
    static const std::string prefix = "ERROR:";
    const size_t contentLength = prefix.size() + errorMessage.size();
    uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE + contentLength ];
    uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - 1 ];
    lws_add_http_common_headers( wsi, code, "text/html", contentLength, &p, end );
    lws_finalize_http_header( wsi, &p, end );
    {
        memcpy( p, prefix.c_str(), prefix.size() );
        memcpy( p + prefix.size(), errorMessage.c_str(), errorMessage.size() );
        p += contentLength;
    }
    lws_write( wsi, start, lws_ptr_diff( p, start ), LWS_WRITE_HTTP_FINAL );
    return 1;
}

}  // namespace Utils
}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
