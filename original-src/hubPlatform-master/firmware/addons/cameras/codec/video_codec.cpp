#include "cameras/codec/video_codec.h"

namespace HubPlatform {
namespace Cameras {

MediaType VideoCodec::getMediaType() const {
    return MediaType::VIDEO;
}

}  // namespace Cameras
}  // namespace HubPlatform
