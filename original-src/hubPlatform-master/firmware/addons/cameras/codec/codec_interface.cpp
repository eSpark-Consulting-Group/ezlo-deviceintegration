#include "cameras/codec/codec_interface.h"

namespace HubPlatform {
namespace Cameras {

ICodec::ICodec( int streamIndex, int64_t bitrate, CodecType codecType, std::string codecName )
: m_streamIndex( streamIndex )
, m_bitrate( bitrate )
, m_codecType( codecType )
, m_codecName( std::move( codecName ) ) {}

CodecType ICodec::getCodecType() const {
    return m_codecType;
}

std::string ICodec::getCodecName() const {
    return m_codecName;
}

int ICodec::getStreamIndex() const {
    return m_streamIndex;
}

int64_t ICodec::getBitrate() const {
    return m_bitrate;
}

}  // namespace Cameras
}  // namespace HubPlatform
