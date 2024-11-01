#pragma once

#include "cameras/codec/codec_interface.h"

namespace HubPlatform {
namespace Cameras {

class VideoCodec : public ICodec {
public:
    using ICodec::ICodec;
    MediaType getMediaType() const override;
};

}  // namespace Cameras
}  // namespace HubPlatform
