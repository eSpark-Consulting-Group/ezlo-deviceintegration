#include "cameras/codec/audio_codec.h"

namespace HubPlatform {
namespace Cameras {

MediaType AudioCodec::getMediaType() const {
    return MediaType::AUDIO;
}

}  // namespace Cameras
}  // namespace HubPlatform
