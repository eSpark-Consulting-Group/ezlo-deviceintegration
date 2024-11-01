#pragma once
#include <cstdint>
#include <string>

namespace HubPlatform {
namespace Cameras {

enum class MediaType { VIDEO, AUDIO };

enum class CodecType { VIDEO_H264, VIDEO_H265, VIDEO_MPEG4, AUDIO_PCM_ALAW, AUDIO_PCM_MULAW, AUDIO_AAC };

class ICodec {
public:
    ICodec( int streamIndex, int64_t bitrate, CodecType codecType, std::string codecName );

    virtual ~ICodec() = default;
    virtual MediaType getMediaType() const = 0;
    virtual CodecType getCodecType() const;
    virtual std::string getCodecName() const;

    int getStreamIndex() const;
    int64_t getBitrate() const;

    int audioChannels;
    int audioSampleRate;

private:
    const int m_streamIndex;
    const int64_t m_bitrate;
    const CodecType m_codecType;
    const std::string m_codecName;
};

}  // namespace Cameras
}  // namespace HubPlatform
