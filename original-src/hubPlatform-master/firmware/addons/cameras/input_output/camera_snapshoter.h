#pragma once

#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/resolution.h"
#include "cameras/packet/packets_buffer.h"
#include "cameras/codec/codec_context.h"
#include "cameras/frame/frame.h"
#include "cameras/defs.h"

#include <functional>
#include <tiny/tiny_thread_pool.h>

namespace HubPlatform {

class AVPacketPrototype;

class CameraSnapshoter final {
public:
    struct InputStream {
        enum AVPixelFormat pixelFormat;
        Resolution resolution;
        int64_t bit_rate;
        AVRational time_base;
    };

    CameraSnapshoter( std::string snapshoterId );
    CameraSnapshoter( const CameraSnapshoter& ) = delete;
    CameraSnapshoter( CameraSnapshoter&& ) = delete;

    void init( const ICameraInput& cameraInput );
    bool makeSnapshot( StorageSharedPtr storage,
                       const ResultCallback& resultCallback,
                       const std::string& requestId,
                       const std::shared_ptr< Resolution >& resolution = nullptr );
    bool isInitialized() const;

private:
    struct DecodeResult {
        DecodeResult() = default;
        int code = 0;
        bool gotResult = false;
        AVFrameWrapper frame;
    };

    struct EncodeResult {
        EncodeResult() = default;
        int code = 0;
        bool gotResult = false;
        AVPacketWrapper packet;
    };

    void receivePacket( const AVPacketPrototype& frame );
    bool waitAndProcessPacket( FileSender& fileSender,
                               AVCodecContext* avCodecContext,
                               const ResultCallback& resultCallback );
    bool processFrame( FileSender& fileSender,
                       AVCodecContext* avCodecContext,
                       AVFrame* avInputFrame,
                       const ResultCallback& resultCallback );
    Json createError( const std::string& errorMessage ) const;
    static DecodeResult decodeVideo( AVCodecContext* codecContext, const AVPacket& packet );
    static EncodeResult encodeVideo( AVCodecContext* codecContext, const AVFrame& frame );
    void removeExpiredSnapshots( std::chrono::seconds expirationPeriod );

private:
    uint32_t m_snapshotCounter = 0;
    PacketsBuffer m_packetBuffer;
    std::atomic_bool m_isRunning;
    PacketHandlerSharedPtr m_packetHandler;
    tiny::thread_pool m_threadPool;
    bool m_isInitialized = false;
    InputStream m_inputStream;
    AVCodec* m_outputCodec = nullptr;
    AVCodecContextWrapper m_inputCodecContext;
    int m_videoStreamIndex = 0;
    std::chrono::seconds m_timeout;
    std::string m_snapshoterId;

private:
    static const size_t NumberOfThreads = 1;  // should be 1 thread otherwise it works incorrect
};

}  // namespace HubPlatform
