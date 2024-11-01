#pragma once

#include "cameras/input_output/resolution.h"
#include "cameras/packet/packets_buffer.h"
#include "cameras/codec/codec_params.h"
#include "cameras/codec/codec_context.h"
#include "cameras/defs.h"
#include "system_utils/filesystem.h"

#include <string>
#include <map>
#include <thread>
#include <future>
#include <atomic>

namespace HubPlatform {

class VideoConverter;
class AudioConverter;

class CameraOutput final {
public:
    using StreamIndex = int;

    CameraOutput( StorageSharedPtr storage,
                  std::string fileNameStart,
                  std::chrono::seconds prerecordedDuration,
                  std::chrono::seconds segmentDuration,
                  int segmentsLimit,
                  const std::string& format );
    ~CameraOutput();

    int getPrerecordedDuration() const;
    int getSegmentDuration() const;
    int getSegmentsLimit() const;

    ///
    /// \brief add*Stream
    /// \param streamIndex
    /// \param inputStream
    /// \param outputResolution resolution to which the input stream will be converted
    ///                         if output and input resolutions match - no convertion will be done
    /// Initializes output streams from input streams. Must be called before start().
    void addVideoStream( int streamIndex, const AVStream& inputStream, Resolution outputResolution );
    void addAudioStream( int streamIndex, const AVStream& inputStream );

    void receivePacket( const AVPacketPrototype& packet );

    ///
    /// \brief Starts separate thread and waits record start by start() method.
    ///         If start() method was called before this method then record starts automatically
    /// \return future to result. 0 is success result
    ///
    std::future< int > allowRecord();

    ///
    /// \brief Starts separate thread and waits record start by start method.
    /// \param maxDurationSec total record duration. It could be separated to equal segments
    ///
    void start( std::chrono::seconds maxDurationSec );
    bool isRecording() const noexcept;

    /// \brief start first segment for single thread solution. Do not use it along start() method
    int startSegment();
    int stopSegment();

    /// \brief handle next packet for single thread solution. Do not use it along start() method
    int processPacket( AVPacketPrototype& packet );

    ///
    /// \brief stop
    /// Finishes recording normally, i.e. trailer of the current segment will be written.
    void stop();

    ///
    /// \brief cancel
    /// Finishes recording emergently, called from destructor.
    void cancel();

    StorageSharedPtr getStorage() const;

private:
    int postProcessPacket( AVPacketPrototype& packet );

    int segmentStart( bool reset );
    int segmentEnd();

    ///
    /// \brief checkStreamIndex
    /// \param index stream index
    /// \return
    /// returns true if we are writing this stream to output
    bool checkStreamIndex( int index ) const;

    void fixPacketTimestamps( AVPacket& packet, int64_t lastDts ) const;

private:
    std::thread m_worker;
    std::atomic_bool m_shouldStart;
    std::atomic_bool m_shouldStop;
    std::atomic_bool m_shouldCancel;
    std::atomic_bool m_stopped;
    PacketsBuffer m_buffer;
    StorageSharedPtr m_storage;

    const std::string m_fileNameStart;

    struct StreamParams {
        AVRational inputTimeBase = {};
        AVCodecParamsWrapper codecParams;
        Resolution outputResolution;
    };

    using TStreamParams = std::map< int /*stream index*/, StreamParams >;
    TStreamParams m_streamParams;

    unsigned char* m_memBuffer;
    int m_segmentCount = 0;
    int64_t m_segmentDurationMcs;
    int64_t m_desiredDurationMcs = 0;

    struct Segment {
        Segment( std::string fileName, std::string outputMuxer, FileSenderSharedPtr fileSender );
        Segment( const Segment& ) = delete;
        ~Segment();
        int init( unsigned char* memBuffer, const TStreamParams& params );
        void deinit();
        int finalize();
        int write( AVPacket& packet );
        static int writePacket( void* opaque, uint8_t* buf, int bufSize );

        void storeStartTimestamp();

        const std::string fileName;
        const std::string outputMuxer;
        FileSenderSharedPtr fileSender;
        AVOutputFormat* outputFormat = nullptr;
        AVFormatContext* outputFormatContext = nullptr;
        AVIOContext* outputIoContext = nullptr;

        std::unique_ptr< VideoConverter > m_videoConverter;
        std::unique_ptr< AudioConverter > m_audioConverter;

        int referenceStream = -1;
        int segmentPacketCount = 0;
        bool recordingStartStored = false;
        int64_t segmentSize = 0;
    };
    std::unique_ptr< Segment > m_currentSegment;

    int64_t m_prerecordedDurationMcs;
    int m_segmentsLimit;
    std::string m_fileExtension;
    std::string m_outputMuxer;
    std::deque< AVPacketPrototype > m_prerecorded;
    const std::size_t m_prerecordBufferSize;
    std::map< StreamIndex, int64_t > m_lastDts;
    std::map< StreamIndex, int64_t > m_offsets;
    int m_recordNumber = 0;
};
}  // namespace HubPlatform
